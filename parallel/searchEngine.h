#ifndef __SEARCHENGINE__
#define __SEARCHENGINE__

#include "searchEngine.decl.h"

#ifdef OPTIMIZE
#undef CHARECOUNT
#else
#define CHARECOUNT
#endif

#define PRIOR_INT 4

extern int sequential_threshold;
extern size_t msg_offset;

class DUMMYMSG : public CMessage_DUMMYMSG {
	public:
};

/* for count solutions */
class countMsg : public CMessage_countMsg {
public:
  int count;
  countMsg(int c) : count(c) {};
};

class SearchNodeMsg : public CMessage_SearchNodeMsg
{
public:
        int size;
	char *objectDump;
	
        /*
	int startDepth, searchDepth;
        int numNodes;
        int maxStackSize;
        */

#ifdef PRIORITY_USAGE
    int parray_index;
    int occupybit_index;
#endif
	SearchNodeMsg( int sdepth, int depth )
	{
          /*
		startDepth = sdepth;
		searchDepth = depth;
          */
	}

        SearchNodeMsg(int sdepth, int depth, int nnodes, int stacksize){
          /*
          startDepth = sdepth;
          searchDepth = depth;
          numNodes = nnodes;
          maxStackSize = stacksize;
          */
        }

        SearchNodeMsg(){
          /*
          numNodes = 1;
          */
        }
};

class AppCore;

class SearchConductor : public CBase_SearchConductor
{
public:	
	double startTime;
	long long numStates;
	int currentSearchDepth, mySearchLimit;
	int groupInitCount;
	int solutionFound;
	int chareNum;
	int parallelize_level;
	AppCore *engineCore;
	
        int stateSize;
        int maxStackNodes;

	SearchConductor( CkArgMsg *m );
	
	void allSearchNodeDone( DUMMYMSG *msg );
	void incState() {numStates++;}
    void start();
	void groupInitComplete();
	void resetFactory();
	void foundSolution();
	void newSearchNode();
	
	void fire();

#ifdef USING_CONTROLPOINTS
    void controlChange(controlPointMsg* msg);
#endif
};

#ifdef FIRST_SOLUTION
class SearchGroup : public NodeGroup
#else
class SearchGroup : public Group
#endif
{
private:
    CkGroupID mygrp;
    int myCount;
    int totalCount;
    int waitFor;
    CthThread threadId;
    AppCore *myCore;

#ifdef FIRST_SOLUTION
    double startTime;
    bool conductorOnSamePE;
    bool solutionFound;
#endif

public:
    int parallelize_level;

    SearchGroup(CkMigrateMessage *m) {}
    SearchGroup(DUMMYMSG *);
	~SearchGroup();
    
    void childCount(countMsg *);
    void increment();
    void sendCounts(DUMMYMSG *);
    int  getTotalCount();

#ifdef FIRST_SOLUTION
    void setStartTime(double s);
    void killSearch();
    bool querySolutionFound();
#endif

	void init();
	void setParallelLevel( int level );
	void searchDepthChange(int depth, CkCallback &cb);
	
	AppCore* getCore();
	
	int getParallelLevel();

};

class SearchNode : public CBase_SearchNode
{
public:
	int myStartDepth;
	int mySearchDepth;
#ifdef PRIORITY_USAGE
    int parray_index;
    int occupybit_index;
    int __priority[PRIOR_INT]; 
#endif

	SearchGroup *myGroup;
	AppCore *myCore;
        SearchNodeMsg *myMsg;

	
	SearchNode( CkMigrateMessage *m ){}
	SearchNode( SearchNodeMsg *msg );
	~SearchNode(){}
	
};


//Expose the searchEngineProxy to the users. Charm++ compiler puts the extern statement in the *.def file
extern CProxy_SearchConductor searchEngineProxy;
#ifdef USING_CONTROLPOINTS
class ThreshMsg : public CMessage_ThreshMsg {
public:
    int threshold;
    ThreshMsg(int threshold_) : threshold(threshold_) {}                                                 
};
#endif

#endif
