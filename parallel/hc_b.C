#include <vector>
#include "searchEngineAPI.h"

#include "hc.h"

using namespace std;
int vertices;
size_t max_stack_size;
int max_stack_nodes;

vector<int> indices;
vector<int> edges;
#define MAX_BRANCH 25





/*
#ifdef MAXVAL
bool cmp(int v1, int v2){
		return (counts[v1]>counts[v2]);
}
#else
bool cmp(int v1, int v2){
		return (counts[v1]<counts[v2]);
}
#endif
*/


class HcState: public State {
public:
// k is the number of vertices added to the path,  v is the last vertex in the path, path is the current state
	int k;
	int v;
    int * path;
	int * in_path;
	int * counts;
   // int *places;

    HcState(int kk, int vertex, int num_vertices)
    {
		k = kk;
		v = vertex;
        path =(int*) malloc(sizeof(int) * num_vertices);
        in_path =(int*) malloc(sizeof(int) * num_vertices);
        counts =(int*) malloc(sizeof(int) * num_vertices);
    }

    ~HcState()
    {
          if(path != NULL)
            free(path);
		  if(in_path != NULL)
            free(in_path); 
		  if(counts != NULL)
            free(counts);
    }
  /*  int getColum(int r)
    {
        return places[r];
    }
    void setColum(int r, int v)
    {
        places[r] = v;
    }
    */
    void copyParent(HcState* p)
    {
        for(int i=0;i<vertices; i++){
            in_path[i] = p->in_path[i];
			counts[i] = p->counts[i];
		}
	 	for(int i=0;i<k; i++){
            path[i] = p->path[i];
		}


    }
	int ispresent(int node)
	{
//	printf("ispresent called on %d on proc %d \n", node, CkMyPe());
	return in_path[node];
	/*for(int i =0; i <N; i++)
				if(path[i] == node)
						return 1;
		return 0;
		*/
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
         HcState *state = (HcState *)qs->registerState(sizeof(HcState)+ 3* sizeof(int) *vertices,1);
         state->k = 1;
		 state->v = 0;
		 //Add path to state ?
	     state->path = (int*)((char*)state+sizeof(HcState));
         state->in_path = (int*)((char*)state+sizeof(HcState)+sizeof(int)*vertices);
         state->counts = (int*)((char*)state+sizeof(HcState)+2*sizeof(int)*vertices);
		 
		 state->path[0] = 0;
		 for(int i=0; i<vertices; i++){
				 state->in_path[i] = 0;
		 }
		 state->in_path[0] = 1;
		 
		 //Initialize counts
		 //Incorrect now
		 state->counts[0] = indices[0];
		 for(int i=1; i<vertices; i++){
				 state->in_path[i] = 0;
			state->counts[i] = indices[i]- indices[i-1];
		 } 
		 qs->push(state);

    }

