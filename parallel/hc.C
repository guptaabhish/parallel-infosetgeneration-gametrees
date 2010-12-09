#include <vector>
#include "searchEngineAPI.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <set>
#include <string.h> // memcpy

#include "hc.h"
//#include "gameLogic.h"

using namespace std;
int vertices;
size_t max_stack_size;
int max_stack_nodes;

uint16_t trueState1[16];


bool whitePerspective;

//vector< vector<uint16_t> > trueState;

uint16_t moveHistory[100];
int maxdepth;
vector< set<uint16_t> > failedMoves(100);

long long rankFileDests[64]; // Rook-like moves; used for rook, king, queen
long long diagonalDests[64]; // used for bishop, king, and queen
long long knightDests[64]; // used only for knights
//	globalState
vector<uint16_t> globalState;

#define MAX_BRANCH 25
#define MAX_DEPTH 8
#define MAX_CHILDREN 64
//#define DEBUG
//#define ONESOL


int nSolutions=0;
//#define PRINT_SOLUTIONS 

// Use sets of moves to keep track of the list of all illegal moves that were tried before
// a legal move was made.  Sets make sense here because it doesn't matter what order the moves
// were tried in.
typedef set< uint16_t > SetMove;
typedef vector< SetMove > VecSetMove;

const long long ONE = 1;
#define IsFree64(a,ind)  !( (a[((ind) / 64)]) & (ONE<<((ind) % 64)) )
#define Set64(a,ind) a[((ind)/64)] = ( a[((ind)/64)] | (ONE<<((ind) % 64)) )
#define IsFree(a,ind)  !( (a[((ind) / 32)]) & (1<<((ind) % 32)) )
#define Set(a,ind) a[((ind)/32)] = ( a[((ind)/32)] | (1<<((ind) % 32)) )
#define Reset(a,ind) a[((ind)/32)] = ( a[((ind)/32)] & (~(1<<((ind) % 32))) )
#define IsFree16(a,ind)  !( (a[((ind) / 16)]) & (1<<((ind) % 16)) )

// Number of slots allocated to generate all legal moves from a given position
// I have no idea whether this is an appropriate number or not.
const int NMOVES = 1000; 

// Maximum number of legal moves in a sequence of moves from the start state.
// This can be changed and will affect the amount of memory allocated up front and the spatial locality
// of the program

///////////// This should be 100.. it is hardcoded in ci file
const int MAXDEPTH = 100;

// Number of uint16_t data in a state.  This cannot be changed without redoing everything.
const int STATESIZE = 16;

// Specify the locations of th bit flags in the structures that encode a move
// A move is a 16-bit entity.  Bits 0-5 are the destination of the move (square 0-63); bits 6-11 are the src
// Bit 12 is reserved to flag a capture (not implemented yet)
// Bit 13 is set for pawn captures
// Bit 14 is set if the move puts or leaves the active player in check 
// Bit 15 is set if the move is attemptable but blocked
const int BLOCKED = ONE << 15;
const int CHECKED = ONE << 14;
const int PAWNTRY = ONE << 13;

// Currently, the start state is copied here and it is referenced during infoset generation to translate the
// sequence of moves into a sequence of states (by copying this state to another location and then repeatedly
// applying moves to it)
//uint16_t globalState[16];

// Foreward declarations.  Some of these could be eliminated by reordering the functions definitions.
uint16_t getBlockVal(uint16_t*,int);
void removePiece(uint16_t *state,uint16_t fromblock);
void addPiece(uint16_t *state,uint16_t toblock,uint16_t pieceVal);
void movePiece(uint16_t *state,uint16_t fromblock, uint16_t toblock);
int moveRook(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates,bool);
int moveBishop(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates,bool);
void generateAttemptableMoves(uint16_t* state, bool whiteMove, uint16_t* moves, int& nMoves, bool verboseo = false);
bool isLegal(const uint16_t& move);

// MDR
// Return the bits pointed to by x as a string of 1s and 0s, with the 0th bit on the right
string disp(int* x, int max)
{
  ostringstream os;
  for (int i = max - 1; i >= 0; i--) {
    os << ((IsFree(x,i)) ? "0" : "1") ; 
  }
  return os.str();
}

// Same as above method, but with a uint16_t pointer.  This is entirely superfluous.  I'm not sure why it's here.
string disp16(uint16_t* x, int max)
{
  ostringstream os;
  for (int i = max - 1; i >= 0; i--) {
    os << ((IsFree16(x,i)) ? "0" : "1") ; 
  }
  return os.str();
}

// Use the src and destination squares together with the appropriate flags to encode an attemptable move
// Bits 0-5 are destination square, 6-11 are souurce square, 13 denotes pawn capture, 14 means this move
// would leave the player who made it in check, 15 means the move is attemptable but blocked
uint16_t encodeMove(uint16_t from, uint16_t to, bool blocked, bool checked, bool pawntry = false)
{
  uint16_t result = 0;
  result = (blocked << 15) | (checked << 14) | (pawntry << 13) | (from << 6) | to ;
  return result;
}

// Return the destination square of an encoded Move
uint16_t extractDestination(const uint16_t move)
{
  return move & 63; //0x003F
}

// Extract the bit fields from a move into isolated structures.  Can be updated to include pawntry flag
void decodeMove(uint16_t move, uint16_t& from, uint16_t& to, bool& blocked, bool& checked)
{
  blocked = (1 << 15) & move; 
  checked = (1 << 14) & move; 
  from = (4032 & move) >> 6; 
  to = 63 & move; 
}

// Return a string representing the move as a src/dest pair, i.e., (13,21) for a move in which a piece
// moves from square 13 to square 21 (0 is in the upper-left corner, home of a black rook).
string decodeMove(uint16_t move)
{
  uint16_t from ;
  uint16_t to;
  bool checked;
  bool blocked;
  decodeMove (move, from, to, blocked, checked);
  ostringstream os;
  os << "(" << from << "," << to << ")";
  return os.str();
}

// Return the rank (1-8) of the specified block
int rankOf(int block)
{
  return 8 - block / 8;
}

// Give a string encoding of a move.  The state is also passed in so that the type of piece can be encoded.
// The encoding is piece-letter source-file source-rank : dest-file dest-rank
string decodeMove(uint16_t* state, uint16_t move)
{
  static char files[9] = "abcdefgh";
  static char pieces[14] = " RNBKQPRNBKQP";
  uint16_t from ;
  uint16_t to;
  bool checked;
  bool blocked;
  decodeMove (move, from, to, blocked, checked);
  ostringstream os;
  int piece = getBlockVal(state,from);
  assert (piece > 0);
  assert (piece < 13);
  os << pieces[piece] << files[from%8] << rankOf(from) << ":" << files[to%8] << rankOf(to);
  return os.str();
}

// Simple move display -- Not important
void dispMove(uint16_t from, uint16_t to, bool blocked, bool checked)
{
//  cout << "from: " << from << " to: " << to << " blocked: " << blocked << " checked: " << checked << endl;
}

void dispMove(uint16_t move)
{
  uint16_t from ;
  uint16_t to;
  bool checked;
  bool blocked;
  decodeMove (move, from, to, blocked, checked);
  dispMove(from,to,blocked,checked);

}


uint16_t getCode(char piece)
{
  switch (piece) {
    case '-' : return 0;
    case 'r' : return 1;
    case 'n' : return 2;
    case 'b' : return 3;
    case 'k' : return 4;
    case 'q' : return 5;
    case 'p' : return 6;
    case 'R' : return 7;
    case 'N' : return 8;
    case 'B' : return 9;
    case 'K' : return 10;
    case 'Q' : return 11;
    case 'P' : return 12;
    default: assert(false);
  }
}

// Initialize a state structure for a string of 64 characters
void fillBoard(uint16_t* state, const string& sState)
{
  assert (sState.size() == 64);
  for (unsigned i = 0; i < sState.size(); i++) {
    addPiece(state,i,getCode(sState[i]));    
  } 
}

// Use this function and the fillBoard function to build interesting test cases.  Draw a board configuration
// and give it a case number. The call this function with the appropriate test number and pass the result to fillBoard
string sampleState(int test)
{
  ostringstream os;
  switch (test) {
	case 0:
    os <<  "RNBQKBNR"
	<< "PPPPPPPP"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "pppppppp"
	<< "rnbqkbnr";
	break;
	case 1:
    os <<  "RK----rk"
	<< "PP----qp"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------";
	break;
	case 2:
    os <<  "--------"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------"
	<< "--------";
	break;
	case 3:
    os <<  "nN-N----"
	<< "--p-----"
	<< "-k---b--"
	<< "-----Rq-"
	<< "--K--B--"
	<< "--------"
	<< "--------"
	<< "--------";
	break;
	case 4:
    os <<  "R-BQ-BN-"
	<< "PP-KPPP-"
	<< "N--P---R"
	<< "--P----P"
	<< "p----pp-"
	<< "--p-----"
	<< "rpq-p--p"
	<< "-nb-kbnr";
	break;
	case 5:
    os <<  "KB--qqqq"
	<< "P-------"
	<< "--------"
	<< "--------"
	<< "q-------"
	<< "q-------"
	<< "q-------"
	<< "q------k";
	break;
    default: assert(false);
  }
  return os.str();

}

void copyState(uint16_t* oldState, uint16_t* newState)
{
  memcpy(newState,oldState,sizeof(uint16_t)*16);
}

// Don't copy over original state
void applyMove(uint16_t* oldState, uint16_t* newState, uint16_t move)
{
  uint16_t from ;
  uint16_t to;
  bool checked;
  bool blocked;
  assert(move!=0);
  decodeMove (move, from, to, blocked, checked);
  assert(from!=to);
  assert (!blocked);
  assert (!checked);
  memcpy(newState,oldState,sizeof(uint16_t)*16);
  movePiece(newState,from,to);
}

// Apply move "in place."  Previous state is overwritten
void applyMove(uint16_t* state, uint16_t move)
{
  uint16_t from;
  uint16_t to;
  bool checked;
  bool blocked;
  assert(move!=0);
  decodeMove (move, from, to, blocked, checked);
  assert(from!=to);
  assert (!blocked);
  assert (!checked);
  movePiece(state,from,to);
}

void printPiece(uint16_t val)
{
        if(val==0) printf("-");
        else if(val==1) printf("r");
        else if(val==2) printf("n");
        else if(val==3) printf("b");
        else if(val==4) printf("k");
        else if(val==5) printf("q");
        else if(val==6) printf("p");
        else if(val==7) printf("R");
        else if(val==8) printf("N");
        else if(val==9) printf("B");
        else if(val==10) printf("K");
        else if(val==11) printf("Q");
        else if(val==12) printf("P");
	else exit(0);
	printf(" ");

}

