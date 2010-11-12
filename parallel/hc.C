#include <vector>
#include "searchEngineAPI.h"

#include "hc.h"
#include "gameLogic.h"

using namespace std;
int vertices;
size_t max_stack_size;
int max_stack_nodes;

vector<int> indices;
vector<int> edges;
#define MAX_BRANCH 25
#define MAX_DEPTH 4
#define MAX_CHILDREN 64
//#define DEBUG
//#define ONESOL


class HcState: public State {
public:

	int k;
	uint16_t board[16];
	bool moveWhite;


    HcState()
    {
	/*	k = kk;
		v = vertex;
        path =(int*) malloc(sizeof(int) * num_vertices);
        in_path =(int*) malloc(sizeof(int) * num_vertices);
        counts =(int*) malloc(sizeof(int) * num_vertices);
		*/
    }

    ~HcState()
    {
   /*       if(path != NULL)
            free(path);
		  if(in_path != NULL)
            free(in_path); 
		  if(counts != NULL)
            free(counts);
			*/
    }
    void copyParent(HcState* p)
    {
			/*TODO*/
    	 

    }
/*
    void printInfor()
    {
        for(int i=0; i<=currentrow; i++)
        {
            CkPrintf(" %d  ", places[i]);
        }
        CkPrintf("currentrow= %d \n", currentrow);
    } */
};

class HcCore: public AppCore {
  public:
    HcCore(){
    }

    inline void getStartState(NodeQueue *qs){

        // Ask for enough memory for the initial state;
         //HcState *startState = (HcState *)qs->registerState(sizeof(HcState),1);
         HcState *startState = (HcState *)qs->registerRootState(sizeof(HcState));
				 
	
		 startState->moveWhite = true;
		 startState->k = 0;
		 startState->board={30874,47495,52428,52428,0,0,0,0,0,0,0,0,26214,26214,4660,21281};
		 qs->push(startState);

    }

    inline void expand(State *curState, NodeQueue *qs){
      HcState *parent = ((HcState *)curState);
      int childnum = 0;
	  int parentk = parent->k;
	  uint16_t * state = parent->board;
	  int moveWhite = parent->moveWhite;

#ifdef DEBUG
		CkPrintf("Expanding state with depth %d  : \n", parentk);
#endif
      if(parentk < sequential_threshold){

			  //Check if it is a solution 
	if(parentk == MAX_DEPTH )
	{
#ifdef DEBUG
		CkPrintf("Goal reached \n"); 
#endif
		return;
	}
	//statesExpanded++;

	/* Node eaxpansion logic ..... ***************************************/
	 uint16_t allowMoves=0;
	if(moveWhite)	
	{

			int i;
		for(i=0;i<64;i++)
		{
		//	CkPrintf("Bef i %d\n",i);
			int newStates=0;
			// If it is a white piece then try moving it
			int val=getBlockVal(state,i);
//			if(val<7 && val>0)
			{
#ifdef DEBUG
				if(i==49) CkPrintf("A white piece at %d\n",val);
#endif
				uint16_t **newstates;
				newstates =NULL;
        			if(val==1) newStates=moveRook(state,moveWhite,i,&newstates,false);
			        else if(val==2) newStates=moveKnight(state,moveWhite,i,&newstates);
			        else if(val==3) newStates=moveBishop(state,moveWhite,i,&newstates,false);
			        else if(val==4) newStates=moveKing(state,moveWhite,i,&newstates);
			        else if(val==5) newStates=moveQueen(state,moveWhite,i,&newstates);
			        else if(val==6) newStates=movePawn(state,moveWhite,i,&newstates);				
				allowMoves+=newStates;

#ifdef DEBUG
				if(i==54 && newStates>0)
				{
//					printf("DDDDDDDDDDDDDDDDDDDDd newStates=%d\n",newStates);
					for(int i=0;i<newStates;i++)
	                        	       printState(newstates[i]);
				}
#endif

	/* ***************************************/
// Push the new states onto queue 
				int j;
				for(j =0; j <newStates; j++) 
				{
				HcState *child = (HcState *)qs->registerState(sizeof(HcState), childnum, MAX_CHILDREN);
			//	HcState *child = (HcState *)qs->registerState(sizeof(HcState));
				child->k = parentk+1;
				child->moveWhite = false;
				copy(&newstates[j][0], &newstates[j][16], child->board);
				qs->push(child);
				childnum++;
				//Free newStates
				delete [] newstates[j];
						
				}

				if(newStates !=0) 
					delete [] newstates;

				/* ***************************************/
// Pushed the new states onto queue 

			}
		}
#ifdef DEBUG
		CkPrintf("White has %d allowed moves\n",allowMoves);
#endif
	}
        else
        {
				int i;
                for(i=0;i<64;i++)
                {
                        int newStates=0;
                        // If it is a white piece then try moving it
                        int val=getBlockVal(state,i);
//                      if(val<7 && val>0)
                        {
#ifdef DEBUG
                                if(i==8) printf("A white piece at %d\n",val);
#endif
                                uint16_t **newstates;
                                if(val==7) newStates=moveRook(state,moveWhite,i,&newstates,false);
                                else if(val==8) newStates=moveKnight(state,moveWhite,i,&newstates);
                                else if(val==9) newStates=moveBishop(state,moveWhite,i,&newstates,false);
                                else if(val==10) newStates=moveKing(state,moveWhite,i,&newstates);
                                else if(val==11) newStates=moveQueen(state,moveWhite,i,&newstates);
                                else if(val==12) newStates=movePawn(state,moveWhite,i,&newstates);
                                allowMoves+=newStates;
#ifdef DEBUG
                                if(i==47 && newStates>0)
                                {
//                                        printf("DDDDDDDDDDDDDDDDDDDDd i=%d newStates=%d\n",i,newStates);
                                      for(int i=0;i<newStates;i++)
                                              printState(newstates[i]);
                                }
#endif
				int j;
				for(j =0; j <newStates; j++) 
				{
				HcState *child = (HcState *)qs->registerState(sizeof(HcState), childnum, MAX_CHILDREN);
			//	HcState *child = (HcState *)qs->registerState(sizeof(HcState));
				child->k = parentk+1;
				child->moveWhite = true;
				copy(&newstates[j][0], &newstates[j][16], child->board);
				qs->push(child);
				childnum++;
				//Free newStates
				delete [] newstates[j];
						
				}

				if(newStates !=0)
				delete [] newstates;
                        }
                }
#ifdef DEBUG
                CkPrintf("Black has %d allowed moves\n",allowMoves);
#endif
        }

	/* Node expansion logic  finish..... ***************************************/
	//							foundSolution();
	  }
	 else{
        recursive_hc(parent);
      }
    }

