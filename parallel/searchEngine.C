#ifdef USING_CONTROLPOINTS
#include <controlPoints.h>
#endif

#include "searchEngine.decl.h"
#include "searchEngine.h"

#include "searchEngineAPI.h"

#include "cmipool.h"

/*readonly*/  int sequential_threshold;
/*readonly*/  size_t msg_offset;

#define MAX_BFS_STACK_THRESH 0.5
#define MAX_DFS_STACK_THRESH 0.8
#define INC_STACK_SIZE_FACT 1.5
#define DEC_STACK_SIZE_FACT 0.8
#define SPLIT_CHARES 5

#ifdef USING_CONTROLPOINTS
CProxy_BThreshold threshGroup;

int cp_grainsize;
int THRESH_MAX;
int THRESH_MIN;
void SearchConductor::controlChange(controlPointMsg* msg) {
    controlPointTimingStamp();
    cp_grainsize = controlPoint("grainsize", THRESH_MIN, THRESH_MAX);

    ThreshMsg *msg1 = new ThreshMsg(cp_grainsize);
    threshGroup.changeThreshold(msg1);
}
class BThreshold : public CBase_BThreshold {
public:
    BThreshold() {}

    void changeThreshold(ThreshMsg *msg) {
        cp_grainsize = msg->threshold;
    }
};

#endif

/* for piority message */
//#ifdef PRIORITY_USAGE
#define SIZEINT sizeof(int)
#define SIZEINTBITS (sizeof(int)*8)
#define BITSNUM (PRIOR_INT  * sizeof(int) * 8)
//#endif

//declare rovs
//searchEngineProxy will be expose to the user space
CProxy_SearchConductor searchEngineProxy;
//group proxy
CProxy_SearchGroup groupProxy;


//implementation of SearchConductor
SearchConductor::SearchConductor( CkArgMsg *m )
{
	searchEngineProxy = thisProxy;
numStates=0;	
        if(m->argc < 2){
          CkAbort("usage: program <sequential_threshold> [pgm args...]\n");
        }

        sequential_threshold = atoi(m->argv[1]);

        CkPrintf("sequential_threshold %d\n", sequential_threshold);
	//group initialize
        DUMMYMSG *_mm = new  DUMMYMSG;
	groupProxy = CProxy_SearchGroup::ckNew(_mm);
        delete m;

        SearchNodeMsg *test = new (2,0) SearchNodeMsg;
        msg_offset = (char *)test->objectDump-(char *)test;
        
        CkPrintf("msg_offset %d\n", msg_offset);
        delete test;
}

//when users call this method, the search engine start searching
void SearchConductor::start()
{

	//init groups, and wait for them
    groupInitCount = 0;
    groupProxy.init();
}

//when group branches complete the initialization, they will call this method to notify the mainchare
void SearchConductor::groupInitComplete()
{
	groupInitCount ++;
	//wait for all group branches to init
	if( groupInitCount == CkNumPes() )
	{
#ifdef USING_CONTROLPOINTS
        ControlPoint::EffectIncrease::Concurrency("grainsize");
        cp_grainsize = controlPoint("grainsize", THRESH_MIN, THRESH_MAX);
        threshGroup = CProxy_BThreshold::ckNew();
        CkCallback cb(CkIndex_SearchConductor::controlChange(NULL), searchEngineProxy);
        registerCPChangeCallback(cb, true);
#endif
 
        groupInitCount = 0;
        //start the real searching
		fire();
	}
}

void SearchConductor::fire()
{
	
    int myStartDepth = 0;
    int mySearchDepth = 3;
    int childPrior = 0;
    engineCore = groupProxy.ckLocalBranch()->getCore();
    //currentSearchDepth = engineCore->getInitialSearchDepth();

    //*************
    // start timer
    //*************
    startTime = CkWallTimer();
#ifdef FIRST_SOLUTION
    SearchGroup *grp = groupProxy.ckLocalBranch();
    grp->setStartTime(startTime);
#endif


    //NodeQueue *qs = engineCore->getInitialNodeQueue();
    SingleNodeQueue qs;
    engineCore->getStartState(&qs);
#ifdef SE_DEBUG
    CkPrintf("fire: queue length %d\n", qs.length());
#endif
  
    //SimpleWalk walk;
    //walk.start(&qs,engineCore);

    //set the QD call back function
    CkStartQD(CkIndex_SearchConductor::allSearchNodeDone((DUMMYMSG *)0), &thishandle);
}