void printState(uint16_t *arr)
{
	printf("\n**********************************************\n");
	for(int i=0;i<64;i++)
	{
		printPiece(getBlockVal(arr,i));
		if(i==63) break;
		if(i%8==7) 
			printf("\n");
		
	}
	printf("\n**********************************************\n");
}

// Return the value of the piece at the specified block (0-63).  Zero means empty
uint16_t getBlockVal(uint16_t *arr,int block)
{
	uint16_t val=arr[block/4];
	int offset=4*(block%4);
//	printf("11 i=%d offset=%d val=%d\n",block,offset,val);
	if(offset<12) 
	val=val>>(12-offset);
//	printf("22 val=%d\n",val);
	if(offset>0) 
	{
		val=val<<12;
		val=val>>12;
	}
//printf("33 val=%d\n",val);
	return val;
}

int moveQueen(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates)
{
	int totalStates=0,statesB,statesR;
	uint16_t **newStatesB,**newStatesR;
	printf("))))))))))))))))))))))))))))))))))0\n");
	//diagonal moves
	statesB=moveBishop(state,moveWhite,fromblock,&newStatesB,true);
	//straight moves
	statesR=moveRook(state,moveWhite,fromblock,&newStatesR,true);
	totalStates=statesB+statesR;

	(*newStates)=new uint16_t*[totalStates];

	for(int i=0;i<statesB;i++) (*newStates)[i]=newStatesB[i];
	for(int i=0;i<statesR;i++) (*newStates)[i+statesB]=newStatesR[i];
	printf("\n\nQueen at %d block can move %d moves\n",fromblock,totalStates);
	return totalStates;

}

void kingHelper(uint16_t *state,uint16_t fromblock,uint16_t *moves,int toblock,bool moveWhite)
{
	int start,end;
        if(moveWhite)
        {
                start=7;end=12;
        }
        else
        {
                start=1;end=6;
        }

                int val=getBlockVal(state,toblock);
                if(val==0)
		{
			removePiece(moves,fromblock);
	                if(moveWhite)
                	        addPiece(moves,toblock,4);
        	        else
	                        addPiece(moves,toblock,10);

		}
                else if(val>=start && val<=end)
                {
                        removePiece(moves,toblock);
	                if(moveWhite)
                	        addPiece(moves,toblock,4);
        	        else
	                        addPiece(moves,toblock,10);

                }
}

int moveKing(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates)
{
        int totalStates=0;
	uint16_t *moves[8];
        int start,end;
        if(moveWhite)
        {
                start=7;end=12;
        }
        else
        {
                start=1;end=6;
        }

	// move straight
	int toblock=fromblock-8;
	int val=getBlockVal(state,toblock);
	if(fromblock/8>0 && (val==0 || (val>=start && val<=end)))
	{
		moves[totalStates]=new uint16_t[16];
		for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
		kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
		totalStates++;
	}
	// move back
        toblock=fromblock+8;
        val=getBlockVal(state,toblock);
	if(fromblock<7 && (val==0 || (val>=start && val<=end)))
	{
                moves[totalStates]=new uint16_t[16];
                for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
                kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
                totalStates++;
	}
	// move left
        toblock=fromblock-1;
        val=getBlockVal(state,toblock);
	if(fromblock%8>0 && (val==0 || (val>=start && val<=end)))
	{
                moves[totalStates]=new uint16_t[16];
                for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
                kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
                totalStates++;
	}
	// move right
        toblock=fromblock+1;
        val=getBlockVal(state,toblock);
	if(fromblock%8<7 && (val==0 || (val>=start && val<=end)))
	{
                moves[totalStates]=new uint16_t[16];
                for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
                kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
                totalStates++;
	}
	// move north-east
        toblock=fromblock-7;
        val=getBlockVal(state,toblock);
	if(fromblock/8>0 && fromblock%8<7 && (val==0 || (val>=start && val<=end)))
	{
                moves[totalStates]=new uint16_t[16];
                for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
                kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
                totalStates++;
	}
	// move north-west
        toblock=fromblock-9;
        val=getBlockVal(state,toblock);
        if(fromblock/8>0 && fromblock%8>0  && (val==0 || (val>=start && val<=end)))
        {
                moves[totalStates]=new uint16_t[16];
                for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
                kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
                totalStates++;
        }
	// move south-east
        toblock=fromblock+9;
        val=getBlockVal(state,toblock);
        if(fromblock/8<7 && fromblock%8<7 && (val==0 || (val>=start && val<=end)))
        {
                moves[totalStates]=new uint16_t[16];
                for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
                kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
                totalStates++;
        }
	// move south-west
        toblock=fromblock+7;
        val=getBlockVal(state,toblock);
        if(fromblock/8<7 && fromblock%8>0 && (val==0 || (val>=start && val<=end)))
        {
                moves[totalStates]=new uint16_t[16];
                for(int i=0;i<16;i++) moves[totalStates][i]=state[i];
                kingHelper(state,fromblock,moves[totalStates],toblock,moveWhite);
                totalStates++;
        }

	(*newStates)=new uint16_t*[totalStates];
	for(int i=0;i<totalStates;i++) (*newStates)[i]=moves[i];
	printf("King at %d block can move %d moves\n",fromblock,totalStates);
        return totalStates;
}

void knightHelper(uint16_t *state,uint16_t *first,int fromblock,int toblock,bool moveWhite)
{
	uint16_t start,end=-1;
        if(moveWhite)
        {
                start=7;end=12;
        }
        else
        {
                start=1;end=6;
        }

		int val=getBlockVal(state,toblock);
                for(int i=0;i<16;i++) first[i]=state[i];
                if(val==0)
                {
                        // If the block was empty
                        removePiece(first,fromblock);
                        if(moveWhite) addPiece(first,toblock,2);
			else addPiece(first,toblock,8);
                }
                else if(val<=end && val>=start)
                {
                        //take it
                        removePiece(first,fromblock);
                        removePiece(first,toblock);

                        if(moveWhite)
                                addPiece(first,toblock,2);
                        else
                                addPiece(first,toblock,8);
                                                
                }

}

int moveKnight(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates)
{
        int totalStates=0;
//printf("----------- coming here for i=%d\n",fromblock);
	uint16_t *first,*second,*third,*four,*five,*six,*seven,*eight;
	first=second=third=four=five=six=seven=eight=NULL;
        uint16_t start,end=-1;
        if(moveWhite)
        {
                start=7;end=12;
        }
        else
        {
                start=1;end=6;
        }

	// 2 Forward 1 left
	int val=getBlockVal(state,fromblock-16-1);
	if(fromblock/8>1 && (fromblock%8)>0 && (val==0 || (val>=start && val<=end)))
	{
		first=new uint16_t[16];
		knightHelper(state,first,fromblock,fromblock-16-1,moveWhite);
		totalStates++;
	}
	// 1 Forward 2 left	
	val=getBlockVal(state,fromblock-8-2);
	if(fromblock/8>0 && fromblock%8>1  && (val==0 || (val>=start && val<=end)))
	{
                second=new uint16_t[16];
		knightHelper(state,second,fromblock,fromblock-8-2,moveWhite);
                totalStates++;
	}
	// 2 Forward 1 right
	val=getBlockVal(state,fromblock-16+1);
	if(fromblock/8>1 && (fromblock%8)<7  && (val==0 || (val>=start && val<=end)))
	{
                third=new uint16_t[16];
                knightHelper(state,third,fromblock,fromblock-16+1,moveWhite);
                totalStates++;
	}
	// 1 forward 2 right 
	val=getBlockVal(state,fromblock-8+2);
	if(fromblock/8>0 && fromblock%8<6  && (val==0 || (val>=start && val<=end)))
	{
                four=new uint16_t[16];
                knightHelper(state,four,fromblock,fromblock-8+2,moveWhite);
                totalStates++;
	}
	// 2 Back 1 left
	val=getBlockVal(state,fromblock+16-1);
	if(fromblock/8<6 && fromblock%8>0  && (val==0 || (val>=start && val<=end)))
        {
                five=new uint16_t[16];
                knightHelper(state,five,fromblock,fromblock+16-1,moveWhite);
                totalStates++;
        }

	// 1 Back 2 left
	val=getBlockVal(state,fromblock+8-2);
	if(fromblock/8<7 && fromblock%8>1  && (val==0 || (val>=start && val<=end)))
        {
                six=new uint16_t[16];
                knightHelper(state,six,fromblock,fromblock+8-2,moveWhite);
                totalStates++;
        }

	// 2 Back 1 right
	val=getBlockVal(state,fromblock+16+1);
	if(fromblock/8<6 && fromblock%8<7  && (val==0 || (val>=start && val<=end)))
        {
                seven=new uint16_t[16];
                knightHelper(state,seven,fromblock,fromblock+16+1,moveWhite);
                totalStates++;
        }

	// 1 Back 2 right
	val=getBlockVal(state,fromblock+8+2);
	if(fromblock/8<7 && fromblock%8<6  && (val==0 || (val>=start && val<=end)))
        {
                eight=new uint16_t[16];
                knightHelper(state,eight,fromblock,fromblock+8+2,moveWhite);
                totalStates++;
        }
	int cc=0;
        (*newStates)=new uint16_t*[totalStates];

	if(first!=NULL) (*newStates)[cc++]=first;
        if(second!=NULL) (*newStates)[cc++]=second;
        if(third!=NULL) (*newStates)[cc++]=third;
        if(four!=NULL) (*newStates)[cc++]=four;
        if(five!=NULL) (*newStates)[cc++]=five;
        if(six!=NULL) (*newStates)[cc++]=six;
        if(seven!=NULL) (*newStates)[cc++]=seven;
        if(eight!=NULL) (*newStates)[cc++]=eight;
	
	printf("\n\nKnight at %d block can move %d moves\n",fromblock,totalStates);
        return totalStates;
}

