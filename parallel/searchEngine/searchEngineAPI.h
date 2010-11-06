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
  UShort prioBytes;

  public:

  virtual bool empty() = 0;
  virtual State *head() = 0;
  virtual State *dequeue() = 0;
  virtual State *registerState(size_t size, unsigned char childnum) = 0;
  virtual void push(State *state) = 0;
  virtual void embed(State *state, size_t size) = 0;
  virtual void embed(State *states, size_t *sizes, int nstates) = 0;
  virtual int length(){ return len; }
  virtual void setCurrentPriority(unsigned int *__p, UShort len){
    currentPrioBits = __p;
    prioBytes = len;
  }
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

  inline State *registerState(size_t size, unsigned char childnum){
    // 1. get parent's (current) priority 
    // 2. calculate child's priority by shifting current 
    // 3. set priority bits and charm queuing strategy
    
    // extend priority bit vector by 1 byte at a time
    int extraBytes = 0; 
    //int extraBytes = sizeof(unsigned char); 

    SearchNodeMsg *msg = new (size,extraBytes+prioBytes) SearchNodeMsg;
    unsigned char *childPrioBits = (unsigned char *)CkPriorityPtr(msg);
    //if(prioBytes > 0){
    //  memcpy(childPrioBits, currentPrioBits, prioBytes);
    //}
    //childPrioBits[prioBytes] = childnum;

    CkSetQueueing(msg,CK_QUEUEING_LIFO);
    //CkSetQueueing(msg,CK_QUEUEING_BLIFO);
    msg->size = size;
    return (State *) msg->objectDump;
  }

  inline void push(State *state){
    SearchNodeMsg *msg = (SearchNodeMsg *)((char *)state-msg_offset); 
    spawn(msg);
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

  inline State *registerState(size_t size, unsigned char childnum){             
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