    inline void expand(State *state, NodeQueue *qs){
      HcState *parent = ((HcState *)state);
    //  parent->places = (int*)((char*)parent+sizeof(NQueenState));
      parent->path = (int*)((char*)parent+sizeof(HcState));
      parent->in_path = (int*)((char*)parent+sizeof(HcState)+sizeof(int)*vertices);
      parent->counts = (int*)((char*)parent+sizeof(HcState)+2*sizeof(int)*vertices);

      int parentk = parent->k;
      int childnum = 0;

								//CkPrintf("Bef Parallel %d  : \n", parentk);
      if(parentk < sequential_threshold){

		int start =0;
		int dest;
		int v = parent->v;
		if(v-1>=0)
			start = indices[v-1];
		int end = indices[v];
	
								//CkPrintf("Parallel %d  : \n", parentk);
		//check for impossibility
		for(int j =0; j <vertices; j++){
				
				if(parent->ispresent(j)==0)
						if(parent->counts[j] <2){

								//CkPrintf("Impossibility detected, returning \n");
								return;
						}
		}	

		//update counts, except 1st vertex
		if(parentk !=1){
				for(int i = start; i <end; i++){
						dest = edges[i];
						parent->counts[dest]--;
				}
		}
	
		int forced = 0;
		if(parentk != vertices)
				for(int i = start; i <end; i++){
						dest = edges[i];
						if(parent->ispresent(dest)==0){
								if(parent->counts[dest] ==1){

									//CkPrintf("Forced Move detected \n");
								//		statesExpanded++;
									HcState *child = (HcState *)qs->registerState(sizeof(HcState)+3* sizeof(int)*vertices, childnum);
									child->path = (int*)((char*)child+sizeof(HcState));
									child->in_path = (int*)((char*)child+sizeof(HcState)+sizeof(int)*vertices);
									child->counts = (int*)((char*)child+sizeof(HcState)+2*sizeof(int)*vertices);
									child->k = parentk; 
									child->copyParent(parent);
									child->k = parentk+1; 
									child->path[parentk] = dest;
									child->in_path[dest] =1;
									child->v = dest;
				               		qs->push(child);
									childnum++;
									//CkPrintf("Generated node with k = %d \n",child->k); 
										forced =1;
										break;
								}
						}
				}

		if(forced ==0){

		for(int i = start; i <end; i++){
				//check to see if already in the path
				//Spcl case if k == vertices
				//2 node case might need spcl handling
				dest = edges[i];
				if(parentk == vertices)
				{
						if (dest== parent->path[0]){
								//foundSolution();
				//			  	getthetime(t2)
								//we are done, found a path, dsiplay and exit ?
								CkPrintf("HC found : \t");
								for(int j =0; j <vertices; j++)
										CkPrintf("%d \t", parent->path[j]+1);
								CkPrintf("\n");
								foundSolution();
  								//CkPrintf("States Expanded: %ld\nTime taken :%f \n", statesExpanded, t2-t1);
								// do housekeeping
							/*	delete []edges;
								delete []indices;
								delete []in_path;
								delete[]path;*/
								
								//exit(0);
						}
				}

				else if(parent->ispresent(dest)==0){
				//		statesExpanded++;

				HcState *child = (HcState *)qs->registerState(sizeof(HcState)+3* sizeof(int)*vertices, childnum);
                child->path = (int*)((char*)child+sizeof(HcState));
                child->in_path = (int*)((char*)child+sizeof(HcState)+sizeof(int)*vertices);
                child->counts = (int*)((char*)child+sizeof(HcState)+2*sizeof(int)*vertices);
                
				child->k = parentk; 
                child->copyParent(parent);
                child->k = parentk+1; 
				child->path[parentk] = dest;
				child->in_path[dest] =1;
				child->v = dest;
			/*	//update counts, except 1st vertex
				if(parentk !=1){
				for(int i = start; i <end; i++){
						dest = edges[i];
						parent->counts[dest]--;
				}
				}*/

									//CkPrintf("Generated node with k = %d \n",child->k); 
                qs->push(child);
				childnum++;

				}
      }

		}
	  }
      else{
        recursive_hc(parent);
      }
    }