int moveRook(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates,bool moveQueen)
{
        int totalStates=0,sMoves=0,bMoves=0,lMoves=0,rMoves=0;
	uint16_t **sStates,**bStates,**rStates,**lStates;
	sStates=new uint16_t*[8];
	bStates=new uint16_t*[8];
	rStates=new uint16_t*[8];
	lStates=new uint16_t*[8];

	int start,end;
        if(moveWhite)
        {
                start=7;end=12;
        }
        else
        {
                start=1;end=6;
        }

	//straight Moves
	uint16_t toblock=fromblock-8;
	while(toblock<64 && toblock>=0)
	{
		uint16_t val=getBlockVal(state,toblock);
		if(val==0)
		{
			sStates[sMoves]=new uint16_t[16];
			for(int i=0;i<16;i++) sStates[sMoves][i]=state[i];
	                removePiece(sStates[sMoves],fromblock);
        	        if(moveWhite)
			{
				if(moveQueen) addPiece(sStates[sMoves],toblock,5);
                	        else addPiece(sStates[sMoves],toblock,1);
			}
	                else 
			{
				if(moveQueen) addPiece(sStates[sMoves],toblock,11);
				else addPiece(sStates[sMoves],toblock,7);		
			}
//			printState(sStates[sMoves]);
			sMoves++;
		}
		else if(val<=end && val>=start)
		{
			sStates[sMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) sStates[sMoves][i]=state[i];
                        removePiece(sStates[sMoves],fromblock);
			removePiece(sStates[sMoves],toblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(sStates[sMoves],toblock,5);
                                else addPiece(sStates[sMoves],toblock,1);
			}
                        else 
			{
				if(moveQueen) addPiece(sStates[sMoves],toblock,11);
				else addPiece(sStates[sMoves],toblock,7);  
			}
//			printState(sStates[sMoves]);
			sMoves++;
			break;
		}
		else break;//the path is blocked
//		printState(sStates[sMoves]-1);
		toblock-=8;
	}
	//backward Moves
	toblock=fromblock+8;
        while(toblock<64 && toblock>=0)
        {
                uint16_t val=getBlockVal(state,toblock);
                if(val==0)
		{
			bStates[bMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) bStates[bMoves][i]=state[i];
                        removePiece(bStates[bMoves],fromblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(bStates[bMoves],toblock,5);
                                else addPiece(bStates[bMoves],toblock,1);
			}
                        else 
			{
				if(moveQueen) addPiece(bStates[bMoves],toblock,11);
				else addPiece(bStates[bMoves],toblock,7);
			}
                        bMoves++;
		}
                else if(val<=end && val>=start)
                {
			bStates[bMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) bStates[bMoves][i]=state[i];
                        removePiece(bStates[bMoves],fromblock);
			removePiece(bStates[bMoves],toblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(bStates[bMoves],toblock,5);
                                else addPiece(bStates[bMoves],toblock,1);
			}
                        else 
			{
				if(moveQueen) addPiece(bStates[bMoves],toblock,11);
				else addPiece(bStates[bMoves],toblock,7);
			}

                        bMoves++;
                        break;
                }
		else break;//the path is blocked
                toblock+=8;
        }
	//right Moves
	toblock=fromblock+1;
        while(toblock%8>0 && toblock%8<8 && toblock<64 && toblock>=0 && toblock%8!=7)
        {
                uint16_t val=getBlockVal(state,toblock);
                if(val==0)
		{
			rStates[rMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) rStates[rMoves][i]=state[i];
                        removePiece(rStates[rMoves],fromblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(rStates[rMoves],toblock,5);
                                else addPiece(rStates[rMoves],toblock,1);
			}
                        else 
			{ 
				if(moveQueen) addPiece(rStates[rMoves],toblock,11);
				else addPiece(rStates[rMoves],toblock,7);
			}

                        rMoves++;
		}
                else if(val<=end && val>=start)
                {
			rStates[rMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) rStates[rMoves][i]=state[i];
                        removePiece(rStates[rMoves],fromblock);
			removePiece(rStates[rMoves],toblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(rStates[rMoves],toblock,5);
                                else addPiece(rStates[rMoves],toblock,1);
			}
                        else 
			{
				if(moveQueen) addPiece(rStates[rMoves],toblock,11);
				else addPiece(rStates[rMoves],toblock,7);
			}

                        rMoves++;
                        break;
                }
		else break;//the path is blocked
                toblock+=1;
        }
	//left Moves
	toblock=fromblock-1;
        while(toblock%8>=0 && toblock%8<7 &&  toblock<64 && toblock>=0 && toblock%8!=0)
        {
                uint16_t val=getBlockVal(state,toblock);
                if(val==0)
		{
			lStates[lMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) lStates[lMoves][i]=state[i];
                        removePiece(lStates[lMoves],fromblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(lStates[lMoves],toblock,5);
                                else addPiece(lStates[lMoves],toblock,1);
			}
                        else 
			{
				if(moveQueen) addPiece(lStates[lMoves],toblock,11);
				else addPiece(lStates[lMoves],toblock,7);
			}

                        lMoves++;
		}
                else if(val<=end && val>=start)
                {
			lStates[lMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) lStates[lMoves][i]=state[i];
                        removePiece(lStates[lMoves],fromblock);
			removePiece(lStates[lMoves],toblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(lStates[lMoves],toblock,5);
                                else addPiece(lStates[lMoves],toblock,1);
			}
                        else 
			{
				if(moveQueen) addPiece(lStates[lMoves],toblock,11);
				else addPiece(lStates[lMoves],toblock,7);
			}

                        lMoves++;
                        break;
                }
		else break;//the path is blocked
                toblock-=1;
        }
	totalStates=sMoves+bMoves+rMoves+lMoves;
        (*newStates)=new uint16_t*[totalStates];
	for(int i=0;i<sMoves;i++) (*newStates)[i]=sStates[i];
        for(int i=0;i<bMoves;i++) (*newStates)[i+sMoves]=bStates[i];
        for(int i=0;i<rMoves;i++) (*newStates)[i+sMoves+bMoves]=rStates[i];
        for(int i=0;i<lMoves;i++) (*newStates)[i+sMoves+bMoves+rMoves]=lStates[i];
	printf("Rook at %d block can move %d moves\n",fromblock,totalStates);

        return totalStates;
}

int moveBishop(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates,bool moveQueen)
{
        int totalStates=0,neMoves=0,nwMoves=0,seMoves=0,swMoves=0;
        uint16_t **neStates,**nwStates,**seStates,**swStates;
        neStates=new uint16_t*[8];
        nwStates=new uint16_t*[8];
        seStates=new uint16_t*[8];
        swStates=new uint16_t*[8];

        int start,end;
        if(moveWhite)
        {
                start=7;end=12;
        }
        else
        {
                start=1;end=6;
        }

	// noth-east moves
        uint16_t toblock=fromblock-7;
        while(toblock<64 && toblock>=0)
        {
                uint16_t val=getBlockVal(state,toblock);
                if(val==0)
                {
                        neStates[neMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) neStates[neMoves][i]=state[i];
                        removePiece(neStates[neMoves],fromblock);
                        if(moveWhite)
			{
                                if(moveQueen) addPiece(neStates[neMoves],toblock,5);
				else addPiece(neStates[neMoves],toblock,3);
			}
                        else 
			{
				if(moveQueen) addPiece(neStates[neMoves],toblock,11);
				else addPiece(neStates[neMoves],toblock,9);
			}
	
                        neMoves++;
                }
                else if(val<=end && val>=start)
                {
                        neStates[neMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) neStates[neMoves][i]=state[i];
                        removePiece(neStates[neMoves],fromblock);
                        removePiece(neStates[neMoves],toblock);
                        if(moveWhite)
			{
                                if(moveQueen) addPiece(neStates[neMoves],toblock,5);
				else addPiece(neStates[neMoves],toblock,3);
			}
                        else 
			{
				if(moveQueen) addPiece(neStates[neMoves],toblock,11);
				else addPiece(neStates[neMoves],toblock,9);
			}
                        neMoves++;
                        break;
                }
                else break;//the path is blocked
		if(toblock%8==7 || toblock/8==0) break;
                toblock-=7;
        }
	// north-west moves
        toblock=fromblock-9;
        while(toblock<64 && toblock>=0)
        {
                uint16_t val=getBlockVal(state,toblock);
                if(val==0)
                {
                        nwStates[nwMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) nwStates[nwMoves][i]=state[i];
                        removePiece(nwStates[nwMoves],fromblock);
                        if(moveWhite)
                        {
                                if(moveQueen) addPiece(nwStates[nwMoves],toblock,5);
                                else addPiece(nwStates[nwMoves],toblock,3);
                        }
                        else 
                        {
                                if(moveQueen) addPiece(nwStates[nwMoves],toblock,11);
                                else addPiece(nwStates[nwMoves],toblock,9);
                        }

                        nwMoves++;
                }
                else if(val<=end && val>=start)
                {
                        nwStates[nwMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) nwStates[nwMoves][i]=state[i];
                        removePiece(nwStates[nwMoves],fromblock);
                        removePiece(nwStates[nwMoves],toblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(nwStates[nwMoves],toblock,5);
                                else addPiece(nwStates[nwMoves],toblock,3);
			}
                        else 
			{
				if(moveQueen) addPiece(nwStates[nwMoves],toblock,11);
				else addPiece(nwStates[nwMoves],toblock,9);
			}

                        nwMoves++;
                        break;
                }
                else break;//the path is blocked
		if(toblock%8==0 || toblock/8==0) break;
                toblock-=9;
        }
	// south-east moves
        toblock=fromblock+9;
        while(toblock<64 && toblock>=0)
        {
                uint16_t val=getBlockVal(state,toblock);
                if(val==0)
                {
                        seStates[seMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) seStates[seMoves][i]=state[i];
                        removePiece(seStates[seMoves],fromblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(seStates[seMoves],toblock,5);
                                else addPiece(seStates[seMoves],toblock,3);
			}
                        else 
			{
				if(moveQueen) addPiece(seStates[seMoves],toblock,11);
				else addPiece(seStates[seMoves],toblock,9);
			}

                        seMoves++;
                }
                else if(val<=end && val>=start)
                {
                        seStates[seMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) seStates[seMoves][i]=state[i];
                        removePiece(seStates[seMoves],fromblock);
                        removePiece(seStates[seMoves],toblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(seStates[seMoves],toblock,5);
                                else addPiece(seStates[seMoves],toblock,3);
			}
                        else 
			{
				if(moveQueen) addPiece(seStates[seMoves],toblock,11);	
				else addPiece(seStates[seMoves],toblock,9);
			}

                        seMoves++;
                        break;
                }
                else break;//the path is blocked
		if(toblock%8==7 || toblock/8==7) break;
                toblock+=9;
        }
	// south-west moves
        toblock=fromblock+7;
        while(toblock<64 && toblock>=0)
        {
                uint16_t val=getBlockVal(state,toblock);
                if(val==0)
                {
                        swStates[swMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) swStates[swMoves][i]=state[i];
                        removePiece(swStates[swMoves],fromblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(swStates[swMoves],toblock,5);
                                else addPiece(swStates[swMoves],toblock,3);
			}
                        else 
			{
				if(moveQueen) addPiece(swStates[swMoves],toblock,11);
				else addPiece(swStates[swMoves],toblock,9);
			}

                        swMoves++;
                }
                else if(val<=end && val>=start)
                {
                        swStates[swMoves]=new uint16_t[16];
                        for(int i=0;i<16;i++) swStates[swMoves][i]=state[i];
                        removePiece(swStates[swMoves],fromblock);
                        removePiece(swStates[swMoves],toblock);
                        if(moveWhite)
			{
				if(moveQueen) addPiece(swStates[swMoves],toblock,5);
                                else addPiece(swStates[swMoves],toblock,3);
			}
                        else 
			{
				if(moveQueen) addPiece(swStates[swMoves],toblock,11);
				else addPiece(swStates[swMoves],toblock,9);
			}

                        swMoves++;
                        break;
                }
                else break;//the path is blocked
		if(toblock%8==0 || toblock/8==7) break;
                toblock+=7;
        }

        totalStates=neMoves+nwMoves+seMoves+swMoves;
        (*newStates)=new uint16_t*[totalStates];
        for(int i=0;i<neMoves;i++) (*newStates)[i]=neStates[i];
        for(int i=0;i<nwMoves;i++) (*newStates)[i+neMoves]=nwStates[i];
        for(int i=0;i<seMoves;i++) (*newStates)[i+neMoves+nwMoves]=seStates[i];
        for(int i=0;i<swMoves;i++) (*newStates)[i+neMoves+nwMoves+seMoves]=swStates[i];
        printf("Bishop at %d block can move %d moves\n",fromblock,totalStates);

        return totalStates;
}