//QD call back function, it means all the searching is complete in current search depth
void SearchConductor::allSearchNodeDone( DUMMYMSG *msg ){
    int numSolutions = groupProxy.ckLocalBranch()->getTotalCount();
    //SpaceStatestarSearchCore *core = groupProxy.ckLocalBranch()->getCore();

    //CkPrintf("Depth %2d completed\n", currentSearchDepth);
#ifdef CHARECOUNT
    CkPrintf("search nodes:%d\n", chareNum);
#endif
	//check if found the solution
	if( numSolutions > 0 ){
          CkPrintf( "%d solutions found in %lf sec, with %d processors\n", numSolutions, CkWallTimer()-startTime, CkNumPes() );	
          CkExit(); 
        }
	else{
          //increase the search depth
          currentSearchDepth = -1; 
          //currentSearchDepth = engineCore->getNextSearchDepth(currentSearchDepth); 
#ifdef CHARECOUNT
          chareNum = 0;
#endif
          if( currentSearchDepth < 0 ){
            //|| currentSearchDepth > engineCore->searchDepthLimit() ){
    CkPrintf( "Allsearchnodedone:  solution is found in %lf sec, with %d processors\n",  CkWallTimer()-startTime, CkNumPes() );	
			CkPrintf("my sols=%d\n",numStates);
            CkExit();
            return;
          }
          groupProxy.searchDepthChange(currentSearchDepth, CkCallbackResumeThread());

          CkPrintf( "Start searching with depth: %d\n", currentSearchDepth );
          int myStartDepth = 0;
          //NodeQueue *qs = engineCore->getInitialNodeQueue();
          SingleNodeQueue qs;
          engineCore->getStartState(&qs);


          //SimpleWalk walk;
          //walk.start(&qs,engineCore);

#ifdef PRIORITY_USAGE
          int childrenNum = iter->size();
          int logChildren = __log(childrenNum);
          int children_index = 0;
          int shift = SIZEINTBITS- logChildren;
#endif
          //set QD callback function
          CkStartQD(CkIndex_SearchConductor::allSearchNodeDone((DUMMYMSG *)0), &thishandle);
        }
}

void SearchConductor::foundSolution()
{
    //CkPrintf("Depth %2d completed\n", currentSearchDepth);
#ifdef CHARECOUNT
    CkPrintf("search nodes:%d\n", chareNum);
#endif
    CkPrintf( "One solution is found in %lf sec, with %d processors\n",  CkWallTimer()-startTime, CkNumPes() );	
    CkExit(); 
}


void SearchConductor::resetFactory()
{

}
		 
void SearchConductor::newSearchNode()
{
	chareNum ++;
}


/* used to count the total number of solutions if all solutions are set */
SearchGroup::SearchGroup(DUMMYMSG *m){ 
  // initialize the local count;
  mygrp = thisgroup;
  myCount = totalCount = 0;
  waitFor = CkNumPes(); // wait for all processors to report
  threadId = NULL;
#ifdef FIRST_SOLUTION
  conductorOnSamePE = false;
  solutionFound = false;
#endif
}

void SearchGroup::increment()
{
  myCount++;
}

#ifdef FIRST_SOLUTION
void SearchGroup::setStartTime(double s){
  conductorOnSamePE = true;
  startTime = s;
}

void SearchGroup::killSearch()
{
  solutionFound = true;
  if(conductorOnSamePE){
    conductorOnSamePE = false;
    CkPrintf("[%d] First solution time: %f s\n", CkMyPe(), CkWallTimer()-startTime);
  }
    CkPrintf( "Kill search solution is found in %lf sec, with %d processors\n",  CkWallTimer()-startTime, CkNumPes() );	
  CkExit();
}

bool SearchGroup::querySolutionFound(){
  return solutionFound;
}

