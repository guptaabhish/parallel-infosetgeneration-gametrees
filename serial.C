//CS598 Proj
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cassert>
#include <string>
#include <iostream>
#include <sstream>
#include <set>
#include <vector>
#include <string.h> // memcpy
using namespace std;

typedef set< uint16_t > SetMove;
typedef vector< SetMove > VecSetMove;

#define IsFree(a,ind)  !( (a[(ind / 32)]) & (1<<(ind % 32)) )
#define Set(a,ind) a[(ind/32)] = ( a[(ind/32)] | (1<<(ind % 32)) )
#define Reset(a,ind) a[(ind/32)] = ( a[(ind/32)] & (~(1<<(ind % 32))) )

#define IsFree16(a,ind)  !( (a[(ind / 16)]) & (1<<(ind % 16)) )
#define Set16(a,ind) a[(ind/16)] = ( a[(ind/16)] | (1<<(ind % 16)) )
#define Reset16(a,ind) a[(ind/16)] = ( a[(ind/16)] & (~(1<<(ind % 16))) )

const int NMOVES = 1000;
const int NLAYERS = 60;
const int STATESIZE = 16;

uint16_t getBlockVal(uint16_t*,int);
void removePiece(uint16_t *state,uint16_t fromblock);
void addPiece(uint16_t *state,uint16_t toblock,uint16_t pieceVal);
void movePiece(uint16_t *state,uint16_t fromblock, uint16_t toblock);
int moveRook(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates,bool);
int moveBishop(uint16_t *state,bool moveWhite,uint16_t fromblock,uint16_t ***newStates,bool);

// MDR
string disp(int* x, int max)
{
  ostringstream os;
  for (int i = max - 1; i >= 0; i--) {
    os << ((IsFree(x,i)) ? "0" : "1") ; 
  }
  return os.str();
}

string disp16(uint16_t* x, int max)
{
  ostringstream os;
  for (int i = max - 1; i >= 0; i--) {
    os << ((IsFree16(x,i)) ? "0" : "1") ; 
  }
  return os.str();
}

uint16_t encodeMove(uint16_t from, uint16_t to, bool blocked, bool checked)
{
  uint16_t result = 0;
  result = (blocked << 15) | (checked << 14) | (from << 6) | to ;
}

void decodeMove(uint16_t move, uint16_t& from, uint16_t& to, bool& blocked, bool& checked)
{
  blocked = (1 << 15) & move; 
  checked = (1 << 14) & move; 
  from = (4032 & move) >> 6; 
  to = 63 & move; 
}

void dispMove(uint16_t from, uint16_t to, bool blocked, bool checked)
{
  cout << "from: " << from << " to: " << to << " blocked: " << blocked << " checked: " << checked << endl;
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

void fillBoard(uint16_t* state, const string& sState)
{
  assert (sState.size() == 64);
  for (unsigned i = 0; i < sState.size(); i++) {
    addPiece(state,i,getCode(sState[i]));    
  } 
}

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
    default: assert(false);
  }
  return os.str();

}

// Don't copy over original state
void applyMove(uint16_t* oldState, uint16_t* newState, uint16_t move)
{
  uint16_t from ;
  uint16_t to;
  bool checked;
  bool blocked;
  decodeMove (move, from, to, blocked, checked);
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
  decodeMove (move, from, to, blocked, checked);
  assert (!blocked);
  assert (!checked);
  movePiece(state,from,to);
}

// MDR

void printPiece(uint16_t val)
{
        if(val==0) printf(" ");
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
		printf("ERROR: in removePiece()\n");exit(0);
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
	uint16_t pieceMask=pieceVal;
	pieceMask=pieceMask<<((3-toOffset)*4);
        switch (toOffset) {
		case 0: destMask=4095; break;
		case 1: destMask=61695; break;
		case 2: destMask=65296; break;
		case 3: destMask=65520; break;
		default: assert(false);
	}
	//printf("destMask: %d pieceMask%d\n",destMask,pieceMask);
	state[toElement]=(state[toElement]&destMask)|pieceMask;
	if(getBlockVal(state,fromblock)!=0)
	{
		printf("ERROR: in removePiece()\n");exit(0);
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

bool empty(uint16_t* state, uint16_t block)
{
  return getBlockVal(state,block) == 0;
}

bool ownPiece(uint16_t* state, uint16_t block, bool white)
{
  uint16_t val = getBlockVal(state,block);
  return (white && (val > 0 && val < 7)) || (!white && (val >= 7 && val < 13));  
}

bool opponentPiece(uint16_t* state, uint16_t block, bool white)
{
  uint16_t val = getBlockVal(state,block);
  return (!white && (val > 0 && val < 7)) || (white && (val >= 7 && val < 13));  
}


void findAttemptableMoves(uint16_t* state, bool whiteMove, uint16_t* moves, int& nMoves)
{
  nMoves = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      if (i == j ) continue;
      if (ownPiece(state,i,whiteMove) && !ownPiece(state,j,whiteMove)) {
        moves[nMoves++] = encodeMove(i,j,false,false);
      }
    }
  }
}

void tryAttemptableMoves(uint16_t* state, bool whiteMove, uint16_t* moves, int nMoves)
{
    uint16_t newState[16];
  for (int i = 0; i < nMoves; i++) {
    dispMove(moves[i]);
    applyMove(state,newState,moves[i]);
    printState(newState);
  }
}