int movePawn(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates)
{
	bool step1Fwd;
	int delta;
	bool step2Fwd,rightKill,leftKill;
	step1Fwd=step2Fwd=rightKill=leftKill=false;
	if(moveWhite) delta=-8;
	else delta=8;
	// One step forward
	if(fromblock+delta>0)
	{
		if(getBlockVal(state,fromblock+delta)==0) 
		{
			// This means it is empty and hence you can move the pawn one step forward
			step1Fwd=true;
		}
	}
	// Two steps forward
	if(fromblock+delta>0)
	{
		if(getBlockVal(state,fromblock+2*delta)==0)
        	{
                	// This means it is empty and hence you can move the pawn two steps forward
			step2Fwd=true;
	        }
	}
	// Kill right diag
	if(fromblock%8!=7 && fromblock+delta+1>0)
	{
		if(moveWhite)
		{
			int val=getBlockVal(state,fromblock+delta+1);
			if(val>6 && val<13)
        	        {
				// This means there is a piece which you can kill
				rightKill=true;
	                }
		}
		else
		{
			int val=getBlockVal(state,fromblock+delta+1);
                        if(val>0 && val<7)
                        {
                                // This means there is a piece which you can kill
				rightKill=true;
                        }
		}
	}
	// Kill Left diag	
        if(fromblock%8!=0 && fromblock+delta-1>0)
        {
                if(moveWhite)
                {
                        int val=getBlockVal(state,fromblock+delta-1);
                        if(val>6 && val<13)
                        {
                                // This means there is a piece which you can kill
				leftKill=true;
                        }
                }
                else
                {
                        int val=getBlockVal(state,fromblock+delta-1);
                        if(val>0 && val<7)
                        {
                                // This means there is a piece which you can kill
				leftKill=true;
                        }
                }
        }
	uint16_t *oneStep,*twoStep,*lkill,*rkill;
	int totalStates=0;
	if(step1Fwd)
	{
		oneStep=new uint16_t[16];
		for(int i=0;i<16;i++) oneStep[i]=state[i];
		removePiece(oneStep,fromblock);
		if(moveWhite)
			addPiece(oneStep,fromblock+delta,6);
		else addPiece(oneStep,fromblock+delta,12);
		totalStates++;
	}

        if(step2Fwd)
        {
                twoStep=new uint16_t[16];
                for(int i=0;i<16;i++) twoStep[i]=state[i];
                removePiece(twoStep,fromblock);
                if(moveWhite) addPiece(twoStep,fromblock+2*delta,6);
		else addPiece(twoStep,fromblock+2*delta,12);
                totalStates++;
        }
	if(leftKill)
	{
		lkill=new uint16_t[16];
		for(int i=0;i<16;i++) lkill[i]=state[i];
		removePiece(lkill,fromblock);
		removePiece(lkill,fromblock+delta-1);
		if(moveWhite)
			addPiece(lkill,fromblock+delta-1,6);
		else
			addPiece(lkill,fromblock+delta-1,12);
		totalStates++;
		
	}
        if(rightKill)
        {
                rkill=new uint16_t[16];
                for(int i=0;i<16;i++) rkill[i]=state[i];
                removePiece(rkill,fromblock);
                removePiece(rkill,fromblock+delta+1);
                if(moveWhite)
                        addPiece(rkill,fromblock+delta+1,6);
                else
                        addPiece(rkill,fromblock+delta+1,12);
                totalStates++;

        }

//if((*newStates)[1]==NULL) 
        (*newStates)=new uint16_t*[totalStates];
        int cc=0;
        if(step1Fwd) (*newStates)[cc++]=oneStep;
        if(step2Fwd) (*newStates)[cc++]=twoStep;
        if(leftKill) (*newStates)[cc++]=lkill;
        if(rightKill) (*newStates)[cc++]=rkill;

//	printState(twoStep);
//	printState(*newStates[0]);
	printf("Pawn at %d block can move %d moves\n",fromblock,totalStates);
	return totalStates;

}

void addPiece(uint16_t *state,uint16_t toblock,uint16_t pieceVal)
{
        int arrElement=toblock/4;
        int offset=toblock%4;
        uint16_t mask;
	removePiece(state,toblock);
	mask=pieceVal;
	mask=mask<<((3-offset)*4);
	state[arrElement]=state[arrElement]|mask;
}

void removePiece(uint16_t *state,uint16_t fromblock)
{
	int arrElement=fromblock/4;
	int offset=fromblock%4;
	uint16_t mask;
	if(offset==0)
		mask=4095;
        else if(offset==1)
                mask=61695;
        else if(offset==2)
                mask=65295;	
        else if(offset==3)
                mask=65520;
	state[arrElement]=state[arrElement]&mask;
	if(getBlockVal(state,fromblock)!=0)
	{
		printf("ERROR: in removePiece(): %d\n",getBlockVal(state,fromblock));assert(false);
	}
}

void movePiece(uint16_t *state,uint16_t fromblock, uint16_t toblock)
{
	//printf("fromblock: %d toblock %d\n",fromblock,toblock);
	int toElement = toblock/4;
	int toOffset = toblock%4;
	uint16_t destMask;
        uint16_t pieceVal = getBlockVal(state,fromblock);
	//printf("pieceval: %d \n",pieceVal);
	removePiece(state,fromblock);
	uint16_t pieceMask = pieceVal;
        if (pieceVal == 6 && toblock / 8 == 0) {
          pieceMask = 5; // white pawn promotion
	  cout << "QUEEN" << endl;
        }
        else if (pieceVal == 12 && toblock / 8 == 7) {
          pieceMask = 11; // black pawn promotion
	  cout << "QUEEN" << endl;
	}
	pieceMask=pieceMask<<((3-toOffset)*4);
        switch (toOffset) {
		case 0: destMask=0x0FFF; break;
		case 1: destMask=0xF0FF; break;
		case 2: destMask=0xFF0F; break;
		case 3: destMask=0xFFF0; break;
		default: assert(false);
	}
//	printf("destMask: %d pieceMask%d\n",destMask,pieceMask);
	state[toElement]=(state[toElement]&destMask)|pieceMask;
	if(getBlockVal(state,fromblock)!=0)
	{
		printState(state);
                printf("toElement: %d toOffset: %d fromblock: %d toblock: %d\n",toElement,toOffset,fromblock,toblock);
                printf("pieceVal: %d pieceMask: %d destMask : %d \n",pieceVal,pieceMask,destMask);
		printf("ERROR: in movePiece(): %d\n",getBlockVal(state,fromblock));assert(false);
	}
}

int move(uint16_t *state,bool moveWhite)
{
	uint16_t allowMoves=0;
	if(moveWhite)
	{
		for(int i=0;i<64;i++)
		{
			int newStates=0;
			// If it is a white piece then try moving it
			int val=getBlockVal(state,i);
//			if(val<7 && val>0)
			{
				if(i==49) printf("A white piece at %d\n",val);
				uint16_t **newstates;
        			if(val==1) newStates=moveRook(state,moveWhite,i,&newstates,false);
			        else if(val==2) newStates=moveKnight(state,moveWhite,i,&newstates);
			        else if(val==3) newStates=moveBishop(state,moveWhite,i,&newstates,false);
			        else if(val==4) newStates=moveKing(state,moveWhite,i,&newstates);
			        else if(val==5) newStates=moveQueen(state,moveWhite,i,&newstates);
			        else if(val==6) newStates=movePawn(state,moveWhite,i,&newstates);				
				allowMoves+=newStates;
				if(i==54 && newStates>0)
				{
//					printf("DDDDDDDDDDDDDDDDDDDDd newStates=%d\n",newStates);
					for(int i=0;i<newStates;i++)
	                        	        printState(newstates[i]);
				}
			}
		}
		printf("White has %d allowed moves\n",allowMoves);
	}
        else
        {
                for(int i=0;i<64;i++)
                {
                        int newStates=0;
                        // If it is a white piece then try moving it
                        int val=getBlockVal(state,i);
//                      if(val<7 && val>0)
                        {
                                if(i==8) printf("A white piece at %d\n",val);
                                uint16_t **newstates;
                                if(val==7) newStates=moveRook(state,moveWhite,i,&newstates,false);
                                else if(val==8) newStates=moveKnight(state,moveWhite,i,&newstates);
                                else if(val==9) newStates=moveBishop(state,moveWhite,i,&newstates,false);
                                else if(val==10) newStates=moveKing(state,moveWhite,i,&newstates);
                                else if(val==11) newStates=moveQueen(state,moveWhite,i,&newstates);
                                else if(val==12) newStates=movePawn(state,moveWhite,i,&newstates);
                                allowMoves+=newStates;
                                if(i==47 && newStates>0)
                                {
//                                        printf("DDDDDDDDDDDDDDDDDDDDd i=%d newStates=%d\n",i,newStates);
                                      for(int i=0;i<newStates;i++)
                                              printState(newstates[i]);
                                }
                        }
                }
                printf("Black has %d allowed moves\n",allowMoves);
        }

}

// The following 64x64 bit structures are used to encode src/dest pairs that are plausible for
// some pieces.  For example, the jth bith of knightDests[i] is set iff it is possible for a knight
// to move from square i to square j.  These bit fields are populated once on initialization and then
// never changed afterwards.
/*
long long rankFileDests[64]; // Rook-like moves; used for rook, king, queen
long long diagonalDests[64]; // used for bishop, king, and queen
long long knightDests[64]; // used only for knights
*/
// These eight directional methods are used only to initialize the ___Dests structures above.
void north(long long* x, int row, int col)
{
  if (row >=0) {
    Set64(x,row*8+col);
    north(x,row-1,col); 
  }
}

void south(long long* x, int row, int col)
{
  if (row <=7) {
    Set64(x,row*8+col);
    south(x,row+1,col); 
  }
}