    void recursive_hc(HcState *parent){
    
	int parentk = parent->k;

		int start =0;
		int dest;
		int v = parent->v;
		if(v-1>=0)
			start = indices[v-1];
		int end = indices[v];
		
		//check for impossibility
		for(int j =0; j <vertices; j++){
				
				if(parent->ispresent(j)==0)
						if(parent->counts[j] <2){

								//CkPrintf("Impossibility detected, returning \n");
								return;
						}
		}	

		//update counts, except 1st vertex
		if(parentk !=1){
				for(int i = start; i <end; i++){
						dest = edges[i];
						parent->counts[dest]--;
				}
		}

			//check for forced move
		int forced = 0;
		if(parentk != vertices)
				for(int i = start; i <end; i++){
						dest = edges[i];
						if(parent->ispresent(dest)==0){
								if(parent->counts[dest] ==1){

								//	CkPrintf("Forced Move detected \n");
								//		statesExpanded++;
										parent->path[parentk]=dest;
										parent->in_path[dest] = 1;
										parent->k = parentk+1;
										parent->v = dest;


										forced =1;

										recursive_hc(parent);
										parent->k = parentk;
										parent->v = v;
										parent->in_path[dest] = 0;
										break;
								}
						}
				}

		if(forced ==0){

		for(int i = start; i <end; i++){
				//check to see if already in the path
				//Spcl case if k == vertices
				//2 node case might need spcl handling
				dest = edges[i];
				if(parentk == vertices)
	

				{
						if (dest== parent->path[0]){
								//foundSolution();
				//			  	getthetime(t2)
								//we are done, found a path, dsiplay and exit ?
								CkPrintf("HC found : \t");
								for(int j =0; j <vertices; j++)
										CkPrintf("%d \t", parent->path[j]+1);
								CkPrintf("\n");
								foundSolution();
								//exit(1);
  								//CkPrintf("States Expanded: %ld\nTime taken :%f \n", statesExpanded, t2-t1);
								// do housekeeping
							/*	delete []edges;
								delete []indices;
								delete []in_path;
								delete[]path;*/
								
								//exit(0);
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

			//update counts, except 1st vertex
		if(parentk !=1){
				for(int i = start; i <end; i++){
						dest = edges[i];
						parent->counts[dest]++;
				}
		}

	}

	//Is it used ??
    inline bool isGoal(State *s){
			return false;
      HcState *state = (HcState *)s;
      if(state->k == vertices )  
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
		if(msg->argc !=3) 
	{
			CkPrintf("Usage :Program name Input hcp filename \n"); 
			CkExit();
	}

	//char* filename = "alb0003.hcp";
	//FILE *file = fopen(filename, "r");
	FILE *file = fopen(msg->argv[2], "r");
	
	//vertices = atoi((*new string(filename)).substr(3, 4).c_str());
	vertices = atoi((*new string(msg->argv[2])).substr(3, 4).c_str());

     delete msg;
	CkPrintf("%d \n", vertices);
	int * edge_counts = new int[vertices];
	int * indices_a = new int[vertices];
			
	for(int i=0; i<vertices; i++){
			edge_counts[i]=0;
	}
	char *str = new char[1000];
	for(int i=0; i<7; i++)
        fgets(str, 1000, file);
        
//	CkPrintf("hello \n");
    int src, dest;
    do{
	   	sscanf(str, "%d %d", &src, &dest);
	   	edge_counts[src-1]++;
	   	edge_counts[dest-1]++;
	   	fgets(str, 1000, file);
	}while(strcmp(str, "-1\n")!=0);

	// We have the edge counts for each vertex now
	// Calculate indices and allocate edge array
	// edge_counts[i] is starting index of vertex i+1
	// vertices is 1000 say
	indices_a[0] = 0;
	for(int i=1; i<vertices; i++){
//			CkPrintf("Bef %d \t \n", edge_counts[i]);
			edge_counts[i]+=edge_counts[i-1];
//			CkPrintf("%d \t \n", edge_counts[i]);
			indices_a[i] = edge_counts[i-1];
	}

	
	int total_edges = edge_counts[vertices-1];
	int * edges_a = new int[total_edges];

	//set to beginning of file again
	fseek(file, 0, SEEK_SET);
	for(int i=0; i<7; i++)
        fgets(str, 1000, file);
        
	// say edge array is 2 3 4
	int start, end;
/*  	for(int i=vertices; i>0; i--){
			//start =0;
			//if(i-2>=0)
			start =indices_a[i-1];
			end =edge_counts[i-1];
			for(int k =start; k<end; k++){
					sscanf(str, "%d %d", &src, &dest);
					edges_a[k]= dest-1;
					edges_a[indices_a[dest-1]++] = src-1;
				//	CkPrintf("%d %d \t", k, edges_a[k]);
				//	CkPrintf("%d %d \n", indices_a[dest-1]-1, edges_a[indices_a[dest-1]-1]);
	   				fgets(str, 1000, file);
			}
	}
*/
	//General format
	int unique_edges = total_edges/2;
	for(int i=0; i<unique_edges; i++){
			//start =0;
			//if(i-2>=0)
					sscanf(str, "%d %d", &src, &dest);
					edges_a[indices_a[src-1]++]= dest-1;
					edges_a[indices_a[dest-1]++] = src-1;
				//	CkPrintf("%d %d \t", k, edges[k]);
				//	CkPrintf("%d %d \n", indices_a[dest-1]-1, edges_a[indices_a[dest-1]-1]);
	   				fgets(str, 1000, file);
	}

	delete []indices_a;
	indices_a = edge_counts;

	indices.assign(indices_a, indices_a+vertices);
	edges.assign(edges_a, edges_a+total_edges);

/*
	//Done reading input file , checking to see if read correctly
	for(int i=0; i<vertices; i++){
			start =0;
			if(i-1>=0)
				start	=edge_counts[i-1];
			end =edge_counts[i];
			for(int k =start; k<end; k++){
					CkPrintf("%d %d \n", i+1, edges_a[k]+1);
	}
	}
*/	


	//Calculate HC


    searchEngineProxy.start();
}

AppCore *newAppCore(){
  return new HcCore;
}

#include "hc.def.h"