void generateMoves(uint16_t* state, bool whiteMove, uint16_t* moveHistory, uint16_t** layers, int depth, int maxdepth)
{
  if (depth == maxdepth) {
	//printState(state);
    cout << "Begin Plausible move sequence: " << depth << endl;
    for (int i = 0; i < depth; i++) {
      cout << "Move #" << i;
      dispMove(moveHistory[i]);
    }
    cout << "End Plausible move sequence: " << depth << endl;
    return;
  }
  int nMoves = 0;
  uint16_t newState[16];
  findAttemptableMoves(state, whiteMove, layers[depth], nMoves);
  assert (nMoves < NMOVES); // 
  for (int i = 0; i < nMoves; i++) {
    for (int j = 0; j < 3*depth; j++) cout << " ";
    dispMove(layers[depth][i]);
    applyMove(state,newState,layers[depth][i]);
    //printState(newState);
    moveHistory[depth] = layers[depth][i];
    generateMoves(newState,!whiteMove,moveHistory,layers,depth+1,maxdepth);
  } 
}

bool isLegal(const uint16_t& move)
{
  static uint16_t mask = 0xC000; // 1100000000000000
  return !(move & mask); // move & mask is non-zero if the move is blocked or would would leave player in check
}

bool hasLegalMove(uint16_t* moves, int nMoves)
{
  for (int i = 0; i < nMoves; i++) {
    if (isLegal(moves[i])) {
      return true;
    }
  }
  return false; // None of the moves was doable
}
void processMoveHistory(uint16_t* state, VecSetMove& failedMoves, uint16_t* moveHistory, int nMoves)
{
  cout << "Moves: " << endl;
  printState(state);
  for (unsigned i = 0; i < nMoves; i++) {
    cout << "Failed moves: " << failedMoves[i].size() << endl;
    uint16_t& move =  moveHistory[i];
    dispMove(move);
    assert(isLegal(move));
    applyMove(state, move);
    printState(state);
  }
}

int generateRandomMoves(uint16_t* state, bool whiteMove, uint16_t* moveHistory, VecSetMove& failedMoves, uint16_t** layers, int depth, int maxdepth)
{
  if (depth == maxdepth) {
	//printState(state);
     return depth;
	//processMoveHistory(failedMoves,moveHistory);
  }
  int nMoves = 0;
  uint16_t newState[16]; 
  findAttemptableMoves(state, whiteMove, layers[depth], nMoves);
  assert (nMoves < NMOVES); // 
  if (!hasLegalMove(layers[depth],nMoves)) return depth; // No legal mvoes from this state
  while (true) { // Keep trying random moves until a legal one is executed
    int attemptedMoveIndex = rand() % nMoves;
    uint16_t& move = layers[depth][attemptedMoveIndex];
    if (isLegal(move)) {
      applyMove(state,newState,move);
      moveHistory[depth] = move;
      return generateRandomMoves(newState,!whiteMove,moveHistory,failedMoves,layers,depth+1,maxdepth);
    } else {
      failedMoves[depth].insert(move); // Note that this illegal move was attempted 
    }
  }
}

int main()
{
/*******************************************************************************************
//	Start config
//	uint16_t state[16]={30874,47495,52428,52428,0,0,0,0,0,0,0,0,26214,26214,4660,21281};
******************************************************************************************/
//	uint16_t state[16]={30874,47495,0,52428,0,0,0,0,0,0,0,0,26214,26214,4660,21281};
//	uint16_t state[16]={30874,47495,52428,52428,0,0,0,0,0,0,0,0,0,26214,4660,21281};
//	uint16_t state[16]={30874,47495,0,0,0,0,0,0,0,0,0,0,26214,26214,4660,21281};
//	uint16_t state[16]={30874,47495,52428,52428,0,0,0,0,0,0,0,0,0,0,4660,21281};
	//uint16_t state[16]={30874,47495,52428,52416,0,0,0,0,0,0,0,12,26214,26214,4660,21281};
	uint16_t attemptableMoves[NMOVES*NLAYERS];
        uint16_t moveHistory[NLAYERS];
	uint16_t* moveList[NLAYERS];
        for (int i = 0; i < NLAYERS; i++) {
		moveList[i] = &attemptableMoves[i*NMOVES];
        }
	int nMovesByLayer[NLAYERS];
        VecSetMove failedMoves(NLAYERS);
	uint16_t state[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        int nMoves = 0;
 	string s = sampleState(1);
        fillBoard(state,s);
	//printState(state);	
	int nExecutedMoves = generateRandomMoves(state,true,moveHistory,failedMoves,moveList,0,NLAYERS);
        processMoveHistory(state,failedMoves,moveHistory,nExecutedMoves);
	//findAttemptableMoves(state,true,attemptableMoves,nMoves);
	//tryAttemptableMoves(state,true,attemptableMoves,nMoves);
	//movePiece(state,0,15);	
	//movePiece(state,63,47);	
	//movePiece(state,47,46);	
	//movePiece(state,60,0);	
	//printState(state);	

/*  Checking pawn kill */
//	uint16_t state[16]={30874,47495,52428,52416,0,0,0,0,0,0,0,12,26214,26214,4660,21281};
/*  Checking Rook kill */
//	uint16_t state[16]={30874,47488,52428,52428,0,0,0,0,0,0,0,7,26214,26214,4660,21281};
/*  Checking Bishop kill */
//	uint16_t state[16]={30874,47495,52428,52428,0,0,0,0,0,0,0,9,26214,26214,4660,21281};
/*  Checking Knight kill */
//  uint16_t state[16]={30874,47495,52428,52428,0,0,0,0,0,0,0,8,26214,26214,4660,21281};
/*  Checking King kill */
//  uint16_t state[16]={30874,47495,52428,52428,0,0,0,0,0,0,0,10,26214,26214,4660,21281};
/*  Checking Queen kill */
//  uint16_t state[16]={30874,47495,52428,52428,0,0,0,0,0,0,0,11,26214,26214,4660,21281};
//	printState(state);
	//move(state,true);
        cout << "Made it this far" << endl;
	return 0;
}