void east(long long* x, int row, int col)
{
  if (col <=7) {
    Set64(x,row*8+col);
    east(x,row,col+1); 
  }
}

void west(long long* x, int row, int col)
{
  assert (row >= 0);
  assert (row <= 7);
  if (col >=0) {
    Set64(x,row*8+col);
    west(x,row,col-1); 
  }
}

void northwest(long long* x, int row, int col)
{
  if (row >=0 && col <= 7) {
    Set64(x,row*8+col);
    northwest(x,row-1,col+1); 
  }
}

void northeast(long long* x, int row, int col)
{
  if (row >=0 && col >= 0) {
    Set64(x,row*8+col);
    northeast(x,row-1,col-1); 
  }
}

void southwest(long long* x, int row, int col)
{
  if (row <=7 && col >= 0) {
    Set64(x,row*8+col);
    southwest(x,row+1,col-1); 
  }
}

void southeast(long long* x, int row, int col)
{
  if (row <=7 && col <= 7) {
    Set64(x,row*8+col);
    southeast(x,row+1,col+1); 
  }
}

// The next few functions are used only to initialize the accessibility ____Dest structures
// above.  
void fillKnight(long long* x, int row, int col)
{
  if ((row >= 0 && row <= 7) &&
      (col >= 0 && col <= 7)) {
    Set64(x,row*8+col);
  }
}

void enumerateRankFile(long long* x, int i)
{
  int row = i / 8;
  int col = i % 8;
  north(x,row-1,col);
  south(x,row+1,col);
  east(x,row,col+1);
  west(x,row,col-1);
}

void enumerateDiagonals(long long* x, int i)
{
  int row = i / 8;
  int col = i % 8;
  northeast(x,row-1,col-1);
  northwest(x,row-1,col+1);
  southwest(x,row+1,col-1);
  southeast(x,row+1,col+1);
}

void enumerateKnight(long long* x, int i)
{
  int row = i / 8;
  int col = i % 8;
  fillKnight(x,row-1,col-2);
  fillKnight(x,row-2,col-1);
  fillKnight(x,row+1,col-2);
  fillKnight(x,row+2,col-1);
  fillKnight(x,row+1,col+2);
  fillKnight(x,row+2,col+1);
  fillKnight(x,row-1,col+2);
  fillKnight(x,row-2,col+1);
}

void enumerateSrcDestPairs()
{
  for (int i = 0; i < 64; i++) {
    enumerateRankFile(rankFileDests+i,i);
//	enumerateRankFile(&rankFileDests[i],i);
    enumerateDiagonals(diagonalDests+i,i);
//	enumerateDiagonals(&diagonalDests[i],i);
    enumerateKnight(knightDests+i,i);
//	enumerateKnight(&knightDests[i],i);


  }
}

// Put a q in the src square (i) and ps in all the squares that are reachable
// for the piece type whose structure is passed in as base (e.g., knightDests)
void displayAccessibleSquares(long long* base, int i)
{
  uint16_t state[16];
  long long* x = base + i;
  string s = sampleState(2);
  cout << "State: " << s << endl;
  for (int j = 0; j < 64; j++) {
    if (i == j) s[j] = 'q';
    else if (!(IsFree64(x,j))) s[j] = 'p';
    //cout << "x: " << *x << " j: " << j << " IsFree(x,j): " << (IsFree64(x,j)) << " 1<<j: " << (ONE << j) << endl;
  }
  fillBoard(state,s);
  printState(state);
}


// TODO: Convert these to bit operations
bool movesRankFile(uint16_t v) { return (v==1 || v== 4 || v == 5 || v == 7 || v == 10 || v == 11); }
bool movesDiagonal(uint16_t v) { return (v==3 || v== 4 || v == 5 || v == 9 || v == 10 || v == 11); }
bool isKing(uint16_t v) { return (v== 4 || v== 10);}
bool isKnight(uint16_t v) { return (v== 2 || v== 8);}

bool empty(uint16_t* state, uint16_t block)
{
  return getBlockVal(state,block) == 0;
}

// Returns true when white=true and val denotes a white piece
// Returns true when white=false and val denotes a black piece
bool ownPiece(uint16_t val, bool white)
{
  return (white && (val > 0 && val < 7)) || (!white && (val >= 7 && val < 13));  
}

bool ownPiece(uint16_t* state, uint16_t block, bool white)
{
  uint16_t val = getBlockVal(state,block);
  return (white && (val > 0 && val < 7)) || (!white && (val >= 7 && val < 13));  
}

// Returns true when white=true and val denotes a black piece
// Returns true when white=false and val denotes a white piece
bool opponentPiece(uint16_t val, bool white)
{
  return (!white && (val > 0 && val < 7)) || (white && (val >= 7 && val < 13));  
}

bool opponentPiece(uint16_t* state, uint16_t block, bool white)
{
  uint16_t val = getBlockVal(state,block);
  return (!white && (val > 0 && val < 7)) || (white && (val >= 7 && val < 13));  
}

bool isPawnTry(const uint16_t& move)
{
  const static uint16_t mask = 0x2000; // 0010000000000000
  return move & mask; // move & mask is non-zero if the move is blocked or would would leave player in check
}

// This version is for use when you just want to check make sure the "blocked" and "checked" bits are cleared
bool isLegal(const uint16_t& move)
{
  const static uint16_t mask = 0xC000; // 1100000000000000
  return !(move & mask); // move & mask is non-zero if the move is blocked or would would leave player in check
}

// This version ignores the blocked and checked bits from move and actually checks to make sure that the from/to
// combination is a legitimate move from state
// TODO
bool isLegalOnBoard(uint16_t* state, bool whiteMove, const uint16_t& move)
{
  assert(false);
  return true;
}

// TODO
bool isAttemptable(uint16_t* state, bool whiteMove, const uint16_t& move)
{
  assert(false);
  return true;
}

bool hasLegalMove(uint16_t* moves, int nMoves)
{
  for (int i = 0; i < nMoves; i++) {
    if (isLegal(moves[i])) {
      return true;
    }
  }
  cout << "NO LEGAL MOVES" << endl;
  return false; // None of the moves was doable
}

// if whiteKing is true, return the location of the white king
// otherwise return the location of the black king
int kingLocation(uint16_t* state, bool whiteKing)
{
  for (int i = 0; i < 64; i++) {
    if (getBlockVal(state,i) == (whiteKing ? 4 : 10)) return i; 
  }
  assert (false); // The king queried is not on the board at all!
}

// Return true if any of the moves in the moves array has a destination of square 
// and is not marked as being blocked.  (Use for determining if someone is in check).
bool unblockedAssaultOn(uint16_t* state, int square, uint16_t* moves, int nMoves)
{
  for (int i = 0; i < nMoves; i++) {
    uint16_t& move = moves[i];
    if (!(move & BLOCKED) && extractDestination(move) == square) {
        //cout << "CHECKED POSITION" << endl;
        //printState(state);
	//assert(false);
	return true;
    }
  }
  return false;
}

// If whiteKing is true, this function returns true iff the white king is in check in state
// If whiteKing is false, this function returns true iff the white black is in check in state
bool kingInCheck(uint16_t* state, bool whiteKing)
{
  //cout << "KING IN CHECK" << endl;
  //printState(state);
  static uint16_t moves[NMOVES];
  int nMoves = 0;
  generateAttemptableMoves(state, !whiteKing, moves, nMoves);
  int kingLoc = kingLocation(state ,whiteKing);
  return unblockedAssaultOn(state , kingLoc, moves, nMoves);

}

// Returns set of all moves from this position that are pawn tries 
set<uint16_t> pawnTries(uint16_t* state, bool whiteKing)
{
  //cout << "KING IN CHECK" << endl;
  //printState(state);
  static uint16_t moves[NMOVES];
  int nMoves = 0;
  set<uint16_t> result;
  generateAttemptableMoves(state, !whiteKing, moves, nMoves);
  for (int i = 0; i < nMoves; i++) {
    if (moves[i] & PAWNTRY) {
      result.insert(moves[i]);
    }
  }
  return result;
}

bool leavesKingInCheck(uint16_t* state, uint16_t move, bool whiteMove) 
{
  static uint16_t currentState[16];
  applyMove(state,currentState,move);
  //printState(currentState);
  return kingInCheck(currentState, whiteMove);
}


// Adds the CHECK flag to every move that would leave the current player's king in check.
// The array of moves is generated (previously) in the generateAttemptable moves method,
// This flag is not set at all.  This method is called afterwards to go back through and
// determine whether the move puts/leaves the king in check.  Don't run the check for check
// for blocked moves, since they will not be allowed anyway.
void checkForCheck(uint16_t* state, bool whiteMove, uint16_t* moves, int nMoves)
{
  for (int i = 0; i < nMoves; i++) {
    uint16_t& move = moves[i];
    if (isLegal(move) && leavesKingInCheck(state,move,whiteMove)) {
      move |= CHECKED; 
    }
  }
}

// Just print out the sequences of states implied by moveHistory, and the number of corresponding
// illegal attempted moves at each step
void processMoveHistory(uint16_t* state, vector<set <uint16_t> > failedMoves, uint16_t * moveHistory, int nMoves)
{
//  cout << "Moves: " << endl;
//  printState(state);
  for (unsigned i = 0; i < nMoves; i++) {
  //  cout << "Failed moves: " << failedMoves[i].size() << endl;
    uint16_t& move =  moveHistory[i];
    dispMove(move);
    assert(isLegal(move));
    applyMove(state, move);
 //   printState(state);
  }
}

// For testing purposes
int generateCannedMoves(uint16_t* state, bool whiteMove, uint16_t* moveHistory, VecSetMove& failedMoves)
{
  static int testNumber = 1;
  switch (testNumber) {
  case 0:
    moveHistory[0] = encodeMove(52,36,false,false);  // white king pawn advances two
    moveHistory[1] = encodeMove(12,28,false,false); // black king pawn advances two
    failedMoves[2].insert( encodeMove(36,28,true,false) ); // white king pawn fails to advance one more
    moveHistory[2] = encodeMove(55,47,false,false); // white rook pawn advances 1
    return 3;
  case 1:
    moveHistory[0] = encodeMove(52,36,false,false); // white king pawn advances two
    moveHistory[1] = encodeMove(13,21,false,false); // black f pawn advances one
    moveHistory[2] = encodeMove(36,28,false,false); // white king pawn advances one more
    failedMoves[3].insert( encodeMove(12,28,true,false) ); // black king pawn fails to advance two
    moveHistory[3] = encodeMove(21,28,false,false,true); // black f pawn captures white king pawn
    moveHistory[4] = encodeMove(59,31,false,false); // white queen jumps out to h file and checks black king
    moveHistory[5] = encodeMove(14,22,false,false); // black blocks check with g pawn
    failedMoves[6].insert( encodeMove(31,13,true,false) ); // white tries to "jump over" the blocking g pawn
    moveHistory[6] = encodeMove(61,52,false,false); // white moves bishop in front of king
    failedMoves[7].insert( encodeMove(15,31,true,false) ); // black fails to advance h file pawn (because of white queen)
    moveHistory[7] = encodeMove(22,31,false,false,true); // black captures white queen with g pawn
    moveHistory[8] = encodeMove(52,31,false,false); // white captures pawn that captured queen and puts black in checkmate
    return 9;
  }
}