bool querySolutionFound(){ 
  return groupProxy.ckLocalBranch()->querySolutionFound();
}
#endif

void SearchGroup::sendCounts(DUMMYMSG *m)
  // This method is invoked via a broadcast. Each branch then reports 
  // its count to the branch on 0 (or via a spanning tree.)
{
  CProxy_SearchGroup grp(mygrp);
  grp[0].childCount(new countMsg(myCount));
  delete m;
}

void SearchGroup::childCount(countMsg *m)
{
  totalCount += m->count;
  waitFor--;
  if (waitFor == 0) 
    if (threadId) { CthAwaken(threadId);}
}

int SearchGroup::getTotalCount()
{
  CProxy_SearchGroup grp(mygrp);
  grp.sendCounts(new DUMMYMSG);//this is a broadcast, as no processor is mentioned
  threadId = CthSelf();
  while (waitFor != 0)  CthSuspend(); 
  return totalCount;
}

SearchGroup::~SearchGroup()
{
	if( myCore != NULL )
		delete myCore;
}

AppCore *newAppCore();
void SearchGroup::init()
{
    //create new search core
    myCore = newAppCore(); 

#ifdef USING_CONTROLPOINTS
    THRESH_MIN = myCore->minimumLevel();
    THRESH_MAX = myCore->maximumLevel();
#endif
    //tell the mainchare that this branch is finished
    searchEngineProxy.groupInitComplete();
}

void SearchGroup::setParallelLevel( int level )
{
	parallelize_level = level;
}

void SearchGroup::searchDepthChange( int depth, CkCallback &cb )
{
	//myCore->searchDepthChangeNotify( depth );
        contribute(0,0,CkReduction::concat,cb);

}

inline int SearchGroup::getParallelLevel()
{
	return parallelize_level;
}

AppCore* SearchGroup::getCore()
{
	return myCore;
}

//implementation of SearchNode

SearchNode::SearchNode(SearchNodeMsg *msg){
  myGroup = groupProxy.ckLocalBranch();
  myCore = myGroup->getCore();
  //int level = msg->startDepth;

  //NodeQueue *qs = myCore->getNodeQueue((State *)msg->objectDump);
  SingleNodeQueue qs;
  //myCore->unpack(&qs, (State *)(msg->objectDump), msg->size);
  qs.embed((State *)(msg->objectDump), msg->size);

#ifdef USE_PRIORITIES
  unsigned int *_priority = (unsigned int *)(CkPriorityPtr(msg));
  UShort len = UsrToEnv(msg)->getPriobits();
  qs.setCurrentPriority(_priority, len);
#endif

#ifdef MEMCHECK
//  CkPrintf("after walk\n");
//  CmiMemoryCheck();
#endif
  
  SimpleWalk walk;
  walk.start(&qs, myCore);
#ifdef MEMCHECK
//  CkPrintf("after walk\n");
// CmiMemoryCheck();
#endif

  delete msg;
  delete this;
}

void SimpleWalk::start(NodeQueue *qs, AppCore *core){
#ifdef SE_DEBUG
  int ndeq = 0;
  CkPrintf("walk start queue size %d\n", qs->length());

#endif
  //qs->testPriorities();
  //CkExit();
  while(!qs->empty()){
#ifdef SE_DEBUG
    ndeq++;
#endif
    State *state = qs->dequeue();
    if(core->isGoal(state)){
      //CkPrintf("*************** GOAL ****************\n");
      foundSolution();
    }
    else{
      core->expand(state,qs);
    }
  }
#ifdef SE_DEBUG
  CkPrintf("walk %d nodes dequeued\n", ndeq);
#endif
}

void spawn(SearchNodeMsg *msg){
  //*CkPriorityPtr(msg) = pb;
  CProxy_SearchNode::ckNew(msg, NULL, -1);
}

void foundSolution(){
#ifdef FIRST_SOLUTION
    //CkPrintf("one solution found\n");
    //CkExit();
    groupProxy.killSearch();
#else
  groupProxy.ckLocalBranch()->increment();
#endif
}

#include "searchEngine.def.h"