    void recursive_hc(HcState *parent){
      int childnum = 0;
	  int parentk = parent->k;
	  uint16_t * state = parent->board;
	  int moveWhite = parent->moveWhite;

	if(parentk == MAX_DEPTH )
	{
#ifdef DEBUG
		CkPrintf("Goal reached \n"); 
#endif
		return;
	}
				/* Node expansion logic ..... ***************************************/
//	statesExpanded++;

	 uint16_t allowMoves=0;
	if(moveWhite)	
	{

			int i;
		for(i=0;i<64;i++)
		{
		//	CkPrintf("Bef i %d\n",i);
			int newStates=0;
			// If it is a white piece then try moving it
			int val=getBlockVal(state,i);
//			if(val<7 && val>0)
			{
#ifdef DEBUG
				if(i==49) CkPrintf("A white piece at %d\n",val);
#endif
				uint16_t **newstates;
        			if(val==1) newStates=moveRook(state,moveWhite,i,&newstates,false);
			        else if(val==2) newStates=moveKnight(state,moveWhite,i,&newstates);
			        else if(val==3) newStates=moveBishop(state,moveWhite,i,&newstates,false);
			        else if(val==4) newStates=moveKing(state,moveWhite,i,&newstates);
			        else if(val==5) newStates=moveQueen(state,moveWhite,i,&newstates);
			        else if(val==6) newStates=movePawn(state,moveWhite,i,&newstates);				
				allowMoves+=newStates;

#ifdef DEBUG
				if(i==54 && newStates>0)
				{
//					printf("DDDDDDDDDDDDDDDDDDDDd newStates=%d\n",newStates);
					for(int i=0;i<newStates;i++)
	                        	       printState(newstates[i]);
				}
#endif

	/* ***************************************/
// Push the new states onto queue 
				int j;
				for(j =0; j <newStates; j++) 
				{
				HcState *child ;
				//	HcState *child = (HcState *)qs->registerState(sizeof(HcState));
				child->k = parentk+1;
				child->moveWhite = false;
				copy(&newstates[j][0], &newstates[j][16], child->board);
				recursive_hc(child);
				//Free newStates
				delete [] newstates[j];
						
				}
				if(newStates !=0)
				delete [] newstates;
	/* ***************************************/
// Pushed the new states onto queue 

			}
		}
#ifdef DEBUG
		CkPrintf("White has %d allowed moves\n",allowMoves);
#endif
	}
        else
        {
				int i;
                for(i=0;i<64;i++)
                {
                        int newStates=0;
                        // If it is a white piece then try moving it
                        int val=getBlockVal(state,i);
//                      if(val<7 && val>0)
                        {
#ifdef DEBUG
                                if(i==8) printf("A white piece at %d\n",val);
#endif
                                uint16_t **newstates;
                                if(val==7) newStates=moveRook(state,moveWhite,i,&newstates,false);
                                else if(val==8) newStates=moveKnight(state,moveWhite,i,&newstates);
                                else if(val==9) newStates=moveBishop(state,moveWhite,i,&newstates,false);
                                else if(val==10) newStates=moveKing(state,moveWhite,i,&newstates);
                                else if(val==11) newStates=moveQueen(state,moveWhite,i,&newstates);
                                else if(val==12) newStates=movePawn(state,moveWhite,i,&newstates);
                                allowMoves+=newStates;
#ifdef DEBUG
                                if(i==47 && newStates>0)
                                {
//                                        printf("DDDDDDDDDDDDDDDDDDDDd i=%d newStates=%d\n",i,newStates);
                                      for(int i=0;i<newStates;i++)
                                              printState(newstates[i]);
                                }
#endif
				int j;
				for(j =0; j <newStates; j++) 
				{
				HcState *child;
				child->k = parentk+1;
				child->moveWhite = true;
				copy(&newstates[j][0], &newstates[j][16], child->board);
				recursive_hc(child);
				//Free newStates
				delete [] newstates[j];
						
				}

				if(newStates !=0)
				delete [] newstates;
                        }
                }
#ifdef DEBUG
                CkPrintf("Black has %d allowed moves\n",allowMoves);
#endif
        }

	/* Node eaxpansion logic  finish..... ***************************************/
	//							foundSolution();

			/*   
		
		//check for impossibility
	

			//check for forced move
		int forced = 0;
	
		if(forced ==0){

		for(int i = start; i <end; i++){
						if(parentk == vertices)
	

				{
									foundSolution();
											}
				}
				else if(parent->ispresent(dest)==0){
				//		statesExpanded++;
						parent->path[parentk]=dest;
						parent->in_path[dest] = 1;
						parent->k = parentk+1;
						parent->v = dest;
						recursive_hc(parent);
						parent->k = parentk;
						parent->v = v;
						parent->in_path[dest] = 0;

						//reset counts
				}
		}

        }
*/
		
	}