int generateRandomMoves(uint16_t* state, bool whiteMove, uint16_t * moveHistory, vector< set <uint16_t> >failedMoves, uint16_t** levels, int depth, int maxdepth)
{
  if (depth == maxdepth) {
	//printState(state);
     return depth;
	//processMoveHistory(failedMoves,moveHistory);
  }
  int nMoves = 0;
  uint16_t newState[16]; 
  generateAttemptableMoves(state, whiteMove, levels[depth], nMoves, false);
  checkForCheck(state, whiteMove, levels[depth], nMoves);
  assert (nMoves < NMOVES); // 
  if (!hasLegalMove(levels[depth],nMoves)) return depth; // No legal moves from this state
  while (true) { // Keep trying random moves until a legal one is executed
    int attemptedMoveIndex = rand() % nMoves;
    uint16_t& move = levels[depth][attemptedMoveIndex];
    if (isLegal(move)) {
   //   cout << "Selected move: " << decodeMove(state,move) << endl;
      applyMove(state,newState,move);
   //   printState(newState);
      moveHistory[depth] = move;
      return generateRandomMoves(newState,!whiteMove,moveHistory,failedMoves,levels,depth+1,maxdepth);
    } else {
      failedMoves[depth].insert(move); // Note that this illegal move was attempted 
    }
  }
}

// TODO
bool samePawnTries(uint16_t* state1, uint16_t* state2, bool whiteMove)
{
  //return true;
  set<uint16_t> state1Tries = pawnTries(state1,whiteMove);
  set<uint16_t> state2Tries = pawnTries(state2,whiteMove);
  return state1Tries == state2Tries;
}

// Returns true if the player whose turn it is in check in both states or
// is not in check in both states.
// TODO: We are not currently declaring whether the check is on a rank, file, long diag, short diag,
// or knight.  And in fact, it is possible to be in check from two different sources at the same time 
// (but not more than two).  The problem is that I've run out of bits in my 16-bit encodings to set these flags
bool sameCheckStatus(uint16_t* state1, uint16_t* state2, bool whiteMove)
{
  return kingInCheck(state1, whiteMove) == kingInCheck(state2, whiteMove);
}

// Returns the number of moves in the array that has one or both of the CHECKED/BLOCKED flags set
unsigned countIllegalMoves(uint16_t* moves, int nMoves)
{
  unsigned count = 0;
  for (int i = 0; i < nMoves; i++) {
    if (!isLegal(moves[i])) count++;
  }
  return count;
}

// Returns true if "move" also appears in moveList
bool foundMatchingMove(const uint16_t move, uint16_t* moveList, int nMoves)
{
      for (int i = 0; i < nMoves; i++) {
        if (move == moveList[i]) {
	  // The attempted but illegal move in the actual state is also attempted and illegal in this
	  // possible state.  This is crucial, because if our input were the actual list of observations, those observations
	  // for this move would have to match exactly.
	  return true;  
        }
      }
      return false; // No matching move found
}

// Generate a list of all the sequences of moves that are consistent with the observations that would have been received by the player denoted by 
// whitePerspective (T=white, F=black).  The moveHistory contains the list of all the successful moves (even indexed moves for white, odds for black)
// and the associated set of unsuccessful moves are in failedMoves, i.e., failedMoves[i] is the set of all unsuccessful moves that were tried
// before the move that was ultimately successful at time i (the successful move is moveHistory[i]).  <moveHistory,failedMoves> together does not include 
// the actual observations, but we will recreate them on the fly by updating trueState with the actual move as we go along in the search tree.
// At levels in the recursion where whiteMove == whitePerspective, we can act as though we know exactly which moves were attempted and which move was
// actually made (because we made those moves).  So we verify that every failedMove is at least attemptable in the possState.
// At the levels where whiteMove != whitePerspective, we must act as though we know the number of failed moves, but not the specific failed Moves.
// In order to continue with the recursion, we must be able to verify that the number of attemptable moves in possState is strictly greater than the
// number of moves in possState.
// Since both players would hear the moderator's declarations about pawn tries and "check", we do not continue unless the pawn tries and check status
// of possState and trueState are equivalent.
	// Arg 0: Player whose perspective we are working from.  If it's white than we assume that we know the even numbered
        // entries in moveHistory and failedMoves EXACTLY and that we have access to the NUMBER of failedMoves for the odd 
	// numbered moves.  For black, it's the other way around.
	// Arg 1: actual start state
	// Arg 2: possible start state (we always assume that we know the start state)
	// Arg 4: whose turn it is initially
	// Arg 5 & Arg 7: the actuall sequences of moves that were accepted and the corresponding lists of other attempted moves 
	// Arg 6 & Arg 8: working space for tracking possible sequences of moves and the alternatives at each level
	// Arg 9: current depth
	// Arg 10: maximum depth (i.e., if you get that far without conflicts, you've found a solution)
void generateInformationSet(bool whitePerspective, uint16_t* trueState, uint16_t* possState, bool whiteMove, uint16_t* moveHistory, 
  uint16_t* possHistory, VecSetMove& failedMoves, uint16_t** levels, int depth, int maxdepth)
{
  // Need to check that the messages match
  if (!samePawnTries(trueState, possState, whiteMove)) return;  // has not been implemented
  if (!sameCheckStatus(trueState, possState, whiteMove)) return; 

  if (depth == maxdepth) { // Then we have found a solution
	nSolutions++;
	// Right now, just display the solution; eventually we'll need to do something else with it
#ifdef PRINT_SOLUTIONS
	uint16_t destructibleState[16];
//TODO UNCOMMENT        copyState(globalState,destructibleState);
        for (int i = 0; i < depth; i++) {
	  if (i%2 == 0) cout << (i/2+1) << ". ";
	  cout << decodeMove(destructibleState,possHistory[i]) << " ";
	  applyMove(destructibleState,possHistory[i]);
	  //cout << (i/2+1) << (i%2 ? "B. " : "W. ") << decodeMove(possHistory[i]) << " ";
        }
        cout << endl;
        printState(destructibleState);
#endif
	return;
  }

  // Note: since the same application is being done for every call at this depth; we could just compute the
  // new global state at this depth once before the initial call and then just move a pointer around
  uint16_t newTrueState[16]; 
  applyMove(trueState,newTrueState,moveHistory[depth]);

  uint16_t newPossState[16]; 
  // Note that the legality of all the moves at levels[depth] will be set according the possible state, not actual
  int nMoves = 0;
  // TODO: Rather than generate ALL the moves in advance (and incur the associated penatly for storing them all at each level)
  // we could conceivably redo things so that we only keep track of enough information (src square, direction, and offset) to
  // generate the NEXT attemptable move
  generateAttemptableMoves(possState, whiteMove, levels[depth], nMoves,false);
  checkForCheck(possState, whiteMove, levels[depth], nMoves);
  assert (nMoves < NMOVES); // 

  if (whitePerspective == whiteMove) { // active player is the player from whose perspective we are working
    SetMove& failures = failedMoves[depth];
    for (SetMove::const_iterator itr = failures.begin(); itr != failures.end(); ++itr) {
      uint16_t move = *itr ;
      if (!foundMatchingMove(move,levels[depth],nMoves)) {
        // This means that one of the illegal moves that this player made in the actual game is not 
        // legal/attemptable from this position.  So this overall sequence of moves is not plausible
	// and we must prune.
        return; 
      }
    }
    // If we get to this point, all of the failed moves are consistent
    // We know exactly what the actual move was; make it
    uint16_t& actualMove = moveHistory[depth];
    if (!foundMatchingMove(actualMove,levels[depth],nMoves)) {
      // This means that the move that we know we made at this depth is not legal under this possible
      // sequence of moves.  So we must prune.
      return; 
    }
    // Otherwise, recurse 
    applyMove(possState,newPossState,actualMove);
    possHistory[depth] = actualMove;
    generateInformationSet(whitePerspective, newTrueState, newPossState, !whiteMove, 
      moveHistory, possHistory, failedMoves, levels, depth+1, maxdepth);
  } else { 
    // We are at a level in the search tree where we are considering the possible moves for the opponent.
    // We assume that we know the number of attempted illegal moves (because we would have heard the moderator
    // declare each one to be illegal as it was made).  But we do not know the move in moveHistory or the specific
    // moves in failedMoves that did not succeed. 

    // Ensure that there are enough attemptable moves in the state to match the observed number of failed attempts
    unsigned nIllegalMoves = countIllegalMoves(levels[depth], nMoves);
    if (nIllegalMoves < failedMoves[depth].size()) {
        // This means that the number of moves possible for the opponent at this hypothetical stage is less than the number of
	// attemptable moves it actually tried.  So we must prune.
	return; 
    }

    // Now we want to try each possible move that is legally executable (not just attemptable) 
    for (int i = 0; i < nMoves; i++) {
      uint16_t& move = levels[depth][i];
      if (isLegal(move)) { // Obviously, we can only execute the moves that are actually legal from this state
        applyMove(possState,newPossState,move);
        possHistory[depth] = move;
        generateInformationSet(whitePerspective, newTrueState, newPossState, !whiteMove, 
          moveHistory, possHistory, failedMoves, levels, depth+1, maxdepth);
      }
    }
  }
}


