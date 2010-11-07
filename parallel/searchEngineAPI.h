#ifndef __SEARCHENGINEAPI__
#define __SEARCHENGINEAPI__


#ifdef USE_CMIPOOL
#include "cmipool.h"
#endif

#include "searchEngine.h"
#include "pup_stl.h"

/*   framework for search engine */

/* The tree node state */

class State{
};


class NodeQueue; 
class Walk; 

class AppCore{
public:
        virtual void getStartState(NodeQueue *qs) = 0;
        virtual void expand(State *state, NodeQueue *qs) = 0;
        virtual bool isGoal(State *s) = 0;
};

/*********/
/* NodeQueue */
/*********/

class NodeQueue {
  protected:
  int len;
  unsigned int *currentPrioBits;
  UShort prioBits;
  UShort prioWords;

  public:

  virtual bool empty() = 0;
  virtual State *head() = 0;
  virtual State *dequeue() = 0;
#ifdef USE_PRIORITIES
  virtual State *registerState(size_t size, unsigned int childnum, unsigned int totalNumChildren) = 0;
  virtual State *registerRootState(size_t size) = 0;
#else
  virtual State *registerState(size_t size) = 0;
#endif
  virtual void push(State *state) = 0;
  virtual void embed(State *state, size_t size) = 0;
  virtual void embed(State *states, size_t *sizes, int nstates) = 0;
  virtual int length(){ return len; }
  virtual void setCurrentPriority(unsigned int *__p, UShort pb){
    currentPrioBits = __p;
    prioBits = pb;
    prioWords = CkPriobitsToInts(pb); 
  }

  NodeQueue(){
  }

  virtual void testPriorities() = 0;
};

/*********/
/* Walk  */
/*********/

class Walk {
  public:
  virtual void start(NodeQueue *, AppCore *) = 0;
};

class SimpleWalk;
class LIFONodeQueue;
class SingleNodeQueue; 

void spawn(SearchNodeMsg *msg);
void foundSolution();

inline int __se_log(int n)
{
    int _mylog = 0;
    for(n=n-1;n>0;n=n>>1)
    {
        _mylog++;
    }
    return _mylog;
}

class SingleNodeQueue : public NodeQueue {
  
  public:
  bool fresh;

  State *parent;
  SingleNodeQueue(){
    len = 0;
    fresh = true;
  }

  inline bool empty(){
    return len == 0;
  }

  inline State *head(){
    return (State *)parent;
  }

  inline State *dequeue(){
    len = 0;
    return (State *)parent;
  }

  inline SearchNodeMsg *stateToMsg(State *st){
    return ((SearchNodeMsg *)((char *)st-msg_offset)); 
  }


#ifdef USE_PRIORITIES
  inline State *registerRootState(size_t size){
    UShort rootBits = 1;
    SearchNodeMsg *msg = new (size, rootBits) SearchNodeMsg;
    unsigned int *pbits = (unsigned int *)CkPriorityPtr(msg);
    *pbits = 0;//(1<<rootBits)-1;
    CkSetQueueing(msg, CK_QUEUEING_BLIFO);
    msg->size = size;

    currentPrioBits = pbits;
    prioBits = rootBits;
    prioWords = CkPriobitsToInts(prioBits);

    return (State *) msg->objectDump;
  }

  inline State *registerState(size_t size, unsigned int childnum, unsigned int totalchildren){

    UShort extraBits = __se_log(totalchildren);
    CkAssert(extraBits <= CkIntbits);
    UShort newpbsize = extraBits+prioBits;
    SearchNodeMsg *msg = new (size, newpbsize) SearchNodeMsg;
    CkSetQueueing(msg, CK_QUEUEING_BLIFO);
    setMsgPriority(msg, extraBits, childnum, CkPriobitsToInts(newpbsize));
    msg->size = size;
    return (State *) msg->objectDump;
  }

  // newpw is the number of words in the msg's priority bitvector
  void setMsgPriority(SearchNodeMsg *msg, UShort childbits, unsigned int childnum, UShort newpw){
    // copy parent's priority to extend it.
    unsigned int *newPriority = (unsigned int *)(CkPriorityPtr(msg));
    
    for(int i=0; i<prioWords; i++)
    {
       newPriority[i] = currentPrioBits[i]; 
    }

    int shiftbits = 0;
    // Two cases 1: with new bits, we do not have to append the number of integers
    if(newpw == prioWords)
    {
        if((childbits+prioBits) % (8*sizeof(unsigned int)) != 0)
            shiftbits = 8*sizeof(unsigned int) - (childbits+prioBits)%(8*sizeof(unsigned int));
        newPriority[prioWords-1] = currentPrioBits[prioWords-1] | (childnum << shiftbits);
    }else if(newpw>prioWords)
    {
        /* have to append a new integer */
        if(prioBits % (8*sizeof(unsigned int)) == 0)
        {
            shiftbits = sizeof(unsigned int)*8 - childbits;
            newPriority[prioWords] = (childnum << shiftbits);
        }else /*higher bits are appended to the last integer and then use anothe new integer */
        {
            int inusebits = prioBits % ( 8*sizeof(unsigned int));
            unsigned int higherbits =  childnum >> (childbits - inusebits);
            newPriority[prioWords-1] = currentPrioBits[prioWords-1] | higherbits; 

            /* lower bits are stored in new integer */
            newPriority[prioWords] = childnum << (8*sizeof(unsigned int) - childbits + inusebits);
        }
    }

  }