	//Is it used ??
    inline bool isGoal(State *s){

#ifndef ONESOL
			return false;
#endif
		CkPrintf("Goal reached \n");	
      HcState *state = (HcState *)s;
      if(state->k == MAX_DEPTH )  
          return true;
      else
        return false;

    }

/*
    inline bool terminate(State *s){
      NQueenState *state = (NQueenState *)s;
      return (state->currentrow == numQueens-1 ? true:false);
    }
	*/

    void unpack(NodeQueue *q, State *ss, size_t size){
      q->embed(ss, size);
    }

};

Hc::Hc( CkArgMsg* msg )
{
	CkPrintf("--------------------\nstart\n");
		if(msg->argc !=2) 
	{
			CkPrintf("Usage :Program name Input hcp filename \n"); 
			CkExit();
	}
   delete msg;

/*
	//char* filename = "alb0003.hcp";
	//FILE *file = fopen(filename, "r");
	FILE *file = fopen(msg->argv[2], "r");
	
	vertices = atoi((*new string(msg->argv[2])).substr(3, 4).c_str());

	char *str = new char[1000];
	for(int i=0; i<7; i++)
        fgets(str, 1000, file);
        
    int src, dest;
    do{
	   	sscanf(str, "%d %d", &src, &dest);
	   	fgets(str, 1000, file);
	}while(strcmp(str, "-1\n")!=0);
*/
    searchEngineProxy.start();
}

AppCore *newAppCore(){
  return new HcCore;
}

#include "hc.def.h"