// If whiteMove is true, then it is white's turn to move from state.
// Generate all the possible attemptable moves from this state.  Store each move as a 16-bit structure in moves
// Increment nMoves for each attemptable move found. 
// If verbose=true, print out the attemptable moves as they are found.
void generateAttemptableMoves(uint16_t* state, bool whiteMove, uint16_t* moves, int& nMoves, bool verbose )
{
  //printState(state);
  static int8_t rookOffsets[] = {-8,1,8,-1}; // -8=NORTH, 1=EAST, 8=SOUTH, -1=WEST
  static int8_t bishopOffsets[] = {-9,-7,9,7}; // -9=NORTHWEST, -7=NORTHEAST, 9=SOUTHEAST, 7=SOUTHWEST
  static int8_t knightOffsets[] = {-17,-15,-10,-6,6,10,15,17}; // offsets for eight potential knightMoves

  // Iterate through the squares in order, looking for pieces that active player owns
  for (int src = 0; src < 64; src ++) {
    uint16_t pieceVal = getBlockVal(state,src);
    if (!pieceVal || opponentPiece(pieceVal,whiteMove)) continue; // Can't move emptiness or opponent's pieces
    uint16_t destPiece = 0;
    bool blocked = false;
    int dest;
     
    // Knight moves
    if (isKnight(pieceVal)) {
      for (int i = 0; i < 8; i++) {
        uint16_t dest = src + knightOffsets[i];
        if (dest > 63 || dest < 0) continue; // we're shooting off the board
        if (!ownPiece(state,dest,whiteMove)) { // can't jump onto my own piece
          if (knightDests[src] & (ONE << dest)) { // the src/dest combo represents a legal knight move
            if (verbose) printf("knight   src: %d dest: %d\n",src,dest);
		// TODO: Add a flag if it's a capture
            moves[nMoves++] = encodeMove(src,dest,false,false);
			assert(moves[nMoves-1]);
          }
        }
      } 
    }
    // Rook moves
    long long* r = rankFileDests + src;
//	long long* r = &rankFileDests[src];
    if (movesRankFile(pieceVal)) {
      for (int dir = 0; dir < 4; dir++) { // N, E, S, W
        blocked = false;
        for (int off = 1; off < 8; off++) {
          dest = src + rookOffsets[dir]*off;
          //cout << dest << endl;
          if (dest < 0 || dest > 63) {
            //printf("Dest %d; break\n",dest);
            break;
          }
          if (IsFree64(r,dest)) { 
            //printf ("IsFree64: %lld dest: %d\n",*r,dest);
            break;
          }
          destPiece = getBlockVal(state,dest);
          if (ownPiece(destPiece,whiteMove)) break; 
          if (verbose) printf("rooklike src: %d dest: %d blocked: %d\n",src,dest,blocked);
          moves[nMoves++] = encodeMove(src,dest,blocked,false);
			assert(moves[nMoves-1]);
		// TODO: Add a flag if it's a capture
	  // If the destination is occupied by an opponent's piece, this move is a capture.
	  // Furthermore, all moves that continue in this direction should be marked as blocked
          if (opponentPiece(destPiece,whiteMove)) blocked = true; 
          if (isKing(pieceVal)) break; // King can move only one square, so look no further in this dir
        }
      }
    }
    // Bishop moves -- this is copy/paste code from rook moves and can be extracted into its 
    // own function;
    long long* b = diagonalDests + src;
//	 long long* b = &diagonalDests[src];

    if (movesDiagonal(pieceVal)) { // Only consider diagonal moves for bishops, queens, and kings
      //displayAccessibleSquares(diagonalDests,src);
      for (int dir = 0; dir < 4; dir++) { // NW, NE, SE, SW
        blocked = false;
        for (int off = 1; off < 8; off++) {
          dest = src + bishopOffsets[dir]*off;
          if (dest < 0 || dest > 63) break;
          if (IsFree64(b,dest)) break;
          destPiece = getBlockVal(state,dest);
          if (ownPiece(destPiece,whiteMove)) break; 
          if (verbose) printf("diagonal src: %d dest: %d blocked: %d\n",src,dest,blocked);
		// TODO: Add a flag if it's a capture
	  // If the destination is occupied by an opponent's piece, this move is a capture.
	  // Furthermore, all moves that continue in this direction should be marked as blocked
          moves[nMoves++] = encodeMove(src,dest,blocked,false);
			assert(moves[nMoves-1]);
          if (opponentPiece(destPiece,whiteMove)) blocked = true; 
          if (isKing(pieceVal)) break; // King can move only one square, so look no further in this dir
        }
      }
    }
    // Pawn moves
    if (pieceVal == 6 || pieceVal == 12) { // pawns
      dest = (pieceVal == 6) ? src - 8 : src + 8; // Can move forward or backward depending on color
      if (dest > 0 && dest < 64) {
        destPiece = getBlockVal(state,dest);
        blocked = opponentPiece(destPiece,whiteMove);
        if (!ownPiece(destPiece,whiteMove)) { // I can attempt to move one square forward
          if (verbose) printf("pawnmove src: %d dest: %d blocked: %d\n",src,dest,blocked);
          moves[nMoves++] = encodeMove(src,dest,blocked,false);
			assert(moves[nMoves-1]);
          // check for possibility of jumping 2
          if ((pieceVal == 6 && (src / 8 ) == 6) || ((src / 8) == 1 && pieceVal == 12)) {
            int dest2 = (pieceVal == 6) ? dest - 8 : dest + 8; // dest space if pawn moves two
	    int dest2Piece = getBlockVal(state,dest2);
	    bool blocked2 = opponentPiece(dest2Piece,whiteMove);
	    if (!ownPiece(dest2Piece,whiteMove)) {
              if (verbose) printf("pawn-up2 src: %d dest: %d blocked: %d\n",src,dest2,blocked || blocked2);
              moves[nMoves++] = encodeMove(src,dest2,blocked || blocked2,false);
			assert(moves[nMoves-1]);
            }
          }
        }
      }
      // The squares where the pawn can capture will be +/- 1 from the "move ahead one" square
      dest++;
      if (dest >= 0 && dest < 64 && (diagonalDests[src] & (ONE << dest)) && opponentPiece(getBlockVal(state,dest),whiteMove)) {
          if (verbose) printf("pawncapt src: %d dest: %d \n",src,dest);
          moves[nMoves++] = encodeMove(src,dest,blocked,false) | (1 << 13); // 13 == PAWN TRY CODE
			assert(moves[nMoves-1]);
      }
      dest -= 2;
      if (dest >= 0 && dest < 64 && (diagonalDests[src] & (ONE << dest)) && opponentPiece(getBlockVal(state,dest),whiteMove)) {
          if (verbose) printf("pawncapt src: %d dest: %d \n",src,dest);
          moves[nMoves++] = encodeMove(src,dest,blocked,false) | (1 << 13); // 13 == PAWN TRY CODE
			assert(moves[nMoves-1]);
      }
    }
  }
}



class HcState: public State {
public:

	//depth	
	int k;
	uint16_t board[16]; //this is posststate
	uint16_t * possHistory; 
	uint16_t trueState[16]; 
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
				 
		uint16_t state[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	
//		uint16_t stateCopy[16];
 		string s = sampleState(0);
        fillBoard(state,s);
  //      copyState(state,stateCopy);
    //    copyState(state,globalState);

		 startState->moveWhite = true;
		 startState->possHistory=NULL;
		 startState->k = 0;
		 copy(&state[0],&state[16],&startState->board[0]);
		 copy(&state[0],&state[16],&startState->trueState[0]);
	//	 startState->board 
		 qs->push(startState);

    }

    inline void expand(State *curState, NodeQueue *qs){
      HcState *parent = ((HcState *)curState);
      int childnum = 0;
	  int parentk = parent->k;

	  int depth = parent->k;
	  
	  assert(depth<=maxdepth);

	  uint16_t * possState = parent->board;
	  int whiteMove = parent->moveWhite;

	 parent->possHistory = (uint16_t *)((char *)parent +sizeof(HcState));

	  uint16_t *possHistory = parent->possHistory;
	  uint16_t *trueState = parent->trueState;

#ifdef DEBUG
		CkPrintf("Expanding state with depth %d  : \n", parentk);
#endif
      if(parentk < sequential_threshold){

			  //Check if it is a solution 
	if(parentk == maxdepth )
	{

	nSolutions++;
#ifdef DEBUG
		CkPrintf("Goal reached \n"); 
		CkPrintf("[%d] nSolutions %d \n",CkMyPe(),nSolutions);

#endif
		CkPrintf("[%d] nSolutions %d \n",CkMyPe(),nSolutions);

		return;
	}
	//statesExpanded++;

	/* Node expansion logic ..... ***************************************/
	////////////////////////////////////////////////////////////////////////////
	

	//TODO
	  // Need to check that the messages match
  if (!samePawnTries(trueState, possState, whiteMove)) return;  // has not been implemented
  if (!sameCheckStatus(trueState, possState, whiteMove)) return; 

  if (depth == maxdepth) { // Then we have found a solution
	// Right now, just display the solution; eventually we'll need to do something else with it
#ifdef PRINT_SOLUTIONS
	uint16_t destructibleState[16];
   //TODO UNCOMMENT     copyState(globalState,destructibleState);
        for (int i = 0; i < depth; i++) {
	  if (i%2 == 0) cout << (i/2+1) << ". ";
	  CkPrintf("%s ", decodeMove(destructibleState,possHistory[i])) ;
	  applyMove(destructibleState,possHistory[i]);
	  //cout << (i/2+1) << (i%2 ? "B. " : "W. ") << decodeMove(possHistory[i]) << " ";
        }
       // Ckut << endl;
      //  printState(destructibleState);
#endif
	return;
  }

  // Note: since the same application is being done for every call at this depth; we could just compute the
  // new global state at this depth once before the initial call and then just move a pointer around
  uint16_t newTrueState[16]; 
  applyMove(trueState,newTrueState,moveHistory[depth]);
	uint16_t levels[NMOVES];
  uint16_t newPossState[16]; 
  // Note that the legality of all the moves at levels[depth] will be set according the possible state, not actual
  int nMoves = 0;
  // TODO: Rather than generate ALL the moves in advance (and incur the associated penatly for storing them all at each level)
  // we could conceivably redo things so that we only keep track of enough information (src square, direction, and offset) to
  // generate the NEXT attemptable move
  generateAttemptableMoves(possState, whiteMove, levels, nMoves,false);
  checkForCheck(possState, whiteMove, levels, nMoves);
  assert (nMoves < NMOVES); // 

  if (whitePerspective == whiteMove) { // active player is the player from whose perspective we are working
    SetMove& failures = failedMoves[depth];
    for (SetMove::const_iterator itr = failures.begin(); itr != failures.end(); ++itr) {
      uint16_t move = *itr ;
      if (!foundMatchingMove(move,levels,nMoves)) {
        // This means that one of the illegal moves that this player made in the actual game is not 
        // legal/attemptable from this position.  So this overall sequence of moves is not plausible
	// and we must prune.
        return; 
      }
    }
    // If we get to this point, all of the failed moves are consistent
    // We know exactly what the actual move was; make it
    uint16_t& actualMove = moveHistory[depth];
    if (!foundMatchingMove(actualMove,levels,nMoves)) {
      // This means that the move that we know we made at this depth is not legal under this possible
      // sequence of moves.  So we must prune.
      return; 
    }
    // Otherwise, recurse 

//	applyMove(possState,newPossState,actualMove);
 //   possHistory[depth] = actualMove;

// Push the new states onto queue 
				HcState *child = (HcState *)qs->registerState(sizeof(HcState) + (depth+1)*sizeof(uint16_t), childnum, MAX_CHILDREN);
			//	HcState *child = (HcState *)qs->registerState(sizeof(HcState));
				child->k = parentk+1;
				child->moveWhite = !whiteMove;
				
				applyMove(possState,child->board,actualMove);
				//set possHistory

				child->possHistory = (uint16_t *)((char *)child +sizeof(HcState));
				for(int p =0; p <child->k-1; p++)
						child->possHistory[p] = possHistory[p];

				child->possHistory[depth] = actualMove;
				copy(&newTrueState[0], &newTrueState[16], child->trueState);

				qs->push(child);
				childnum++;
				//Free newStates
			//	delete [] newstates[j];
						
				

//				if(newStates !=0) 
//					delete [] newstates;

				/* ***************************************/
// Pushed the new states onto queue 

//      generateInformationSet(whitePerspective, newTrueState, newPossState, !whiteMove, moveHistory, possHistory, failedMoves, levels, depth+1, maxdepth);
  } else { 
    // We are at a level in the search tree where we are considering the possible moves for the opponent.
    // We assume that we know the number of attempted illegal moves (because we would have heard the moderator
    // declare each one to be illegal as it was made).  But we do not know the move in moveHistory or the specific
    // moves in failedMoves that did not succeed. 

    // Ensure that there are enough attemptable moves in the state to match the observed number of failed attempts
    unsigned nIllegalMoves = countIllegalMoves(levels, nMoves);
    if (nIllegalMoves < failedMoves[depth].size()) {
        // This means that the number of moves possible for the opponent at this hypothetical stage is less than the number of
	// attemptable moves it actually tried.  So we must prune.
	return; 
    }

    // Now we want to try each possible move that is legally executable (not just attemptable) 
    for (int i = 0; i < nMoves; i++) {
      uint16_t& move = levels[i];
      if (isLegal(move)) { // Obviously, we can only execute the moves that are actually legal from this state

	HcState *child = (HcState *)qs->registerState(sizeof(HcState) + (depth+1)*sizeof(uint16_t), childnum, MAX_CHILDREN);
			//	HcState *child = (HcState *)qs->registerState(sizeof(HcState));
				child->k = parentk+1;
				child->moveWhite = !whiteMove;
				
				applyMove(possState,child->board,move);
				//set possHistory

				child->possHistory = (uint16_t *)((char *)child +sizeof(HcState));
				for(int p =0; p <(child->k)-1; p++)
						child->possHistory[p] = possHistory[p];

				child->possHistory[depth] = move;
				
				copy(&newTrueState[0], &newTrueState[16], child->trueState);
			//	copy(&possHistory, &possHistory[j][16], child->board);

				qs->push(child);
				childnum++;

//        applyMove(possState,newPossState,move);
  //      possHistory[depth] = move;
    //    generateInformationSet(whitePerspective, newTrueState, newPossState, !whiteMove,  moveHistory, possHistory, failedMoves, levels, depth+1, maxdepth);
      }
    }
  }
}

	
	
	////////////////////////////////////////////////////////////////////////////
/*	uint16_t allowMoves=0;
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

	//							foundSolution();
	  }
	*/
	 else{
			 	uint16_t attemptableMoves[NMOVES*MAXDEPTH];

	// Partition the space allocated above into different levels, where each level allows for NMOVES moves
        // This is done to avoid having to reallocate memory every time we want to generate a new list of moves
        // and also to potentially improve the spatial locality of the program
	uint16_t* moveList[MAXDEPTH];
        for (int i = 0; i < MAXDEPTH; i++) {
		moveList[i] = &attemptableMoves[i*NMOVES];
        }

        recursive_hc(parent->trueState,parent->board,parent->moveWhite,parent->possHistory,depth,moveList);
      }
    }