  void printPriority(SearchNodeMsg *pm){
    UShort pw = UsrToEnv(pm)->getPrioWords();
    unsigned int *pr = (unsigned int *)(CkPriorityPtr(pm));
    for(int i = 0; i < pw; i++){
      CkPrintf("[%d] 0x%x\n", i, pr[i]);
    }
  }

  void testPriorities(){
    int size = 10;
    int pbsize = 1;
    SearchNodeMsg *m;
    int numchildren = 64;
    int levels = 10;
    State *thisLevelState;

    State *rootState = registerRootState(size);
    CkPrintf("root priority:\n");
    printPriority(stateToMsg(rootState));
    thisLevelState = rootState;

    for(int j = 0; j < levels; j++){
     
        CkPrintf("\n\n parent:");
        printPriority(stateToMsg(thisLevelState));
        setCurrentPriority(
            (unsigned int *)(CkPriorityPtr(stateToMsg(thisLevelState))),
        UsrToEnv(stateToMsg(thisLevelState))->getPriobits()
      );

      for(int i = 0; i < numchildren; i+=32){
        State *childState = registerState(size, i, numchildren);
        CkPrintf("child %d priority:\n", i);
        printPriority(stateToMsg(childState));
        if(i == 0){
          thisLevelState = childState;
        }
      }

    }

    CkExit();
  }

#else
  inline State *registerState(size_t size){
    SearchNodeMsg *msg = new (size,0) SearchNodeMsg;
    CkSetQueueing(msg,CK_QUEUEING_LIFO);
    msg->size = size;
    return (State *) msg->objectDump;
  }
#endif

  inline void push(State *state){
    spawn(stateToMsg(state));
  }

  inline void embed(State *state, size_t size){
    len = size;
    parent = state;
  }

  void embed(State *states, size_t *sizes, int nstates){
    CkAbort("Cannot use multi-node embedding with SingleNodeQueue\n");
  }

};

class LIFONodeQueue : public NodeQueue { 
  // data is stored here
  char *memory;
  // size of each entry is stored here
  int *sizes;
  char *writeptr;
  char *bdry;
  int curentry;
  int nmax;
  //int threshold;                                                     

  public:                                                            
  LIFONodeQueue(int max, int n){ 
    memory = (char *)CmiPoolAlloc(max); 
    sizes = (int *)CmiPoolAlloc(n);
    nmax = n-1;
    curentry = -1;
    bdry = memory+max; 
    writeptr = memory;
  }
  ~LIFONodeQueue(){
    CmiPoolFree(memory); 
    CmiPoolFree(sizes);
  }                         

  void testPriorities() {}
  inline bool empty(){
    return curentry < 0;
  }

  inline State *head(){
    size_t size = sizes[curentry]; 
    return (State *)(writeptr-size);
  }

  inline State *dequeue(){
#ifdef BOUNDCHECK
    if(curentry < 0){
      CkAbort("Stack underflow (sizes)\n");
    }
#endif
    size_t size = sizes[curentry];
#ifdef SE_DEBUG
    CkPrintf("deq curentry %d writeptr 0x%x size %d memory 0x%x\n", curentry, writeptr, size, memory);
#endif
#ifdef BOUNDCHECK
    if(writeptr-size < memory){
      CkAbort("Stack underflow\n");
    }
#endif
    writeptr -= size;
    curentry--;
    return (State *)writeptr;
  }

  inline State *registerState(size_t size, unsigned int childnum, unsigned int numchildren){             
    State *ret = (State *)writeptr;
#ifdef BOUNDCHECK
    if(writeptr+size >= bdry || curentry > nmax){
      CkAbort("Overlfow: increase DS(stack) maxsize\n");
    }
#endif
    writeptr += size;
    curentry++;
    sizes[curentry] = size;
#ifdef SE_DEBUG
    CkPrintf("registerState memory 0x%x writeptr 0x%x curentry %d size %d\n",
              memory, writeptr, curentry, size);
#endif
    return ret;
  }

  inline void push(State *state){
  }

  inline void embed(State *state, size_t size){
    memcpy(memory, state, size);
    writeptr = memory+size;
    curentry = 0;
    sizes[0] = size;

#ifdef SE_DEBUG
    CkPrintf("embed memory 0x%x writeptr 0x%x curentry %d size %d\n",
              memory, writeptr, curentry, size);
#endif
  }

  inline int length(){
    return curentry+1;
  }

  inline void embed(State *states, size_t *sz, int nstates){
    int totalSize = 0;
    for(int i = 0; i < nstates; i++){
      int size = sz[i];
      totalSize += size;
      sizes[i] = size; 
    }
    memcpy(memory, states, totalSize); 
    curentry = nstates-1;
    writeptr = memory+totalSize;
  }
};

/* SimpleWalk: */
/* always dequeues head of queue and 
   expands it if necessary/possible. */
   
class SimpleWalk : public Walk {
  public:
  void start(NodeQueue *, AppCore *);
};



#endif