    void recursive_hc(uint16_t *trueState,uint16_t *possState,bool whiteMove,uint16_t *possHistory,int depth,uint16_t **levels){


	  assert(depth<=maxdepth);

  // Need to check that the messages match
  if (!samePawnTries(trueState, possState, whiteMove)) return;  // has not been implemented
  if (!sameCheckStatus(trueState, possState, whiteMove)) return; 

  if (depth == maxdepth) { // Then we have found a solution
	nSolutions++;
//	if(nSolutions==0) 
			CkPrintf("[%d] nSolutions %d \n",CkMyPe(),nSolutions);

	// Right now, just display the solution; eventually we'll need to do something else with it
#ifdef PRINT_SOLUTIONS
	uint16_t destructibleState[16];
        copyState(globalState,destructibleState);
        for (int i = 0; i < depth; i++) {
	  if (i%2 == 0) cout << (i/2+1) << ". ";
	  cout << decodeMove(destructibleState,possHistory[i]) << " ";
	  applyMove(destructibleState,possHistory[i]);
	  //cout << (i/2+1) << (i%2 ? "B. " : "W. ") << decodeMove(possHistory[i]) << " ";
        }
        cout << endl;
        printState(destructibleState);
#endif
	return;
  }

  // Note: since the same application is being done for every call at this depth; we could just compute the
  // new global state at this depth once before the initial call and then just move a pointer around
  uint16_t newTrueState[16]; 

  if(moveHistory[depth]==0) CkPrintf("depth %d \n ",depth);
  assert(moveHistory[depth]);

  applyMove(trueState,newTrueState,moveHistory[depth]);

  uint16_t newPossState[16]; 
  // Note that the legality of all the moves at levels[depth] will be set according the possible state, not actual
  int nMoves = 0;
  // TODO: Rather than generate ALL the moves in advance (and incur the associated penatly for stori

 // TODO: Rather than generate ALL the moves in advance (and incur the associated penatly for storing them all at each level)
  // we could conceivably redo things so that we only keep track of enough information (src square, direction, and offset) to
  // generate the NEXT attemptable move
  generateAttemptableMoves(possState, whiteMove, levels[depth], nMoves,false);
  checkForCheck(possState, whiteMove, levels[depth], nMoves);
  assert (nMoves < NMOVES); // 

  if (whitePerspective == whiteMove) { // active player is the player from whose perspective we are working
    SetMove& failures = failedMoves[depth];
    for (SetMove::const_iterator itr = failures.begin(); itr != failures.end(); ++itr) {
      uint16_t move = *itr ;
      if (!foundMatchingMove(move,levels[depth],nMoves)) {
        // This means that one of the illegal moves that this player made in the actual game is not 
        // legal/attemptable from this position.  So this overall sequence of moves is not plausible
	// and we must prune.
        return; 
      }
    }
    // If we get to this point, all of the failed moves are consistent
    // We know exactly what the actual move was; make it
    uint16_t& actualMove = moveHistory[depth];
    if (!foundMatchingMove(actualMove,levels[depth],nMoves)) {
      // This means that the move that we know we made at this depth is not legal under this possible
      // sequence of moves.  So we must prune.
      return; 
    }
    // Otherwise, recurse 
    applyMove(possState,newPossState,actualMove);
    possHistory[depth] = actualMove;
    recursive_hc(newTrueState, newPossState, !whiteMove, 
      possHistory, depth+1,levels);
  } else { 
    // We are at a level in the search tree where we are considering the possible moves for the opponent.
    // We assume that we know the number of attempted illegal moves (because we would have heard the moderator
    // declare each one to be illegal as it was made).  But we do not know the move in moveHistory or the specific
    // moves in failedMoves that did not succeed. 

    // Ensure that there are enough attemptable moves in the state to match the observed number of failed attempts
    unsigned nIllegalMoves = countIllegalMoves(levels[depth], nMoves);
    if (nIllegalMoves < failedMoves[depth].size()) {
        // This means that the number of moves possible for the opponent at this hypothetical stage is less than the number of
	// attemptable moves it actually tried.  So we must prune.
	return; 
    }

    // Now we want to try each possible move that is legally executable (not just attemptable) 
    for (int i = 0; i < nMoves; i++) {
      uint16_t& move = levels[depth][i];
      if (isLegal(move)) { // Obviously, we can only execute the moves that are actually legal from this state
        applyMove(possState,newPossState,move);
        possHistory[depth] = move;
        recursive_hc(newTrueState, newPossState, !whiteMove, 
          possHistory, depth+1,levels);
      }
    }





			//////////////////////////////////////////////////////

	}
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
	 nSolutions = 0;

	// One time set up to mark plausible src/destination pairs for different piece types
        enumerateSrcDestPairs();
        //assert (argc == 2);
        //srand(atoi(argv[1]));
        srand(63214234);
        //srand(time(0));

	// Keep lists of moves that are attemptable at each depth
	uint16_t attemptableMoves[NMOVES*MAXDEPTH];

	// Partition the space allocated above into different levels, where each level allows for NMOVES moves
        // This is done to avoid having to reallocate memory every time we want to generate a new list of moves
        // and also to potentially improve the spatial locality of the program
	uint16_t* moveList[MAXDEPTH];
        for (int i = 0; i < MAXDEPTH; i++) {
		moveList[i] = &attemptableMoves[i*NMOVES];
        }

	// Generate an actual sequence of moves; then use this sequence of moves (implicitly the observations
        // that would have been received from ONE of the players if this WERE the actual sequence of moves
        // to generate the information set -- the set of ALL sequences of moves (including this one) for which
        // the observations would be the same
      //  uint16_t moveHistory[MAXDEPTH];

	// Keep track of the list of failed moves at each step.  failedMoves[i] gives the set of moves that were
	// attempted but not accepted before the corresponding legal move in moveHistory[i] was issued.
       // VecSetMove failedMoves(MAXDEPTH);
	/*	set <uint16_t>
		for(int k =0; k <MAXDEPTH; k++)
				failedMoves.push_back(new set<uint16_t>);
				*/
        // Use this to keep track of POSSIBLE sequences of moves that match the (implied) observations from moveHistory
        // If we ever make it to the deepest level without generating a conflict, then we have found a solution in the 
        // search tree
        //uint16_t possHistory[MAXDEPTH];

	// Initialize board to any starting configuration of interest
	// state will contain the start state of interest
	uint16_t state[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint16_t stateCopy[16];
 	string s = sampleState(0);
        fillBoard(state,s);
        copyState(state,stateCopy);
       //TODO
//UNCOMMENT		copyState(state,globalState);

	// Randomly generate a sequence of moves OR produce a carefully crafted example sequence
        int nMoves = 0;

		
	int nExecutedMoves = generateRandomMoves(state,true,moveHistory,failedMoves,moveList,0,MAX_DEPTH);

	maxdepth = nExecutedMoves;

//int nExecutedMoves = generateCannedMoves(state,true,moveHistory,failedMoves);
	// Display the actual sequence of moves (for testing/debugging purposes)

		
		cout << nExecutedMoves << endl;
        processMoveHistory(stateCopy,failedMoves,moveHistory,nExecutedMoves);
	//return 0;

	cout << "BEGINNING INFORMATION SET GENERATION" << endl;

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
	fclose()
*/

   
    searchEngineProxy.start();
}

AppCore *newAppCore(){
  return new HcCore;
}

#include "hc.def.h"
