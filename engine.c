#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
//#include <windows.h>

#include "engine.h"
#include "poll.h"
#include "command.h"

/*#include "gcc_defs.h"*/


extern int move_num, uci;
extern t_Move move_hist[400];
 int Game50;
extern t_Flag flag;

int comp_time;
static DWORD search_time;

t_Piece init_pos_pieces[] = {
	null, null, null, null, null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null, null, null, null, null,
	null, null, rook, knight, bishop, queen, king, bishop, knight, rook, null, null,
	null, null, pawn, pawn, pawn, pawn, pawn, pawn, pawn, pawn, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, pawn, pawn,	pawn, pawn, pawn, pawn, pawn, pawn, null, null,
	null, null, rook, knight, bishop, queen, king, bishop, knight, rook, null, null,
	null, null, null, null, null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null, null, null, null, null
};

t_Color init_pos_color[] = {
	null, null, null, null, null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null, null, null, null, null,
	null, null, white, white, white, white, white, white, white, white, null, null,
	null, null, white, white, white, white, white, white, white, white, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, empty, empty, empty, empty, empty, empty, empty, empty, null, null,
	null, null, black, black, black, black, black, black, black, black, null, null,
	null, null, black, black, black, black, black, black, black, black, null, null,
	null, null, null, null, null, null, null, null, null, null, null, null,
	null, null, null, null, null, null, null, null, null, null, null, null
};

t_SquareIndex init_piece_list_white[] = {30,42,41,40,32,43,44,39,27,28,31,29,
										 26,38,45,33,null};
t_SquareIndex init_piece_list_black[] = {114,102,101,100,116,104,111,99,98,105,103,
										 112,115,113,110,117,null};
t_SquareIndex knight_move_index[] = {25,-25,23,-23,14,-14,10,-10};


/* Transposition table stuff */
BITBOARD from_random_1[64];
BITBOARD from_random_2[64];
BITBOARD to_random_1[64];
BITBOARD to_random_2[64];

TABLE_ENTRY *ptrtoTT[2];
BITBOARD HashKey, BD, hash_mask;

static int piece_val[8] = {0,100,300,320,500,900,30000,0};

#ifndef OLD
static t_SquareIndex sweep[7] = {0, 0, 0, 1, 1, 1, 0};
static t_SquareIndex Dstpwn[3] = {4, 6, 0};
static t_SquareIndex Dstart[7] = {6, 4, 8, 4, 0, 0, 0};
static t_SquareIndex Dstop[7] = {7, 5, 15, 7, 3, 7, 7};

static t_SquareIndex Dir[16] =
	{
	  1,  12,  -1, -12,  11,  13, -11, -13,
	 10, -10,  14, -14,  23, -23,  25, -25
	};
#endif
/*static int init_piece_type_map[2][17] = {
									{pawn,pawn,pawn,knight,pawn,pawn,pawn,knight,bishop,
									 bishop,queen,rook,pawn,pawn,king,rook,null},
									{pawn,pawn,pawn,knight,pawn,knight,pawn,pawn,pawn,pawn,
									 bishop,bishop,queen,rook,king,rook,null}
									};*/

t_SquareIndex map[64]=
   {26,27,28,29,30,31,32,33,
    38,39,40,41,42,43,44,45,
    50,51,52,53,54,55,56,57,
    62,63,64,65,66,67,68,69,
    74,75,76,77,78,79,80,81,
    86,87,88,89,90,91,92,93,
    98,99,100,101,102,103,104,105,
    110,111,112,113,114,115,116,117};

t_SquareIndex unmap[144] = {
							-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
							-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
							-1,-1,0,1,2,3,4,5,6,7,-1,-1,
							-1,-1,8,9,10,11,12,13,14,15,-1,-1,
							-1,-1,16,17,18,19,20,21,22,23,-1,-1,
							-1,-1,24,25,26,27,28,29,30,31,-1,-1,
							-1,-1,32,33,34,35,36,37,38,39,-1,-1,
							-1,-1,40,41,42,43,44,45,46,47,-1,-1,
							-1,-1,48,49,50,51,52,53,54,55,-1,-1,
							-1,-1,56,57,58,59,60,61,62,63,-1,-1,
							-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
							-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
							};

static int center_score[2][64] = {
    {
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,1,2,2,1,0,0,
        0,0,4,6,6,4,0,0,
        0,0,5,8,8,5,0,0,
        0,0,6,7,7,6,0,0,
        0,0,2,4,4,2,0,0,
        0,0,0,0,0,0,0,0
    },
    {
        0,0,0,0,0,0,0,0,
        0,0,2,4,4,2,0,0,
        0,0,6,7,7,6,0,0,
        0,0,5,8,8,5,0,0,
        0,0,4,6,6,4,0,0,
        0,0,1,2,2,1,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0
    }
};
    
static int knight_pos_bonus[2][64] = { { -25,-10,0,0,0,0,-10,-20,
									  -20,-5,0,5,5,0,-5,-20,
									  -10,5,15,5,5,15,5,-10,
									  -5,0,5,20,20,5,0,-5,
									  -5,-5,0,10,10,0,-5,-5,
									  0,0,0,0,0,0,0,0,
									  -10,-10,-10,-10,-10,-10,-10,-10,
									  -20,-20,-20,-20,-20,-20,-20,-20},
									{ -20,-20,-20,-20,-20,-20,-20,-20,
									  -10,-10,-10,-10,-10,-10,-10,-10,
									  0,0,0,0,0,0,0,0,
									  -5,-5,0,10,10,0,-5,-5,
									  -5,0,5,20,20,5,0,-5,
									  -10,5,15,5,5,15,5,-10,
									  -20,-5,0,5,5,0,-5,-20,
									  -25,-10,0,0,0,0,-10,-20 } };

static int bishop_pos_bonus[2][64] = { {
   										10, 5, 0, 0, 0, 0, 5,10,
										10,15,10,10,10,10,15,10,
										10,10,15,10,10,15,10,10,
										10,10,10,15,15,10,10,10,
										10,10,10,15,15,10,10,10,
										10,10,15,10,10,15,10,10,
										10,15,10,10,10,10,15,10,
										10,10,10,10,10,10,10,10
									   },
									 {
   										10,10,10,10,10,10,10,10,
										10,15,10,10,10,10,15,10,
										10,10,15,10,10,15,10,10,
										10,10,10,15,15,10,10,10,
										10,10,10,15,15,10,10,10,
										10,10,15,10,10,15,10,10,
										10,15,10,10,10,10,15,10,
										10, 5, 0, 0, 0, 0, 5,10

									   }
									 };
static int pawn_pos_bonus[2][64] = { {
   										0,0,0,0,0,0,0,0,
										5,5,0,0,0,0,5,5,
										0,0,5,10,10,0,0,0,
										0,0,10,20,20,0,0,0,
										0,0,0,20,20,0,0,0,
										0,0,0,0,0,0,0,0,
										0,0,0,0,0,0,0,0,
										0,0,0,0,0,0,0,0
									   },
									 {
   										0,0,0,0,0,0,0,0,
										0,0,0,0,0,0,0,0,
										0,0,0,0,0,0,0,0,
										0,0,0,20,20,0,0,0,
										0,0,10,20,20,0,0,0,
										0,0,5,10,10,0,0,0,
										5,5,0,0,0,5,5,5,
										0,0,0,0,0,0,0,0

									   }
									 };
static int passed_pawn_bonus[8] = {0,0,0,5,10,25,50,0};

static int king_center_bonus[2][64] = {
								   {0,0,0,0,0,0,0,0,
								    2,2,2,2,2,2,2,2,
									0,2,10,10,10,10,2,0,
									0,5,10,15,15,10,5,0,
									0,5,10,15,15,10,5,0,
									0,5,10,10,10,10,5,0,
									0,5,10,10,10,10,5,0,
									0,5,5,5,5,5,5,0
								   },
								   {0,5,5,5,5,5,5,0,
								    0,5,10,10,10,10,5,0,
									0,5,10,10,10,10,5,0,
									0,5,10,15,15,10,5,0,
									0,5,10,15,15,10,5,0,
									0,2,10,10,10,10,2,0,
									2,2,2,2,2,2,2,2,
									0,0,0,0,0,0,0,0
								   }
								};
static int piece_index_map[144];
int material_score;
int material[2];
#ifdef ARRAY_PIECELIST
static t_SquareIndex piece_list[2][17];
#else
t_SquareIndex **piece_list;
#endif
static int king_moved[MAX_MOVES+1][2];
static int rook_ks_moved[MAX_MOVES+1][2];
static int rook_qs_moved[MAX_MOVES+1][2];
static int castled[2];
t_SquareIndex **enpassant_square;

static int svalue[144];
t_SquareIndex attack[2][144];
int num_attackers[2][144];
t_SquareIndex history[144][144];
int pawn_struct[2][8];

t_Move killer[MAX_MOVES], killer2[MAX_MOVES], killer3[MAX_MOVES];
static t_Move PV[MAX_PVSIZE];
static int searching_pv;
static int try_tt_move;
static t_Move tt_move;
int nodes, quiescense_nodes, cutoffs, tt_hits, tt_probes;

t_Turn computer_player;

int RepetitionCheck(int depth)
{
	register short i,f,t,count;
	short rep, b[144];
	//t_Move m;
	signed char type;
	memset(b,0,144*sizeof(short));
	rep = count = 0;
	for (i = move_num+depth-2; i >= Game50-1; i--)
    {
      f = move_hist[i].from; t = move_hist[i].to;
	  type = move_hist[i].type;
	  /*printf("%d: %d %d\n",i,f,t);*/
      if (type != short_castle && type != long_castle)
        {
          b[f]++; b[t]--;
          if (b[f] == 0) count--; else count++;
          if (b[t] == 0) count--; else count++;
          if (count == 0) rep++;
        }
    }
	return rep;
}

void PostPV(int depth,int best,int elapsed_time,t_Move best_line[],int post_stats)
{
	int p;
#ifdef XBOARD

	printf("%d %d %d %d ",depth,best,(int)(elapsed_time/10),nodes);
	for (p=0;best_line[p].type != null;p++) {
		printf("%c%d-%c%d ",'a'+rank(best_line[p].from)-1,file(best_line[p].from),
				'a'+rank(best_line[p].to)-1,file(best_line[p].to));
	}
	printf("\n");
#else
	if (!post_stats) {
		printf("%d:",depth);
		for (p=0;best_line[p].type != null;p++) {
			printf("%c%d-%c%d ",'a'+rank(best_line[p].from)-1,file(best_line[p].from),
				'a'+rank(best_line[p].to)-1,file(best_line[p].to));
		}
		printf("\n");
	}
	if (post_stats) {
		printf("Score: %d Total nodes searched: %d Elapsed time: %.2fs nps: %d\n",best,nodes,elapsed_time/1000.0,(int)(nodes/((float)elapsed_time/1000.)));
		printf("Quiescense nodes searched: %d TT probes: %d TT hits: %d cutoffs: %d\n",quiescense_nodes,tt_probes,tt_hits,cutoffs);
	}
#endif
}
#ifdef OLD

void PlaceAttacker(t_Board pos, t_Turn side, t_SquareIndex *a)
{
}

#else
void PlaceAttacker(t_Board pos, t_Turn side, t_SquareIndex *a)
{
	t_Piece piece;
	int i, d, j;
	t_SquareIndex m, m0, *sqptr, u;
	int *s;

	memset(a, 0, 144 * sizeof(t_SquareIndex));
	memset(num_attackers[side],0,144*sizeof(int));
	Dstart[pawn] = Dstpwn[side];
	Dstop[pawn] = Dstart[pawn] + 1;
	sqptr = &piece_list[side][0];
	for (i = 0; piece_list[side][i] != null; i++) {
		piece = pos[*sqptr].piece;

		m0 = *sqptr;
		s = &svalue[*sqptr];
		*s = 0;
		sqptr++;
		if (piece == empty) continue;
		if (sweep[piece]) {
			for (j = Dstart[piece]; j <= Dstop[piece]; j++) {
				d = Dir[j];
				m = m0 + d;
				u = unmap[m];
				while (u >= 0) {
					*s += 2;

					num_attackers[side][m]++;

					if ((a[m] == 0) || (piece < a[m]))
						a[m] = piece;

					if (pos[m].color == empty) {
						m += d;
						u = unmap[m];
					}
					else
						u = -1;
				}
			}
		}
		else {
			for (j = Dstart[piece]; j <= Dstop[piece]; j++) {
				m = m0 + Dir[j];
				if ((u = unmap[m]) >= 0) {
					num_attackers[side][m]++;
					if ((a[m] == 0) || (piece < a[m])) {
						a[m] = piece;
					}
				}
			}
		};
	};

	return;
}
#endif

int Comp(const void *move1, const void *move2)
{
	if (((t_Move*)move1)->score > ((t_Move*)move2)->score) return -1;
	if (((t_Move*)move1)->score == ((t_Move*)move2)->score) return 0;

	return 1;
}

/***************
*
* IsInCheck returns true if the current turn's king is in check.
* If not it returns false.
*
****************/
#ifdef OLD
int IsInCheck(t_Board pos,t_Turn turn)
{
	signed char sign;
	t_SquareIndex to;
	int i, j, d;
	//t_Piece piece;
	t_Turn other_turn = other(turn);
	t_SquareIndex square;

	square = piece_list[turn][0]; /* king's square */
	/*printf("king pos:%d\n",square);*/
	sign = (turn == white ? 1 : -1);

	/* Check for pawn attacks on the king */
	to = square-11*sign;
	/*printf("to:%d pos[to].color:%d\n",to,pos[to].color); getchar();*/
	if (pos[to].color == other_turn && pos[to].piece == pawn) return true;
	to = square-13*sign;
	if (pos[to].color == other_turn && pos[to].piece == pawn) return true;

	/* Check for knight attacks on the king */
	for (i=0;i<8;i++) {
		to = square+knight_move_index[i];
		if (pos[to].color == other_turn && pos[to].piece == knight) return true;
	}

	/* Check for bishop-like attacks on the king (bishops and queen) */
	for (i=-1;i<=1;i+=2)
		for (j=-1;j<=1;j+=2)
			for (d=1;;d++) {
				to = square+d*(12*i+j);
				if (pos[to].color == other_turn && (pos[to].piece == bishop ||
					pos[to].piece == queen)) return true;
				else if (pos[to].color != empty)
					break;
			}

	/* Check for rook-like attacks on the king (rooks and queen) */
	for (i=-1;i<=1;i+=2) {
		for (d=1;;d++) {
			to = square+d*i;
			if (pos[to].color == other_turn && (pos[to].piece == rook ||
				pos[to].piece == queen)) return true;
			else if (pos[to].color != empty)
				break;
		}
		for (d=1;;d++) {
			to = square+12*d*i;
			if (pos[to].color == other_turn && (pos[to].piece == rook ||
				pos[to].piece == queen)) return true;
			else if (pos[to].color != empty)
				break;
		}
	}
	return false;
}
#else

int IsInCheck(t_Board pos, t_Turn turn)
{
	if (attack[other(turn)][piece_list[turn][0]] > 0) return true;

	return false;
}

#endif

/***************
*
* IsKingAttacked returns true if the current turn's king is attacked.
* If not it returns false. It defers from IsInCheck in the fact that the king
* can be attacked by the other king in an illegal possition. This function is used to
* determine if the position of the king is legal.
*
****************/
#ifdef OLD
int IsKingAttacked(t_Board pos,t_Turn turn)
{
	signed char sign;
	t_SquareIndex to;
	int i, j, d;
	//t_Color color;
	//t_Piece piece;
	t_Turn other_turn = other(turn);
	t_SquareIndex square;

	square = piece_list[turn][0]; /* king's square */
	/*printf("king pos:%d\n",square);*/
	sign = (turn == white ? 1 : -1);

	/* Check for pawn attacks on the king */
	to = square-11*sign;
	/*printf("to:%d pos[to].color:%d\n",to,pos[to].color); getchar();*/
	if (pos[to].color == other_turn && pos[to].piece == pawn) return true;
	to = square-13*sign;
	if (pos[to].color == other_turn && pos[to].piece == pawn) return true;

	/* Check for knight attacks on the king */
	for (i=0;i<8;i++) {
		to = square+knight_move_index[i];
		if (pos[to].color == other_turn && pos[to].piece == knight) return true;
	}
	/* Check for bishop-like attacks on the king (bishops and queen) */
	for (i=-1;i<=1;i+=2)
		for (j=-1;j<=1;j+=2)
			for (d=1;;d++) {
				to = square+d*(12*i+j);
				if (pos[to].color == other_turn && (pos[to].piece == bishop ||
					pos[to].piece == queen || (d == 1 && pos[to].piece == king))) return true;
				else if (pos[to].color != empty)
					break;
			}

	/* Check for rook-like attacks on the king (rooks and queen) */
	for (i=-1;i<=1;i+=2) {
		for (d=1;;d++) {
			to = square+d*i;
			if (pos[to].color == other_turn && (pos[to].piece == rook ||
				pos[to].piece == queen || (d == 1 && pos[to].piece == king))) return true;
			else if (pos[to].color != empty)
				break;
		}
		for (d=1;;d++) {
			to = square+12*d*i;
			if (pos[to].color == other_turn && (pos[to].piece == rook ||
				pos[to].piece == queen || (d == 1 && pos[to].piece == king))) return true;
			else if (pos[to].color != empty)
				break;
		}
	}
	return false;
}
#else
int IsKingAttacked(t_Board pos, t_Turn turn)
{
	if (attack[other(turn)][piece_list[turn][0]] > 0) return true;

	return false;
}
#endif

int LookupTT(int *depth, t_Move *move, int *score,int *upperbound, int *lowerbound, int *type, int player)
{

	TABLE_ENTRY *temp;
	temp = ptrtoTT[player]+(HashKey & hash_mask);
	if (temp->flag) {
		/*while (temp->bd != BD) {
			if ((temp = temp->next) == NULL) return 0;
		}*/
		/*if (temp->bd != BD || temp->hash_key != HashKey) return 0;*/
		/*tt_hits++;*/
		if (temp->bd == BD && temp->hash_key == HashKey) {
			*depth = temp->depth;
			*score = temp->score;
			*upperbound = temp->upperbound;
			*lowerbound = temp->lowerbound;
			*type = temp->type;
			move->from = temp->from;
			move->to = temp->to;
			return true;
		}
		else return false;
		/*for (i=0;i<SIZE;i++) {
			for (j=0;j<SIZE;j++) {
			printf("%d ",(Board[j][i]==EMPTY?5:Board[j][i]));
			}
			printf("\n");
		}
		printf("move: %d %d\n",move->movex,move->movey);
		printf("score: %d\n",*score);
		printf("position: %ld\n",HashKey & hash_mask);
		getchar();*/
		/*sprintf(str,"move:%d %d, tdepth:%u, score:%d",move->movex,move->movey,*depth,*score);
		ShowMessage(str);*/
	}
	return (false);
}

int StoreInTT(int depth, t_Move move, int score,int upperbound, int lowerbound, int type, int player)
{
	TABLE_ENTRY *temp;

	temp = ptrtoTT[player]+(HashKey & hash_mask);
	/*while (temp != NULL) {
		if (temp->bd == BD) {
			temp->depth = depth;
			temp->from = move.from;
			temp->to = move.to;
			temp->score = score;
			return true;
		}
		if ( (temp->next = (TABLE_ENTRY *)malloc(sizeof(TABLE_ENTRY))) == NULL)
			return false;
		temp = temp->next;
	}*/


	temp->depth = depth;
	temp->from = move.from;
	temp->to = move.to;
	temp->score = score;
	temp->upperbound = upperbound;
	temp->lowerbound = lowerbound;
	temp->bd = BD;
	temp->hash_key = HashKey;
	temp->flag = true;
	temp->type = type;
	return true;

}

void ClearTT(void)
{
	int i;
#ifndef XBOARD
	printf("Clearing transposition table\n");
#endif
	for (i=0;i<TTSIZE;i++) {
		(ptrtoTT[white]+i)->flag=0;
		(ptrtoTT[black]+i)->flag=0;
		/*(ptrtoTT[0]+i)->depth=0;
		(ptrtoTT[0]+i)->score=0;
		(ptrtoTT[0]+i)->move=0;
		(ptrtoTT[0]+i)->flag=0;
		(ptrtoTT[0]+i)->bd=0;
		(ptrtoTT[1]+i)->depth=0;
		(ptrtoTT[1]+i)->score=0;
		(ptrtoTT[1]+i)->move=0;
		(ptrtoTT[1]+i)->flag=0;
		(ptrtoTT[1]+i)->bd=0;
		(ptrtoTT[0]+i)->hash_key=0;
		(ptrtoTT[1]+1)->hash_key=0;*/
		/*(ptrtoTT+i)->depth=0;
		(ptrtoTT+i)->score=0;*/
		/*(ptrtoTT+i)->upperbound=0;
		(ptrtoTT+i)->lowerbound=0;*/
		/*(ptrtoTT+i)->move=0;
		(ptrtoTT+i)->flag=0;
		(ptrtoTT+i)->bd = 0;*/
	}
#ifndef XBOARD
	printf("done\n");
#endif
}

/********************
*
*	GeneratePawnMove generates all possible moves for a given pawn
*	except captures and En Passant captures. It places the moves in
*	the move_list array
*
*********************/
void GeneratePawnMoves(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	char sign;
	int attacked;
	t_SquareIndex to;
	t_SquareIndex u_to, u_from;
	t_Turn other_turn = other(turn);
	sign = (turn == white ? 1 : -1);

	to = square+12*sign;
	u_to = unmap[to];
	u_from = unmap[square];

	if (pos[to].color == empty && file(to) != (turn == white ? 8 : 1)) {
		move_list[*num_moves].from = square;
		move_list[*num_moves].to = to;
		move_list[*num_moves].captured_piece = null;
		move_list[*num_moves].type = normal;

		if (searching_pv && PV[ply].from == square && PV[ply].to == to)
			move_list[*num_moves].score = PV_SCORE;
		else if (try_tt_move && tt_move.from == square && tt_move.to == to)
			move_list[*num_moves].score = TTMOVE_SCORE;
		else {
			move_list[*num_moves].score = pawn_pos_bonus[turn][u_to]-pawn_pos_bonus[turn][u_from];
			if (num_attackers[other_turn][to] > num_attackers[turn][to])
				move_list[*num_moves].score = -piece_val[pawn];
			if (killer[depth].type != null) {
				if (killer[depth].from == square && killer[depth].to == to)
					move_list[*num_moves].score = KILLERMV1_SCORE;
			}
			else if (killer2[depth].type != null) {
				if (killer2[depth].from == square && killer2[depth].to == to)
					move_list[*num_moves].score = KILLERMV2_SCORE;
			}
			else if (killer3[depth].type != null) {
				if (killer3[depth].from == square && killer3[depth].to == to)
					move_list[*num_moves].score = KILLERMV3_SCORE;
			}
			if (history[square][to])
				move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
		}
		if (incheck) {
			MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
			PlaceAttacker(pos,other_turn,attack[other_turn]);
			attacked = IsKingAttacked(pos,turn);
			UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
			if (!attacked)
				(*num_moves)++;
		}
		else
			(*num_moves)++;
		to = square+24*sign;
		u_to = unmap[to];
		if (file(square) == (turn == white ? 2 : 7) && pos[to].color == empty) {
			move_list[*num_moves].from = square;
			move_list[*num_moves].to = to;
			move_list[*num_moves].captured_piece = null;
			move_list[*num_moves].type = normal;
			if (searching_pv && PV[ply].from == square && PV[ply].to == to)
				move_list[*num_moves].score = PV_SCORE;
			else if (try_tt_move && tt_move.from == square && tt_move.to == to)
				move_list[*num_moves].score = TTMOVE_SCORE;
			else {
				move_list[*num_moves].score = pawn_pos_bonus[turn][u_to]-pawn_pos_bonus[turn][u_from];
				if (num_attackers[other_turn][to] > num_attackers[turn][to])
				move_list[*num_moves].score = -piece_val[pawn];
				if (killer[depth].type != null) {
					if (killer[depth].from == square && killer[depth].to == to)
						move_list[*num_moves].score += KILLERMV1_SCORE;
				}
				else if (killer2[depth].type != null) {
					if (killer2[depth].from == square && killer2[depth].to == to)
						move_list[*num_moves].score += KILLERMV2_SCORE;
				}
				else if (killer3[depth].type != null) {
					if (killer3[depth].from == square && killer3[depth].to == to)
						move_list[*num_moves].score += KILLERMV3_SCORE;
				}
				if (history[square][to])
					move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
			}

			if (incheck) {
				MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				if (!attacked)
					(*num_moves)++;
			}
			else
				(*num_moves)++;
		}
	}
}
/********************
*
*	GeneratePawnCaptures generates all possible captures for a given pawn
*	except En Passant captures. It places the moves in the move_list array
*
*********************/
void GeneratePawnCaptures(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	signed char sign;
	t_SquareIndex to;
	int attacked;
	t_Turn other_turn = other(turn);
	t_SquareIndex u_to, u_from;

	sign = (turn == white ? 1 : -1);
	to = square+11*sign;
	u_to = unmap[to];
	u_from = unmap[square];

	if (file(square) != (turn == white ? 7 : 2) && pos[to].color == other(turn)) {
		move_list[*num_moves].from = square;
		move_list[*num_moves].to = square+11*sign;
		move_list[*num_moves].captured_piece = pos[to].piece;
		move_list[*num_moves].type = capture;
		if (searching_pv && PV[ply].from == square && PV[ply].to == to)
			move_list[*num_moves].score = PV_SCORE;
		else if (try_tt_move && tt_move.from == square && tt_move.to == to)
			move_list[*num_moves].score = TTMOVE_SCORE;
		else {
			move_list[*num_moves].score = 10+piece_val[pos[to].piece]
			+pawn_pos_bonus[turn][u_to]-pawn_pos_bonus[turn][u_from];
			if (killer[depth].type != null) {
				if (killer[depth].from == square && killer[depth].to == to)
					move_list[*num_moves].score += KILLERMV1_SCORE;
			}
			else if (killer2[depth].type != null) {
				if (killer2[depth].from == square && killer2[depth].to == to)
					move_list[*num_moves].score += KILLERMV2_SCORE;
			}
			else if (killer3[depth].type != null) {
				if (killer3[depth].from == square && killer3[depth].to == to)
					move_list[*num_moves].score += KILLERMV3_SCORE;
			}
			else if (history[square][to])
				move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
		}

		if (incheck) {
			MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
			PlaceAttacker(pos,other_turn,attack[other_turn]);
			attacked = IsKingAttacked(pos,turn);
			UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
			if (!attacked)
				(*num_moves)++;
		}
		else
			(*num_moves)++;
	}
	to = square+13*sign;
	if (pos[to].color == other(turn)) {
		move_list[*num_moves].from = square;
		move_list[*num_moves].to = square+13*sign;
		move_list[*num_moves].captured_piece = pos[to].piece;
		move_list[*num_moves].type = capture;
		if (searching_pv && PV[ply].from == square && PV[ply].to == to)
			move_list[*num_moves].score = PV_SCORE;
		else if (try_tt_move && tt_move.from == square && tt_move.to == to)
			move_list[*num_moves].score = TTMOVE_SCORE;
		else {
			move_list[*num_moves].score = piece_val[pos[to].piece]+
			pawn_pos_bonus[turn][u_to]-pawn_pos_bonus[turn][u_from];

			if (killer[depth].type != null) {
				if (killer[depth].from == square && killer[depth].to == to)
					move_list[*num_moves].score += KILLERMV1_SCORE;
			}
			else if (killer2[depth].type != null) {
				if (killer2[depth].from == square && killer2[depth].to == to)
					move_list[*num_moves].score += KILLERMV2_SCORE;
			}
			else if (killer3[depth].type != null) {
				if (killer3[depth].from == square && killer3[depth].to == to)
					move_list[*num_moves].score += KILLERMV3_SCORE;
			}
			else if (history[square][to])
				move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
		}

		if (incheck) {
			MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
			PlaceAttacker(pos,other_turn,attack[other_turn]);
			attacked = IsKingAttacked(pos,turn);
			UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
			if (!attacked)
				(*num_moves)++;
		}
		else
			(*num_moves)++;
	}
}

/********************
*
*	GenerateEnPassantCaptures generates any possible enpassant pawn capture wn
*	for a given pawn. It places the move in the move_list array
*
*********************/
void GenerateEnPassantCaptures(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	signed char sign;
	t_SquareIndex to;
	int attacked, i;
	t_Turn other_turn = other(turn);
	t_SquareIndex u_to, u_from;


	u_from = unmap[square];

	sign = (turn == white ? 1 : -1);

	for (i=11;i<=13;i+=2) {
		to = square+i*sign;
		u_to = unmap[to];
		if (to == enpassant_square[depth][turn]) {
			move_list[*num_moves].from = square;
	  		move_list[*num_moves].to = to;
			move_list[*num_moves].type = enpassant;
			move_list[*num_moves].captured_piece = null;
			if (searching_pv && PV[ply].from == square && PV[ply].to == to)
				move_list[*num_moves].score = PV_SCORE;
			else if (try_tt_move && tt_move.from == square && tt_move.to == to)
				move_list[*num_moves].score = TTMOVE_SCORE;
			else {
				move_list[*num_moves].score = 10+piece_val[pawn]+pawn_pos_bonus[turn][u_to]-pawn_pos_bonus[turn][u_from];
				if (killer[depth].type != null) {
					if (killer[depth].from == square && killer[depth].to == to)
						move_list[*num_moves].score += KILLERMV1_SCORE;
				}
				else if (killer2[depth].type != null) {
					if (killer2[depth].from == square && killer2[depth].to == to)
						move_list[*num_moves].score += KILLERMV2_SCORE;
				}
				else if (killer3[depth].type != null) {
					if (killer3[depth].from == square && killer3[depth].to == to)
						move_list[*num_moves].score += KILLERMV3_SCORE;
				}
				else if (history[square][to])
					move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
			}
			if (incheck) {
				MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				if (!attacked)
					(*num_moves)++;
			}
			else (*num_moves)++;
		}

	}

}

/********************
*
*	GeneratePromotions generates all possible promotion moves for a given pawn.
*   It places the moves in the move_list array
*
*********************/
void GeneratePromotions(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	char sign;
	int attacked;
	t_SquareIndex i, to, prom_from_file, prom_to_file;
	t_SquareIndex u_to, u_from;
	t_Piece prom_piece;
	t_Turn other_turn = other(turn);

	sign = (turn == white ? 1 : -1);
	prom_from_file = (turn == white ? 7 : 2);
	prom_to_file = (turn == white ? 8 : 1);

	if (file(square) != prom_from_file) return;

	for (i=11;i<=13;i+=2) {
		to = square+i*sign;
		u_to = unmap[to];
		u_from = unmap[square];

		if (pos[to].color == other_turn && file(to) == prom_to_file) {
			for (prom_piece = queen; prom_piece >= knight; prom_piece--) {
				move_list[*num_moves].from = square;
				move_list[*num_moves].to = to;
				move_list[*num_moves].captured_piece = pos[to].piece;
				move_list[*num_moves].type = promotion;
				move_list[*num_moves].prom_piece = prom_piece;
				if (searching_pv && PV[ply].from == square && PV[ply].to == to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = piece_val[pos[to].piece]+piece_val[prom_piece]-piece_val[pawn];

					if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][to])
						move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
				}

				if (incheck) {
					MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					PlaceAttacker(pos,other_turn,attack[other_turn]);
					attacked = IsKingAttacked(pos,turn);
					UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					if (!attacked) {
						(*num_moves)++;
					}
				}
				else {
					(*num_moves)++;
				}

			}
		}
	}
	to = square+12*sign;
	u_to = unmap[to];
	u_from = unmap[square];

	if (pos[to].color == empty && file(to) == prom_to_file) {
		for (prom_piece = queen; prom_piece >= knight; prom_piece--) {

			move_list[*num_moves].from = square;
			move_list[*num_moves].to = to;
			move_list[*num_moves].captured_piece = null;
			move_list[*num_moves].type = promotion;
			move_list[*num_moves].prom_piece = prom_piece;
			if (searching_pv && PV[ply].from == square && PV[ply].to == to)
				move_list[*num_moves].score = PV_SCORE;
			else if (try_tt_move && tt_move.from == square && tt_move.to == to)
				move_list[*num_moves].score = TTMOVE_SCORE;
			else {
				move_list[*num_moves].score = piece_val[prom_piece]-piece_val[pawn];
				if (killer[depth].type != null) {
					if (killer[depth].from == square && killer[depth].to == to)
						move_list[*num_moves].score += KILLERMV1_SCORE;
				}
				else if (killer2[depth].type != null) {
					if (killer2[depth].from == square && killer2[depth].to == to)
						move_list[*num_moves].score += KILLERMV2_SCORE;
				}
				else if (killer3[depth].type != null) {
					if (killer3[depth].from == square && killer3[depth].to == to)
						move_list[*num_moves].score += KILLERMV3_SCORE;
				}
				else if (history[square][to])
					move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
			}

			if (incheck) {
				MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				if (!attacked)
					(*num_moves)++;
			}
			else
				(*num_moves)++;

		}
	}
}

/********************
*
*	GenerateKnightMoves generates all possible moves for a given knight
*	except captures. It places the moves in the move_list array
*
*********************/
void GenerateKnightMoves(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	int i, attacked;
	t_SquareIndex move_index, to;
	t_SquareIndex u_to, u_from;
	t_Turn other_turn = other(turn);

	u_from = unmap[square];

	/* Scan possible moves in the knight_move_index array and check if resulting
	   moves are good. Moves may not be good if i) the destination square
	   is occupied by an own piece (illegal move), ii) the destination
	   square is occupied by an opponent piece (move is a capture),
	   iii) the destination square is out of the board and therefor
	   it's value is null. All three possibilities are discarded by the
	   fact that the destination square is not empty.
	*/
	for (i=0;i<8;i++) {
		move_index = knight_move_index[i];
		to = square+move_index;
		u_to = unmap[to];
		u_from = unmap[square];
		if (pos[square+move_index].color == empty) {
			move_list[*num_moves].from = square;
			move_list[*num_moves].to = to;
			move_list[*num_moves].captured_piece = null;
			move_list[*num_moves].type = normal;
			if (searching_pv && PV[ply].from == square && PV[ply].to == to)
				move_list[*num_moves].score = PV_SCORE;
			else if (try_tt_move && tt_move.from == square && tt_move.to == to)
				move_list[*num_moves].score = TTMOVE_SCORE;
			else {
				move_list[*num_moves].score = knight_pos_bonus[turn][u_to]-knight_pos_bonus[turn][u_from];
				if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
					move_list[*num_moves].score = -piece_val[knight];
				else if (killer[depth].type != null) {
					if (killer[depth].from == square && killer[depth].to == to)
						move_list[*num_moves].score += KILLERMV1_SCORE;
				}
				else if (killer2[depth].type != null) {
					if (killer2[depth].from == square && killer2[depth].to == to)
						move_list[*num_moves].score += KILLERMV2_SCORE;
				}
				else if (killer3[depth].type != null) {
					if (killer3[depth].from == square && killer3[depth].to == to)
						move_list[*num_moves].score += KILLERMV3_SCORE;
				}
				else if (history[square][to])
					move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
			}

			if (incheck) {
				MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				if (!attacked)
					(*num_moves)++;
			}
			else
				(*num_moves)++;
		}
	}
}

/********************
*
*	GenerateKnightCaptures generates all possible capture moves for a given knight
*	It places the moves in the move_list array.
*
*********************/
void GenerateKnightCaptures(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	int i, attacked;
	t_SquareIndex to;
	t_Turn other_turn = other(turn);

	/* Scan possible moves in the knight_move_index array and check if resulting
	   moves are captures, that is if the destination square is occupied by an
	   opponent piece, and the square is not out of the board.
	*/
	for (i=0;i<8;i++) {
		to = square+knight_move_index[i];
		if (pos[to].color == other(turn)) {
			move_list[*num_moves].from = square;
			move_list[*num_moves].to = to;
			move_list[*num_moves].captured_piece = pos[to].piece;
			move_list[*num_moves].type = capture;
			if (searching_pv && PV[ply].from == square && PV[ply].to == to)
				move_list[*num_moves].score = PV_SCORE;
			else if (try_tt_move && tt_move.from == square && tt_move.to == to)
				move_list[*num_moves].score = TTMOVE_SCORE;
			else {
				move_list[*num_moves].score = 10+piece_val[pos[to].piece];

				if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
					move_list[*num_moves].score -= piece_val[knight];
				if (killer[depth].type != null) {
					if (killer[depth].from == square && killer[depth].to == to)
						move_list[*num_moves].score += KILLERMV1_SCORE;
				}
				else if (killer2[depth].type != null) {
					if (killer2[depth].from == square && killer2[depth].to == to)
						move_list[*num_moves].score += KILLERMV2_SCORE;
				}
				else if (killer3[depth].type != null) {
					if (killer3[depth].from == square && killer3[depth].to == to)
						move_list[*num_moves].score += KILLERMV3_SCORE;
				}
				else if (history[square][to])
					move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
			}
			if (incheck) {
				MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
				if (!attacked)
					(*num_moves)++;
			}
			else
				(*num_moves)++;
		}
	}
}

/********************
*
*	GenerateBishopMoves generates all possible moves for a given bishop
*	except captures. It places the moves in the move_list array
*
*********************/
void GenerateBishopMoves(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	t_SquareIndex i, dx, dy, to;
	int attacked;
	t_SquareIndex u_to, u_from;
	t_Turn other_turn = other(turn);

	u_from = unmap[square];

	/* Look at the squares diagonal to the piece square and check if they
	   are occupied. If not, there is legal non capturing bishop-like move
	   to that square. Then continue looking at the diagonal one square
	   further. If the square is occupied stop looking in that direction and
	   look for another */
	for (dx=-1;dx<=1;dx+=2)
		for (dy=-1;dy<=1;dy+=2)
			for (i=1;;i++) {
				to = square+i*(dx+12*dy);
				u_to = unmap[to];
				if (pos[to].color == empty) {
					move_list[*num_moves].from = square;
					move_list[*num_moves].to = to;
					move_list[*num_moves].captured_piece = null;
					move_list[*num_moves].type = normal;
					if (searching_pv && PV[ply].from == square && PV[ply].to == to)
						move_list[*num_moves].score = PV_SCORE;
					else if (try_tt_move && tt_move.from == square && tt_move.to == to)
						move_list[*num_moves].score = TTMOVE_SCORE;
					else {
						move_list[*num_moves].score = bishop_pos_bonus[turn][u_to]-bishop_pos_bonus[turn][u_from];
						if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
							move_list[*num_moves].score -= piece_val[pos[square].piece];
						else if (killer[depth].type != null) {
							if (killer[depth].from == square && killer[depth].to == to)
								move_list[*num_moves].score += KILLERMV1_SCORE;
						}
						else if (killer2[depth].type != null) {
							if (killer2[depth].from == square && killer2[depth].to == to)
								move_list[*num_moves].score += KILLERMV2_SCORE;
						}
						else if (killer3[depth].type != null) {
							if (killer3[depth].from == square && killer3[depth].to == to)
								move_list[*num_moves].score += KILLERMV3_SCORE;
						}
						else if (history[square][to])
							move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
					}

					if (incheck) {
						MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
						PlaceAttacker(pos,other_turn,attack[other_turn]);
						attacked = IsKingAttacked(pos,turn);
						UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
						if (!attacked)
							(*num_moves)++;
					}
					else
						(*num_moves)++;
				}
				else
					break;
			}

}

/********************
*
*	GenerateBishopCaptures generates all possible capture moves for a given bishop
*	It places the moves in the move_list array.
*
*********************/
void GenerateBishopCaptures(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	t_SquareIndex i, dx, dy, to;
	int attacked;
	t_SquareIndex u_to, u_from;
	t_Turn other_turn = other(turn);

	u_from = unmap[square];
	/* Same as GenerateBishopMoves but now we see if the square being looked at
	   is occupied by an opponent piece, so we generate a bishop capture move */

	for (dx=-1;dx<=1;dx+=2)
		for (dy=-1;dy<=1;dy+=2)
			for (i=1;;i++) {
				to = square+i*(dx+12*dy);
				u_to = unmap[to];
				if (pos[to].color == other(turn)) {
					move_list[*num_moves].from = square;
					move_list[*num_moves].to = to;
					move_list[*num_moves].captured_piece = pos[to].piece;
					move_list[*num_moves].type = capture;
					if (searching_pv && PV[ply].from == square && PV[ply].to == to)
						move_list[*num_moves].score = PV_SCORE;
					else if (try_tt_move && tt_move.from == square && tt_move.to == to)
						move_list[*num_moves].score = TTMOVE_SCORE;
					else {
						move_list[*num_moves].score = 10+piece_val[pos[to].piece]+
						bishop_pos_bonus[turn][u_to]-bishop_pos_bonus[turn][u_from];
						if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
							move_list[*num_moves].score -= piece_val[pos[square].piece];

						if (killer[depth].type != null) {
							if (killer[depth].from == square && killer[depth].to == to)
								move_list[*num_moves].score += KILLERMV1_SCORE;
						}
						else if (killer2[depth].type != null) {
							if (killer2[depth].from == square && killer2[depth].to == to)
								move_list[*num_moves].score += KILLERMV2_SCORE;
						}
						else if (killer3[depth].type != null) {
							if (killer3[depth].from == square && killer3[depth].to == to)
								move_list[*num_moves].score += KILLERMV3_SCORE;
						}
						else if (history[square][to])
							move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
					}
					if (incheck) {
						MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
						PlaceAttacker(pos,other_turn,attack[other_turn]);
						attacked = IsKingAttacked(pos,turn);
						UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
						if (!attacked)
							(*num_moves)++;
					}
					else
						(*num_moves)++;
					break;
				}
				else if (pos[to].color != empty)
					break;
			}

}

/********************
*
*	GenerateRookMoves generates all possible moves for a given rook
*	except captures. It places the moves in the move_list array
*
*********************/
void GenerateRookMoves(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	t_SquareIndex i, d, to;
	int attacked;
	t_Turn other_turn = other(turn);

	/* First check for possible rook moves in the same rank */
	for (d=-1;d<=1;d+=2)
 		for (i=1;;i++) {
			to = square+12*d*i;
			if (pos[to].color == empty) {
				move_list[*num_moves].from = square;
				move_list[*num_moves].to = to;
				move_list[*num_moves].captured_piece = null;
				move_list[*num_moves].type = normal;
				if (searching_pv && PV[ply].from == square && PV[ply].to == to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = 0;
					if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
						move_list[*num_moves].score -= piece_val[pos[square].piece];

					if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][to])
						move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
				}
				if (incheck) {
					MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					PlaceAttacker(pos,other_turn,attack[other_turn]);
					attacked = IsKingAttacked(pos,turn);
					UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					if (!attacked)
						(*num_moves)++;
				}
				else
					(*num_moves)++;

			}
			else
				break;
		}

	/* Now check for possible rook moves in the same file */
	for (d=-1;d<=1;d+=2)
 		for (i=1;;i++) {
			to = square+d*i;
			if (pos[to].color == empty) {
				move_list[*num_moves].from = square;
				move_list[*num_moves].to = to;
				move_list[*num_moves].captured_piece = null;
				move_list[*num_moves].type = normal;
				if (searching_pv && PV[ply].from == square && PV[ply].to == to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = 0;
					if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
						move_list[*num_moves].score -= piece_val[pos[square].piece];
					if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == to)
						move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][to])
						move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
				}
				if (incheck) {
					MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					PlaceAttacker(pos,other_turn,attack[other_turn]);
					attacked = IsKingAttacked(pos,turn);
					UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					if (!attacked)
						(*num_moves)++;
				}
				else
					(*num_moves)++;


			}
			else
				break;
		}

}

/********************
*
*	GenerateRookCaptures generates all possible capture moves for a given rook
*	It places the moves in the move_list array.
*
*********************/
void GenerateRookCaptures(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	t_SquareIndex i, d, to;
	int attacked;
	t_Turn other_turn = other(turn);

	/* First check for possible rook capture moves in the same rank */
	for (d=-1;d<=1;d+=2)
 		for (i=1;;i++) {
			to = square+12*d*i;
			if (pos[to].color == other(turn)) {
				move_list[*num_moves].from = square;
				move_list[*num_moves].to = to;
				move_list[*num_moves].captured_piece = pos[to].piece;
				move_list[*num_moves].type = capture;
				if (searching_pv && PV[ply].from == square && PV[ply].to == to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = 5+piece_val[pos[to].piece];
					if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
						move_list[*num_moves].score -= piece_val[pos[square].piece];
					else if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][to])
						move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
				}
				if (incheck) {
					MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					PlaceAttacker(pos,other_turn,attack[other_turn]);
					attacked = IsKingAttacked(pos,turn);
					UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					if (!attacked)
						(*num_moves)++;
				}
				else
					(*num_moves)++;
				break;
			}
			else if (pos[to].color != empty)
				break;
		}

	/* Now check for possible rook captures in the same file */
	for (d=-1;d<=1;d+=2)
 		for (i=1;;i++) {
			to = square+d*i;
			if (pos[to].color == other(turn)) {
				move_list[*num_moves].from = square;
				move_list[*num_moves].to = to;
				move_list[*num_moves].captured_piece = pos[to].piece;
				move_list[*num_moves].type = capture;
				if (searching_pv && PV[ply].from == square && PV[ply].to == to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = 5+piece_val[pos[to].piece];
					if (num_attackers[other_turn][to] > num_attackers[turn][to]-1)
						move_list[*num_moves].score -= piece_val[pos[square].piece];
					else if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][to])
						move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
				}
				if (incheck) {
					MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					PlaceAttacker(pos,other_turn,attack[other_turn]);
					attacked = IsKingAttacked(pos,turn);
					UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					if (!attacked)
						(*num_moves)++;
				}
				else
					(*num_moves)++;
				break;
			}
			else if (pos[to].color != empty)
				break;
		}
}

/********************
*
*	GenerateKingMoves generates all possible moves for a the king
*	except captures. It places the moves in the move_list array
*
*********************/
void GenerateKingMoves(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	t_SquareIndex dx, dy, to;
	int attacked;
	t_Turn other_turn = other(turn);

	for (dy=-1;dy<=1;dy++)
		for (dx=-1;dx<=1;dx++) {
			if (dx == 0 && dy == 0) continue;
			to = square+12*dy+dx;
			if (pos[to].color == empty) {
				move_list[*num_moves].from = square;
				move_list[*num_moves].to = to;
				move_list[*num_moves].captured_piece = null;
				move_list[*num_moves].type = normal;
				if (searching_pv && PV[ply].from == square && PV[ply].to == to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = -15;
					if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][to])
						move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
				}
				if (incheck) {
					MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					PlaceAttacker(pos,other_turn,attack[other_turn]);
					attacked = IsKingAttacked(pos,turn);
					UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					if (!attacked)
						(*num_moves)++;
				}
				else
					(*num_moves)++;
			}
		}

}

/********************
*
*	GenerateKingCaptures generates all possible capture moves for a the king
*.  It places the moves in the move_list array
*
*********************/
void GenerateKingCaptures(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	t_SquareIndex dx, dy, to;
	int attacked;
	t_Turn other_turn = other(turn);

	for (dy=-1;dy<=1;dy++)
		for (dx=-1;dx<=1;dx++) {
			if (dx == 0 && dy == 0) continue;
			to = square+12*dy+dx;
			if (pos[to].color == other(turn)) {
				move_list[*num_moves].from = square;
				move_list[*num_moves].to = to;
				move_list[*num_moves].captured_piece = pos[to].piece;
				move_list[*num_moves].type = capture;

				if (searching_pv && PV[ply].from == square && PV[ply].to == to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = piece_val[pos[to].piece]-50;
					if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][to])
						move_list[*num_moves].score += history[square][to]*HISTORY_BONUS;
				}
				if (incheck) {
					MakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					PlaceAttacker(pos,other_turn,attack[other_turn]);
					attacked = IsKingAttacked(pos,turn);
					UnMakeMove(pos,&move_list[*num_moves],MAX_MOVES,turn);
					if (!attacked)
						(*num_moves)++;
				}
				else
					(*num_moves)++;
			}

		}
}

void GenerateCastleMoves(t_Board pos,t_SquareIndex square,t_Move *move_list,int *num_moves,t_Turn turn,int depth,int ply,int incheck)
{
	t_SquareIndex offset, k_from, k_to;
	t_Move move;
	int attacked, type;
	t_Turn other_turn = other(turn);

	if (incheck) return;
	if (king_moved[depth-1][turn]) return;
    
	offset = 0;
	if (turn == black) offset = 84;

	for (type=short_castle;type<=long_castle;type++) {
		if (type == short_castle) {
			if (rook_ks_moved[depth-1][turn]) continue;
			if (turn == white && (pos[33].piece != rook || pos[33].color != white)) continue;
			else if (turn == black && (pos[117].piece != rook || pos[117].color != black)) continue;

			k_from = offset+30;
			k_to = offset+32;
			if (pos[k_from+1].piece == empty && pos[k_to].piece == empty) {
				move.from = k_from; move.to = k_from+1; move.captured_piece = null;
				move.type = normal;
                if (pos[move.from].piece == empty)
                    printf("Trying to move empty square\n");
                
                    
				MakeMove(pos,&move,MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move,MAX_MOVES,turn);
				if (attacked) continue;

				move.from = k_from; move.to = k_to; move.captured_piece = null;
				move.type = normal;
				MakeMove(pos,&move,MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move,MAX_MOVES,turn);
				if (attacked) continue;
				move_list[*num_moves].from = k_from;
				move_list[*num_moves].to = k_to;
				move_list[*num_moves].captured_piece = null;
				move_list[*num_moves].type = short_castle;
				if (searching_pv && PV[ply].from == square && PV[ply].to == k_to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == k_to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = 100;
					if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == k_to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == k_to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == k_to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][k_to])
						move_list[*num_moves].score += history[square][k_to]*HISTORY_BONUS;
				}
				(*num_moves)++;
			}
		}
		else {
			if (rook_qs_moved[depth-1][turn]) return;
			if (turn == white && (pos[26].piece != rook || pos[26].color != white)) return;
			else if (turn == black && (pos[110].piece != rook || pos[110].color != black)) return;
			k_from = offset+30;
			k_to = offset+28;
			if (pos[k_from-1].piece == empty && pos[k_from-2].piece == empty
			    && pos[k_from-3].piece == empty) {
				move.from = k_from; move.to = k_from-1; move.captured_piece = null;
				move.type = normal;
				MakeMove(pos,&move,MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move,MAX_MOVES,turn);
				if (attacked) return;

				move.from = k_from; move.to = k_to; move.captured_piece = null;
				move.type = normal;
				MakeMove(pos,&move,MAX_MOVES,turn);
				PlaceAttacker(pos,other_turn,attack[other_turn]);
				attacked = IsKingAttacked(pos,turn);
				UnMakeMove(pos,&move,MAX_MOVES,turn);
				if (attacked) return;
				move_list[*num_moves].from = k_from;
				move_list[*num_moves].to = k_to;
				move_list[*num_moves].captured_piece = null;
				move_list[*num_moves].type = long_castle;
				if (searching_pv && PV[ply].from == square && PV[ply].to == k_to)
					move_list[*num_moves].score = PV_SCORE;
				else if (try_tt_move && tt_move.from == square && tt_move.to == k_to)
					move_list[*num_moves].score = TTMOVE_SCORE;
				else {
					move_list[*num_moves].score = 100;
					if (killer[depth].type != null) {
						if (killer[depth].from == square && killer[depth].to == k_to)
							move_list[*num_moves].score += KILLERMV1_SCORE;
					}
					else if (killer2[depth].type != null) {
						if (killer2[depth].from == square && killer2[depth].to == k_to)
							move_list[*num_moves].score += KILLERMV2_SCORE;
					}
					else if (killer3[depth].type != null) {
						if (killer3[depth].from == square && killer3[depth].to == k_to)
							move_list[*num_moves].score += KILLERMV3_SCORE;
					}
					else if (history[square][k_to])
						move_list[*num_moves].score += history[square][k_to]*HISTORY_BONUS;
				}
				(*num_moves)++;
			}
		}
	}



}

/********************
*
*	GenerateMoves generate all posible moves in a certain
*	position given by pos. The function returns a t_Move list of the moves as
*	an argument.
*	The return value is the total number of moves generated.
*
*********************/
int GenerateMoves(t_Board pos,t_Turn turn,t_Move *move_list,int depth,int incheck)
{
	int i, num_moves=0, ply = depth-move_num;
	t_Piece piece;
	t_SquareIndex square;
	/* Search for pieces in the piece list until no more are found */


	for (i=0;piece_list[turn][i] != null;i++) {

		square = piece_list[(int)turn][i];
		piece = pos[square].piece;


		/* Now generate the moves for the piece being considered.
		   The moves generated are placed in move_list */

		switch (piece) {
			case pawn:
				GeneratePromotions(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				GeneratePawnCaptures(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				GenerateEnPassantCaptures(pos,square,move_list,&num_moves,turn,depth-1,ply,incheck);
				break;

			case knight:
				GenerateKnightCaptures(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case bishop:
				GenerateBishopCaptures(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case rook:
				GenerateRookCaptures(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case queen:
				GenerateBishopCaptures(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				GenerateRookCaptures(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case king:
				GenerateKingCaptures(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			default:

				break;
		}

		/*printf("Move: %c%d - %c%d\n",'a'+rank(move_list[i].from)-1,file(move_list[i].from),
				'a'+rank(move_list[i].to)-1,file(move_list[i].to));
		getchar();*/

	}
	for (i=0;piece_list[turn][i] != null;i++) {
		square = piece_list[(int)turn][i];
		piece = pos[square].piece;

		/* Now generate all normal moves */
		switch (piece) {
			case pawn:
				GeneratePawnMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case knight:
				GenerateKnightMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case bishop:
				GenerateBishopMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case rook:
				GenerateRookMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case queen:
				GenerateBishopMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				GenerateRookMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			case king:
				GenerateCastleMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				GenerateKingMoves(pos,square,move_list,&num_moves,turn,depth,ply,incheck);
				break;

			default:

				break;
		}
	}

#ifndef NOMOVEORDER
	qsort(move_list,num_moves,sizeof(t_Move),Comp);
#endif
	return (num_moves);
}


/************
*
* GenerateCapturesToSquare: Generate those capturing moves whose
* destination square is given by to.
*
*************/

int GenerateCapturesToSquare(t_Board pos,t_Turn turn,t_Move *move_list,t_SquareIndex to)

{
	int i, j, d, sign, num_moves=0;
	t_SquareIndex from;
	//t_Piece piece;
	//t_Color color;

	sign = (turn == white ? 1 : -1);



	/* Check for pawn captures on the to square */

	for (i=11;i<=13;i+=2) {
		from = to-i*sign;
		if (pos[from].piece == pawn && pos[from].color == turn) {
			move_list[num_moves].from = from;
			move_list[num_moves].to = to;
			move_list[num_moves].captured_piece = pos[to].piece;
			move_list[num_moves].type = capture;
			num_moves++;
		}
	}

	/* Check for knight captures on the square to */
	for (i=0;i<8;i++) {
		from = to+knight_move_index[i];
		if (pos[from].piece == knight && pos[from].color == turn) {
			move_list[num_moves].from = from;
			move_list[num_moves].to = to;
			move_list[num_moves].captured_piece = pos[to].piece;
			move_list[num_moves].type = capture;
			num_moves++;
		}
	}
	/* Check for bishop-like captures on the to square (bishops and queen)
	   and king captures */

	for (i=-1;i<=1;i+=2)
		for (j=-1;j<=1;j+=2)
			for (d=1;;d++) {
				from = to+d*(12*i+j);
				if (pos[from].color == turn && (pos[from].piece == bishop ||
				pos[from].piece == queen || (d == 1 && pos[from].piece == king))) {
					move_list[num_moves].from = from;
					move_list[num_moves].to = to;
					move_list[num_moves].captured_piece = pos[to].piece;
					move_list[num_moves].type = capture;
					num_moves++;
				}
				else if (pos[from].color != empty)
					break;
			}

	/* Check for rook-like captures on the to square (rooks and queen) and
	   king captrues */
	for (i=-1;i<=1;i+=2) {
		for (d=1;;d++) {
			from = to+d*i;
			if (pos[from].color == turn && (pos[from].piece == rook ||
			pos[from].piece == queen || (d == 1 && pos[from].piece == king))) {
				move_list[num_moves].from = from;
				move_list[num_moves].to = to;
				move_list[num_moves].captured_piece = pos[to].piece;
				move_list[num_moves].type = capture;
				num_moves++;
			}
			else if (pos[from].color != empty)
				break;
		}
		for (d=1;;d++) {
			from = to+12*d*i;
			if (pos[from].color == turn && (pos[from].piece == rook ||
				pos[from].piece == queen || (d == 1 && pos[from].piece == king))) {
				move_list[num_moves].from = from;
				move_list[num_moves].to = to;
				move_list[num_moves].captured_piece = pos[to].piece;
				move_list[num_moves].type = capture;
				num_moves++;
			}
			else if (pos[from].color != empty)
				break;
		}
	}
	return num_moves;
}

int SwapSearch(t_Board pos,t_Turn turn,t_SquareIndex to)
{
	int num_moves, i, score, best;
	t_Move move_list[30];

	if (pos[to].piece == king) return piece_val[king];

	best = (turn == white ? material[white]-material[black] : material[black]-material[white]);

	num_moves = GenerateCapturesToSquare(pos,turn,move_list,to);

	for (i=0;i<num_moves;i++) {
		MakeMove(pos,&move_list[i],MAX_MOVES,turn);
		to = move_list[i].to;
		score = -SwapSearch(pos,other(turn),to);
		UnMakeMove(pos,&move_list[i],MAX_MOVES,turn);
		if (score == -piece_val[king]) continue;
		if (score > best) best = score;
	}
	return best;
}

void SortCaptures(t_Board pos,t_Turn turn,t_Move *move_list,int num_moves)
{
	int i;
	//t_Piece piece;
	t_SquareIndex from, to;

	for (i=0;i<num_moves;i++) {
		from = move_list[i].from;
		to = move_list[i].to;
		MakeMove(pos,&move_list[i],MAX_MOVES,turn);
		move_list[i].score = -SwapSearch(pos,other(turn),to);

		UnMakeMove(pos,&move_list[i],MAX_MOVES,turn);
	}

	qsort(move_list,num_moves,sizeof(t_Move),Comp);
}


/********************
*
*	GenerateCaptures generate all posible capture moves in a certain
*	position given by pos. The function returns a t_Move list of the moves as
*	an argument.
*	The return value is the total number of moves generated.
*
*********************/
int GenerateCaptures(t_Board pos,t_Turn turn,int depth,t_Move *move_list)
{
	int i, num_moves=0, ply = depth-move_num;
	t_Piece piece;
	t_SquareIndex square;
	/* Search for pieces in the piece list until no more are found */
	for (i=0;piece_list[turn][i] != null;i++) {
		square = piece_list[(int)turn][i];
		piece = pos[square].piece;


		/* Now generate the moves for the piece being considered.
		   The moves generated are placed in move_list */
		switch (piece) {
			case pawn:
				GeneratePromotions(pos,square,move_list,&num_moves,turn,depth,ply,false);
				GeneratePawnCaptures(pos,square,move_list,&num_moves,turn,depth,ply,false);
				GenerateEnPassantCaptures(pos,square,move_list,&num_moves,turn,depth-1,ply,false);
				break;

			case knight:
				GenerateKnightCaptures(pos,square,move_list,&num_moves,turn,depth,ply,false);
				break;

			case bishop:
				GenerateBishopCaptures(pos,square,move_list,&num_moves,turn,depth,ply,false);
				break;

			case rook:
				GenerateRookCaptures(pos,square,move_list,&num_moves,turn,depth,ply,false);
				break;

			case queen:
				GenerateBishopCaptures(pos,square,move_list,&num_moves,turn,depth,ply,false);
				GenerateRookCaptures(pos,square,move_list,&num_moves,turn,depth,ply,false);
				break;

			case king:
				GenerateKingCaptures(pos,square,move_list,&num_moves,turn,depth,ply,false);
				break;

			default:

				break;
		}
		/*printf("Move: %c%d - %c%d\n",'a'+rank(move_list[i].from)-1,file(move_list[i].from),
				'a'+rank(move_list[i].to)-1,file(move_list[i].to));
		getchar();*/

	}
	if (num_moves == 0) return 0;
#ifndef NOMOVEORDER
	SortCaptures(pos,turn,move_list,num_moves);
#endif

	return (num_moves);
}

void ClearKiller(void)
{
	int i;
	for (i=0;i<MAX_MOVES;i++) {
		killer[i].type = null;
		killer2[i].type = null;
		killer3[i].type = null;
	}
}

void ClearHistory(void)
{
	int i, j;
	for (i=0;i<144;i++) {
		for (j=0;j<144;j++)
			history[i][j] = 0;
	}
}

/***************
*
* Evaluate gives a score for the current possition
*
****************/
int Evaluate(t_Board pos,t_Turn turn)
{
	int i, score = 0, sign, sign2, piece_sign, dx, dy, square64, square144, offset;
	int r, f;
	t_Color color, other_color;
	int total_attackers;
	t_Turn other_turn = other(turn);

	sign = (turn == white ? 1 : -1);
	score += sign*(material[white]-material[black]);

	score += castled[turn]*20-castled[other_turn]*20;

	for (dy=0;dy<8;dy++)
		for (dx=0;dx<8;dx++) {
			square144 = 26+12*dy+dx;
			square64 = 8*dy+dx;
			color = pos[square144].color;
            piece_sign = (color == turn ? 1 : -1);
            if (pos[square144].piece != empty)
                score += piece_sign*num_attackers[color][square144]*center_score[color][square64];
            
			switch (pos[square144].piece) {
				case empty:
					break;
				case pawn:
					if (pos[square144].color == turn)
						score += pawn_pos_bonus[turn][square64];
					else if (pos[square144].color == other_turn)
						score -= pawn_pos_bonus[other(turn)][square64];

					offset = 0;
					if (pos[square144].color == black) offset = 5;
					if (pos[square144].color == turn && dy == 1+offset && (dx == 3 || dx == 4)) score -= 15;
					r = rank(square144)-1;
					f = file(square144)-1;
                    if (r > 0 && r < 8) {
						if (pawn_struct[white][r] > 0 &&
						pawn_struct[black][r-1] == 0 &&
						pawn_struct[black][r] == 0 &&
						pawn_struct[black][r+1] == 0)
							score += sign*passed_pawn_bonus[f];
						else if (pawn_struct[black][r] > 0 &&
						pawn_struct[white][r-1] == 0 &&
						pawn_struct[white][r] == 0 &&
						pawn_struct[white][r+1] == 0)
							score -= sign*passed_pawn_bonus[7-f];
                    }
                    else if (r == 0) {
						if (pawn_struct[white][r] > 0 &&
						pawn_struct[black][r] == 0 &&
						pawn_struct[black][r+1] == 0)
							score += sign*passed_pawn_bonus[f];
						else if (pawn_struct[black][r] > 0 &&
						pawn_struct[white][r] == 0 &&
						pawn_struct[white][r+1] == 0)
							score -= sign*passed_pawn_bonus[7-f];
                    }
                    else if (r == 7) {
						if (pawn_struct[white][7] > 0 &&
						pawn_struct[black][6] == 0 &&
						pawn_struct[black][7] == 0)
							score += sign*passed_pawn_bonus[f];
						else if (pawn_struct[black][7] > 0 &&
						pawn_struct[white][6] == 0 &&
						pawn_struct[white][7] == 0)
							score -= sign*passed_pawn_bonus[7-f];
                    }
					break;
				case knight:
					if (pos[square144].color == turn)
						score += knight_pos_bonus[turn][square64];
					else if (pos[square144].color == other(turn))
						score -= knight_pos_bonus[other(turn)][square64];
					break;

				case bishop:
					if (pos[square144].color == turn)
						score += bishop_pos_bonus[turn][square64];
					else if (pos[square144].color == other(turn))
						score -= bishop_pos_bonus[other(turn)][square64];
					break;

				case rook:
                    
					if (pawn_struct[turn][rank(square144)-1] == 0) {
						score += 10;
						if (pawn_struct[other_turn][rank(square144)-1] == 0) score += 15;
					}
					break;

				case queen:
                    if (color == white) {
						if (move_num < 20 && square144 != 29) score -= sign*10;
                    }
                    else {
						if (move_num < 20 && square144 != 113) score += sign*10;
                    }
					break;

				case king:
					other_color = other(pos[square144].color);
					sign2 = (other_color == turn ? 1 : -1);
					if (material[white]+material[black] < 2500) score -= sign2*king_center_bonus[color][square64];
					total_attackers = num_attackers[other_color][square144+sign*11];
					total_attackers += num_attackers[other_color][square144+sign*12];
					total_attackers += num_attackers[other_color][square144+sign*13];
					total_attackers += num_attackers[other_color][square144+1];
					total_attackers += num_attackers[other_color][square144-1];
					total_attackers += num_attackers[other_color][square144-sign*11];
					total_attackers += num_attackers[other_color][square144-sign*12];
					total_attackers += num_attackers[other_color][square144-sign*13];
					score += 5*sign2*total_attackers;
					break;

				default:

					break;
			}
		}
	for (i=0;i<8;i++) {
		if (pawn_struct[turn][i] > 1) score -= 15;
		if (pawn_struct[other_turn][i] > 1) score += 15;
		if (i > 0 && i < 8 && pawn_struct[turn][i] > 0 && pawn_struct[turn][i-1] == 0 &&
			pawn_struct[turn][i+1] == 0) score -= 10;
		if (i > 0 && i < 8 && pawn_struct[other_turn][i] > 0 && pawn_struct[other_turn][i-1] == 0 &&
			pawn_struct[other_turn][i+1] == 0) score += 10;
	}
	return score;
}

int QuiescenseSearch(t_Board pos,t_Turn turn,int ply,int depth,int alpha,int beta,t_Move best_line[])
{
	int i, p, num_moves, score, best, value;
	int at, captured_piece_val, incheck;
    int delta;
	t_Turn other_turn = other(turn);
	t_Move move_list[100];
	t_Move next_line[MAX_PVSIZE];
	int ttDepth;
	t_Move ttMove;
	int ttScore, ttAlpha, ttBeta, ttType;
	t_SquareIndex from, to;
	int Rule50 = Game50;

	best_line[ply].type = null;
	/*best_line[ply+1].type = null;*/


	quiescense_nodes++;
	nodes++;

	if (ply == depth+1 && LookupTT(&ttDepth,&ttMove,&ttScore,&ttAlpha,&ttBeta,&ttType,turn)) {
		tt_probes++;
#ifdef TRACESEARCH
        printf("t probe: d:%d m:%c%d-%c%d s:%d a:%d b:%d t:%s turn:%d\n",ttDepth,'a'+rank(ttMove.from)-1,file(ttMove.from),'a'+rank(ttMove.to)-1,file(ttMove.to),ttScore,ttAlpha,ttBeta,ttType==normal_move?"nor":ttType==fail_high?"fh":"fl",turn);
#endif
        if (ttDepth >= move_num+depth-ply) {
#ifdef TRACESEARCH
            printf("t hit\n");
#endif
        
			tt_hits++;
			switch (ttType) {
				case normal_move:
					return ttScore;
					break;
				case fail_high:
					if (ttBeta >= beta) return ttBeta;
					break;
				case fail_low:
					if (ttAlpha <= alpha) return ttAlpha;
					break;
			}

		}
	}

	if ((incheck = IsInCheck(pos,turn))) {
		if ((num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,true)) == 0) return -10000+ply; // No moves to uncheck: Checkmate
	}
	else {
		score = Evaluate(pos,turn);
        //if (score >= beta)
        //    return beta;
		best = score;
		if (score > alpha) {
			alpha = score;
			if (score >= beta) return score;
		}
        //PlaceAttacker(pos,other_turn,attack[other_turn]);
		num_moves = GenerateCaptures(pos,turn,move_num+ply,move_list);
#ifdef TRACESEARCH
        printf("num q moves: %d\n",num_moves);
#endif
	}

	if (num_moves == 0 || ply-depth >= MAX_QUIESCENSE_DEPTH || move_num+ply > MAX_MOVES-2) {

		return (score);
	}

	for (i=0;i<num_moves/2;i++) {
		if (!incheck && move_list[i].score < 0) return best;

		from = move_list[i].from;
		to = move_list[i].to;
		if (pos[to].color == other_turn && pos[to].piece == king) {
			return piece_val[king];
		}
		delta = piece_val[pos[to].piece] - score;
		if (delta < alpha-50) {
			best_line[ply+1].type = null;
			continue;
		}
		captured_piece_val = piece_val[pos[to].piece];
		MakeMove(pos,&move_list[i],move_num+ply,turn);
		PlaceAttacker(pos,other_turn,attack[other_turn]);

		if (IsKingAttacked(pos,turn)) {
			move_list[i].score = -piece_val[king];
			/*best_line[ply+1].type = null;*/
			UnMakeMove(pos,&move_list[i],move_num+ply,turn);
			Game50 = Rule50;
			continue;
		}
		at = attack[other_turn][to];

		if (at != 0 && piece_val[at] < piece_val[pos[to].piece]-captured_piece_val) {
			UnMakeMove(pos,&move_list[i],move_num+ply,turn);
			continue;
		}
#ifdef TRACESEARCH
		for (p=0;p<ply;p++) printf(" ");
		printf("%d/%d:%c%d-%c%d ply:%d ",i+1,num_moves+1,'a'+rank(move_list[i].from)-1,
			    file(move_list[i].from),'a'+rank(move_list[i].to)-1,
				file(move_list[i].to),ply+1);
		getchar();
#endif
		PlaceAttacker(pos,turn,attack[turn]);

		value = -QuiescenseSearch(pos,other_turn,ply+1,depth,-beta,-alpha,next_line);
		UnMakeMove(pos,&move_list[i],move_num+ply,turn);

		Game50 = Rule50;
#ifdef TRACESEARCH
		for (p=0;p<ply;p++) printf(" ");
		printf("s:%d\n",value);
#endif
		if (value == -piece_val[king]) move_list[i].score = -piece_val[king];
		else if (value > piece_val[king] - 50) return value;
		if (value > best) {
			best = value;
			for (p=ply;next_line[p-1].type != null;p++) {
				best_line[p] = next_line[p];
			}
			best_line[p+1].type = null;
			best_line[ply].from = move_list[i].from;
			best_line[ply].to = move_list[i].to;
			best_line[ply].type =  move_list[i].type;
			best_line[ply].captured_piece =  move_list[i].captured_piece;
			if (killer2[move_num+ply].from != move_list[i].from &&
				killer2[move_num+ply].to != move_list[i].to) {
				killer3[move_num+ply] = killer2[move_num+ply];
				killer2[move_num+ply] = move_list[i];
			}
			if (best > alpha) {
                cutoffs++;
				alpha = best;
				if (alpha >= beta) {
					killer[move_num+ply] = move_list[i];
					if (ply < depth+2)
						StoreInTT(move_num+depth-ply,best_line[ply],best,alpha,beta,fail_high,turn);

					best_line[ply+1].type = null;
					return (alpha);
				}
			}
		}

	}

	if (ply < depth+2)
		StoreInTT(move_num+depth-ply,best_line[ply],best,alpha,beta,normal_move,turn);

	if (best == -piece_val[king]) return -piece_val[king]+ply;

	return (best);
}

int NegaScout (t_Board pos,t_Turn turn,int ply,int depth,int alpha,int beta,int do_null_move,t_Move best_line[])
{                        /* compute minimax value of position pos */
	int a, b, t, i, num_moves, p, best = -INFINITY;
    int in_check;
    //int null_depth_reduce;
	int extensions = 0;
	//t_SquareIndex from, to;
	//int ttDepth, ttScore, ttAlpha, ttBeta, ttType;
	//t_Move ttMove;
	t_Move move_list[200];
	t_Turn other_turn = other(turn);
	t_Move next_line[MAX_PVSIZE];
	t_SquareIndex old_attack[2][144];
	int old_nattacks[2][144];
	int Rule50 = Game50;

	nodes++;

	if (!(nodes % NODES_BETWEEN_POLL))
		Poll();

	try_tt_move = false;
	/*if (LookupTT(&ttDepth,&ttMove,&ttScore,&ttAlpha,&ttBeta,&ttType,turn)) {
		tt_probes++;
		if (ttDepth >= move_num+depth-ply) {
			tt_hits++;
			switch (ttType) {
				case normal_move:
					return ttScore;
					break;
				case fail_high:
					if (ttBeta >= beta) return ttBeta;
					break;
				case fail_low:
					if (ttAlpha <= alpha) return ttAlpha;
					break;
			}

		}
		else if (ttType == normal_move) {
			tt_move = ttMove;
			try_tt_move = true;
		}
	}*/

	if ((in_check = IsInCheck(pos,turn))) {
		if (ply < 20)
			extensions++;
		if ((num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,true)) == 0) {
			try_tt_move = false;
			return -10000+ply;
		}
	}
	else {
		if (ply >= depth) {
#ifndef NOQUIESCE
			return QuiescenseSearch(pos,turn,ply,depth,alpha,beta,best_line);
#else
			best_line[ply+1].type = null;
			return Evaluate(pos,turn);
#endif
		}
		num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,false);
		try_tt_move = false;
	}
	if (num_moves == 0) return 0;			/* Stalemate */

	/* Null move search */

	/*if (do_null_move && !in_check) {
#ifdef TRACESEARCH
		printf("null move turn:%d",other_turn); getchar();
#endif
		null_depth_reduce = (depth-ply) < 4 ? 1 : 3;
		t = -NegaScout (pos,other_turn,ply,depth-null_depth_reduce,-beta,-alpha,false,next_line);
		if (t >= beta) {
			best_line[ply+1].type = null;
			return beta;
		}
	}*/
	a = alpha;
	b = beta;

	for ( i = 0; i < num_moves; i++ ) {
		if (searching_pv) {
			searching_pv = (i == 0 ? true : false);
		}

#ifdef TRACESEARCH
		for (p=0;p<ply;p++) printf(" ");
		printf("%d/%d:%c%d-%c%d ply:%d",i+1,num_moves+1,'a'+rank(move_list[i].from)-1,
			    file(move_list[i].from),'a'+rank(move_list[i].to)-1,
				file(move_list[i].to),ply+1);
		getchar();
#endif
		/*from = move_list[i].from;
		to = move_list[i].to;
		if (pos[to].color == other_turn && pos[to].piece == king) {
			return piece_val[king];
		}*/

		MakeMove(pos,&move_list[i],move_num+ply,turn);

		PlaceAttacker(pos,turn,attack[turn]);
		PlaceAttacker(pos,other_turn,attack[other_turn]);
		memcpy(old_attack,attack,288*sizeof(t_SquareIndex));
		memcpy(old_nattacks,num_attackers,288*sizeof(int));
		if (IsKingAttacked(pos,turn)) {
			move_list[i].score = -INFINITY;
			UnMakeMove(pos,&move_list[i],move_num+ply,turn);
			Game50 = Rule50;
			continue;
		}


		t = -NegaScout (pos,other_turn,ply+1,depth+extensions,-b,-a,true,next_line);

		if ((t > a) && (t < beta) && (i > 0) && (ply < depth-1)) {
			memcpy(attack,old_attack,288*sizeof(t_SquareIndex));
			memcpy(num_attackers,old_nattacks,288*sizeof(int));
			a = -NegaScout (pos,other_turn,ply+1,depth+extensions,-beta, -t,true,next_line);
		}
		UnMakeMove(pos,&move_list[i],move_num+ply,turn);
		Game50 = Rule50;

		if (t > piece_val[king] - 50) return t;
		a = MAX( a, t );

		if (a > best) {
			best = a;
			for (p=ply;next_line[p-1].type != null;p++) {
				best_line[p] = next_line[p];
			}
			best_line[p+1].type = null;

			best_line[ply].from = move_list[i].from;
			best_line[ply].to = move_list[i].to;
			best_line[ply].type =  move_list[i].type;
			best_line[ply].captured_piece =  move_list[i].captured_piece;

			if (killer2[move_num+ply].from != move_list[i].from &&
				killer2[move_num+ply].to != move_list[i].to) {
				killer3[move_num+ply] = killer2[move_num+ply];
				killer2[move_num+ply] = move_list[i];
			}
			if ( a >= beta ) {
				/*printf("cutoff at move_num:%d:%c%d-%c%d\n",i,'a'+rank(move_list[i].from)-1,
					file(move_list[i].from),'a'+rank(move_list[i].to)-1,
					file(move_list[i].to));

				PrintBoard(pos,white); getchar();*/

				killer[move_num+ply] = move_list[i];

				best_line[ply+1].type = null;
				StoreInTT(move_num+depth-ply,best_line[ply],best,a,b,fail_high,turn);
				history[best_line[ply].from][best_line[ply].to]++;
				cutoffs++;
				return ( a );                /* cut-off */
			}
		}


		b = a + 1;                         /* set new null window */
		if (flag.timeout) return best;
	}
	history[best_line[ply].from][best_line[ply].to]++;


	StoreInTT(move_num+depth-ply,best_line[ply],best,a,b,(alpha==a)?fail_low:normal_move,turn);



	if (best == -piece_val[king]) return -10000+ply;

	return ( a );
}

int NegaScout2 (t_Board pos,t_Turn turn,int ply,int depth,int alpha,int beta,int do_null_move,t_Move best_line[])
{                        /* compute minimax value of position pos */
    int a, b, t, i, num_moves, p, best = -INFINITY;
    int in_check;
    //int null_depth_reduce;
    int extensions = 0;
    //t_SquareIndex from, to;
    //int ttDepth, ttScore, ttAlpha, ttBeta, ttType;
    //t_Move ttMove;
    t_Move move_list[200];
    t_Turn other_turn = other(turn);
    t_Move next_line[MAX_PVSIZE];
    t_SquareIndex old_attack[2][144];
    int old_nattacks[2][144];
    int Rule50 = Game50;
    
    nodes++;
    
    if (!(nodes % NODES_BETWEEN_POLL))
        Poll();
    
    try_tt_move = false;
    /*if (LookupTT(&ttDepth,&ttMove,&ttScore,&ttAlpha,&ttBeta,&ttType,turn)) {
     tt_probes++;
     if (ttDepth >= move_num+depth-ply) {
     tt_hits++;
     switch (ttType) {
     case normal_move:
					return ttScore;
					break;
     case fail_high:
					if (ttBeta >= beta) return ttBeta;
					break;
     case fail_low:
					if (ttAlpha <= alpha) return ttAlpha;
					break;
     }
     
     }
     else if (ttType == normal_move) {
     tt_move = ttMove;
     try_tt_move = true;
     }
     }*/
    
    if ((in_check = IsInCheck(pos,turn))) {
        if (ply < 20)
            extensions++;
        if ((num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,true)) == 0) {
            try_tt_move = false;
            return -10000+ply;
        }
    }
    else {
        if (ply >= depth) {
#ifndef NOQUIESCE
            return QuiescenseSearch(pos,turn,ply,depth,alpha,beta,best_line);
#else
            best_line[ply+1].type = null;
            return Evaluate(pos,turn);
#endif
        }
        num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,false);
        try_tt_move = false;
    }
    if (num_moves == 0) return 0;			/* Stalemate */
    
    /* Null move search */
    
    /*if (do_null_move && !in_check) {
     #ifdef TRACESEARCH
     printf("null move turn:%d",other_turn); getchar();
     #endif
     null_depth_reduce = (depth-ply) < 4 ? 1 : 3;
     t = -NegaScout (pos,other_turn,ply,depth-null_depth_reduce,-beta,-alpha,false,next_line);
     if (t >= beta) {
     best_line[ply+1].type = null;
     return beta;
     }
     }*/
    a = alpha;
    b = beta;
    
    for ( i = 0; i < num_moves; i++ ) {
        if (searching_pv) {
            searching_pv = (i == 0 ? true : false);
        }
        
#ifdef TRACESEARCH
        for (p=0;p<ply;p++) printf(" ");
        printf("%d/%d:%c%d-%c%d ply:%d",i+1,num_moves+1,'a'+rank(move_list[i].from)-1,
               file(move_list[i].from),'a'+rank(move_list[i].to)-1,
               file(move_list[i].to),ply+1);
        getchar();
#endif
        /*from = move_list[i].from;
         to = move_list[i].to;
         if (pos[to].color == other_turn && pos[to].piece == king) {
         return piece_val[king];
         }*/
        
        MakeMove(pos,&move_list[i],move_num+ply,turn);
        
        PlaceAttacker(pos,turn,attack[turn]);
        PlaceAttacker(pos,other_turn,attack[other_turn]);
        memcpy(old_attack,attack,288*sizeof(t_SquareIndex));
        memcpy(old_nattacks,num_attackers,288*sizeof(int));
        if (IsKingAttacked(pos,turn)) {
            move_list[i].score = -INFINITY;
            UnMakeMove(pos,&move_list[i],move_num+ply,turn);
            Game50 = Rule50;
            continue;
        }
        
        if (i > 0) {
            t = -NegaScout (pos,other_turn,ply+1,depth+extensions,-alpha-1,-alpha,true,next_line);
            
            if ((t > alpha) && (t < beta) && (ply < depth-1)) {
                memcpy(attack,old_attack,288*sizeof(t_SquareIndex));
                memcpy(num_attackers,old_nattacks,288*sizeof(int));
                t = -NegaScout (pos,other_turn,ply+1,depth+extensions,-beta, -t,true,next_line);
            }
        }
        else {
            t = -NegaScout (pos,other_turn,ply+1,depth+extensions,-beta,-alpha,true,next_line);
        }
        
        UnMakeMove(pos,&move_list[i],move_num+ply,turn);
        Game50 = Rule50;
        
        if (t > piece_val[king] - 50) return t;
        alpha = MAX( alpha, t );
        
        if (alpha > beta) {
            best = alpha;
            for (p=ply;next_line[p-1].type != null;p++) {
                best_line[p] = next_line[p];
            }
            best_line[p+1].type = null;
            
            best_line[ply].from = move_list[i].from;
            best_line[ply].to = move_list[i].to;
            best_line[ply].type =  move_list[i].type;
            best_line[ply].captured_piece =  move_list[i].captured_piece;
            
            if (killer2[move_num+ply].from != move_list[i].from &&
                killer2[move_num+ply].to != move_list[i].to) {
                killer3[move_num+ply] = killer2[move_num+ply];
                killer2[move_num+ply] = move_list[i];
            }
            //if ( a >= beta ) {
                /*printf("cutoff at move_num:%d:%c%d-%c%d\n",i,'a'+rank(move_list[i].from)-1,
                 file(move_list[i].from),'a'+rank(move_list[i].to)-1,
                 file(move_list[i].to));
                 
                 PrintBoard(pos,white); getchar();*/
                
                killer[move_num+ply] = move_list[i];
                
                best_line[ply+1].type = null;
                StoreInTT(move_num+depth-ply,best_line[ply],best,a,b,fail_high,turn);
                history[best_line[ply].from][best_line[ply].to]++;
                cutoffs++;
                //return ( alpha );                /* cut-off */
            break;
            //}
        }
        
        
        //b = a + 1;                         /* set new null window */
        if (flag.timeout) return best;
    }
    history[best_line[ply].from][best_line[ply].to]++;
    
    
    StoreInTT(move_num+depth-ply,best_line[ply],best,a,b,(alpha==a)?fail_low:normal_move,turn);
    
    
    
    if (best == -piece_val[king]) return -10000+ply;
    
    return alpha;
}

int AlphaBeta(t_Board pos,t_Turn turn,int ply,int depth,int alpha,int beta,int do_null_move,t_Move best_line[])
{
	int i, p, num_moves, score, best=-INFINITY, alpha_o = alpha;
	int in_check;
	int extensions=0, null_depth_reduce;
	t_Move ttMove;
	int ttScore, ttAlpha, ttBeta, ttType, ttDepth;
	int Rule50 = Game50;
	t_Turn other_turn = other(turn);
	t_Move move_list[200];
	t_Move next_line[MAX_PVSIZE];
	t_SquareIndex from, to;
	/*best_line[ply+1].type = null;*/

	nodes++;

	if (!(nodes % NODES_BETWEEN_POLL))
		Poll();


	try_tt_move = false;
#ifndef NOTTABLE
	if (LookupTT(&ttDepth,&ttMove,&ttScore,&ttAlpha,&ttBeta,&ttType,turn)) {
		tt_probes++;
#ifdef TRACESEARCH
        printf("t probe: d:%d m:%c%d-%c%d s:%d a:%d b:%d t:%s turn:%d\n",ttDepth,'a'+rank(ttMove.from)-1,file(ttMove.from),'a'+rank(ttMove.to)-1,file(ttMove.to),ttScore,ttAlpha,ttBeta,ttType==normal_move?"nor":ttType==fail_high?"fh":"fl",turn);
#endif
		if (ttDepth >= move_num+depth-ply) {
			tt_hits++;
#ifdef TRACESEARCH
            printf("t hit\n");
#endif
			switch (ttType) {
				case normal_move:
					return ttScore;
					break;
				case fail_high:
					if (ttBeta >= beta) {
						return ttBeta;
					}
					break;
				case fail_low:
					if (ttAlpha <= alpha) {
						return ttAlpha;
					}
					break;
			}

		}
		else if (ttType == normal_move) {
			tt_move = ttMove;
			try_tt_move = true;
		}

	}
#endif

	if (RepetitionCheck(ply)) return 0;

	if ((in_check = IsInCheck(pos,turn))) {
		if (ply < 20)
			extensions++;
		if ((num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,true)) == 0) {
			return -10000+ply;
		}
	}

	else {
		if (ply >= depth) {
#ifndef NOQUIESCE
			return QuiescenseSearch(pos,turn,ply,depth,alpha,beta,best_line);
#else
			best_line[ply+1].type = null;
			return Evaluate(pos,turn);
#endif
		}

		num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,false);
	}

	if (flag.draw) printf("flag.draw in alphabeta: %d, ply: %d\n",flag.draw,ply);
	if (num_moves == 0) return 0;

	/* Null move search */

	if (depth-ply > 1 && do_null_move && !in_check) {
		/*null_depth_reduce = (depth-ply) < 4 ? 1 : 3;*/
		null_depth_reduce = 1;
		score = -AlphaBeta (pos,other_turn,ply+1,depth-null_depth_reduce,-beta,-alpha,false,next_line);
		if (score > beta) {
			best_line[ply+1].type = null;
			return score;
		}
	}

	for (i=0;i<num_moves;i++) {
		if (searching_pv) {
			searching_pv = (i == 0 ? true : false);
		}

		from = move_list[i].from;
		to = move_list[i].to;
		if (pos[to].color == other_turn && pos[to].piece == king) {
			return piece_val[king];
		}
		MakeMove(pos,&move_list[i],move_num+ply,turn);
		PlaceAttacker(pos,turn,attack[turn]);
		PlaceAttacker(pos,other_turn,attack[other_turn]);

		if (IsKingAttacked(pos,turn)) {
			move_list[i].score = -piece_val[king];
			UnMakeMove(pos,&move_list[i],move_num+ply,turn);
			Game50 = Rule50;
			continue;
		}
		/*if (depth-ply == 2) {
			if (move_list[i].type != capture && !IsInCheck(pos,other_turn)) {
				score = Evaluate(pos,turn);
				if (score+350 < alpha) {
					UnMakeMove(pos,&move_list[i],move_num+ply,turn);
					return score;
				}
			}
		}
		else if (depth-ply == 3) {
			if (move_list[i].type != capture && !IsInCheck(pos,other_turn)) {
				score = Evaluate(pos,turn);
				if (score+500 < alpha) {
					depth--;
				}
			}
		}*/

#ifdef TRACESEARCH
		for (p=0;p<ply;p++) printf(" ");
		printf("%d/%d:%c%d-%c%d ply:%d a:%d b:%d",i+1,num_moves+1,'a'+rank(move_list[i].from)-1,
			    file(move_list[i].from),'a'+rank(move_list[i].to)-1,
				file(move_list[i].to),ply+1,alpha,beta);
		//getchar();
#endif
		/*if (depth == 4) {
			for (p=0;p<ply;p++) printf(" ");
			printf("%d/%d:%c%d-%c%d ply:%d\n",i+1,num_moves+1,'a'+rank(move_list[i].from)-1,
		    	file(move_list[i].from),'a'+rank(move_list[i].to)-1,
				file(move_list[i].to),ply+1);
		}*/
		score = -AlphaBeta(pos,other_turn,ply+1,depth+extensions,-beta,-alpha,do_null_move,next_line);
		UnMakeMove(pos,&move_list[i],move_num+ply,turn);

		Game50 = Rule50;

#ifdef TRACESEARCH
		for (p=0;p<ply;p++) printf(" ");
        printf("s:%d\n",score); getchar();
#endif

		if (score > piece_val[king] - 50) return score;

		if (score > best) {
			best = score;

			for (p=ply;next_line[p-1].type != null;p++) {
				best_line[p] = next_line[p];
			}
			best_line[p+1].type = null;

			best_line[ply].from = move_list[i].from;
			best_line[ply].to = move_list[i].to;
			best_line[ply].type =  move_list[i].type;
			best_line[ply].captured_piece =  move_list[i].captured_piece;


			if (killer2[move_num+ply].from != move_list[i].from &&
				killer2[move_num+ply].to != move_list[i].to) {
				killer3[move_num+ply] = killer2[move_num+ply];
				killer2[move_num+ply] = move_list[i];
			}
			if (best > alpha) {
				alpha = best;
				if (alpha >= beta) {
                    cutoffs++;
					killer[move_num+ply] = move_list[i];
					//best_line[ply+1].type = null;
#ifndef NOTTABLE
					StoreInTT(move_num+depth-ply,best_line[ply],best,alpha,beta,fail_high,turn);
#endif
					history[best_line[ply].from][best_line[ply].to]++;
					return ( alpha );
				}
			}
		}


		/*if (flag.timeout) return best;*/

	}

	history[best_line[ply].from][best_line[ply].to]++;


	/*if (depth > 3) {
		PrintBoard(pos,turn);
		printf("no cutoff i: %d num_moves:%d\n",i,num_moves); getchar();
	}*/
#ifndef NOTTABLE
	StoreInTT(move_num+depth-ply,best_line[ply],best,alpha,beta,(alpha==alpha_o)?fail_low:normal_move,turn);
#endif
	if (best == -piece_val[king]) return -piece_val[king]+ply;

	return best;
}

uint64_t GetTimeInMiliSeconds(void)
{
    uint64_t        time;
    uint64_t        timeMiliSec;
    static mach_timebase_info_data_t    sTimebaseInfo;
    

    
    time = mach_absolute_time();

    
    // Convert to nanoseconds.
    
    // If this is the first time we've run, get the timebase.
    // We can use denom == 0 to indicate that sTimebaseInfo is
    // uninitialised because it makes no sense to have a zero
    // denominator is a fraction.
    
    if ( sTimebaseInfo.denom == 0 ) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    
    // Do the maths. We hope that the multiplication doesn't
    // overflow; the price you pay for working in fixed point.
    
    timeMiliSec = 1e-6*time * sTimebaseInfo.numer / sTimebaseInfo.denom;
    
    return timeMiliSec;
}


DWORD ElapsedTime(int init)
{
	//DWORD current_time = GetTickCount();
    DWORD current_time = GetTimeInMiliSeconds();
	static DWORD initial_time;
	if (init) {
		initial_time = current_time;
		return 0;
	}
	else
		return ((current_time - initial_time));
}

/***************
*
* SearchMove performs the search of the possition to eventually return a
* move for the current turn.
*
****************/
int SearchMove(t_Board pos,t_Move *move,t_Move *move_list,int num_moves,int depth,t_Turn turn,int alpha,int beta,t_Move best_line[])
{
	//int m, n;
	int i, p, ply = 0, best = -INFINITY;
	int cond2 = 0, Rule50 = Game50;

	t_Turn other_turn = other(turn);
	//t_SquareIndex old_attack[2][144];
	t_Move next_line[100];
	//int old_nattacks[2][144];

	ClearBuffer();

	next_line[2].type = null;
#ifdef TRACESEARCH
	printf("depth: %d\n",depth);
#endif
#ifndef XBOARD
	printf("alpha: %d beta: %d\n",alpha,beta);
#endif
	best = -INFINITY;

	for (i=0;i<num_moves;i++) {
		if (depth == 1) searching_pv = true;
		else
			searching_pv = (i==0 ? true : false);

		MakeMove(pos,&move_list[i],move_num,turn);
		PlaceAttacker(pos,turn,attack[turn]);
		PlaceAttacker(pos,other_turn,attack[other_turn]);
		if (IsKingAttacked(pos,turn)) {
			move_list[i].score = -INFINITY;
			UnMakeMove(pos,&move_list[i],move_num,turn);
			Game50 = Rule50;
			continue;
		}

#ifdef TRACESEARCH
		for (p=0;p<ply;p++) printf(" ");
		printf("%d/%d:%c%d-%c%d ply:%d\n",i+1,num_moves+1,'a'+rank(move_list[i].from)-1,
		    file(move_list[i].from),'a'+rank(move_list[i].to)-1,
			file(move_list[i].to),ply+1);
		//getchar();
#endif
		computer_player = turn;

		/*move_list[i].score = MTDF(pos,other_turn,ply+1,depth,move_list[i].score,next_line);*/
#ifdef ASPIRATION_WINDOW
        if (i>0) {
            alpha = best - 1/4; beta = best + 1/4;
        }
        while (1) {
#endif
#if	defined(ALPHABETA_SEARCH)
        /* There is a bug with null move: when doing the null move, the king_moved status is not updated properly (doesn't transmit status to next depth) and the GenerateCastleMoves doesn't think the king has moved, even in position where it has. Then it tries to generate a king castle move but when this move is made (in MakeMove), the from square is empty (-1) and this attempts to access an element in the piece_index_map array at position -1. Aparentely, this just goes to the last element of this array, which should always be null to indicate the end of array, but it is then modified. This currupts the whole move generation system. I think the king_moved status has to be copied to depth+1 before doing the null move (calling AlphaBeta) in the null_move if clause in AlphaBeta function */
            //printf("alpha: %d, beta: %d\n",alpha,beta);
		move_list[i].score = -AlphaBeta(pos,other_turn,ply+1,depth,-beta,-alpha,false,next_line);
#else
		move_list[i].score = -NegaScout2(pos,other_turn,ply+1,depth,-beta,-alpha,false,next_line);
#endif
#ifdef ASPIRATION_WINDOW
            if (move_list[i].score > beta)
                beta = move_list[i].score + 1;
            
            else if (move_list[i].score < alpha) alpha = move_list[i].score - 1;
            else break;
        }
#endif
#ifdef TRACESEARCH
		for (p=0;p<ply;p++) printf(" ");
        printf("a:%d b:%d s:%d\n",alpha,beta,move_list[i].score); getchar();
#endif

		UnMakeMove(pos,&move_list[i],move_num,turn);

		Game50 = Rule50;

		if (move_list[i].score > best || (cond2 = (move_list[i].score == best &&
			rand()%2)) ) {

			best = move_list[i].score;
			if (cond2) (move_list[i].score)++;
			move->from = move_list[i].from;
			move->to = move_list[i].to;
			move->captured_piece = move_list[i].captured_piece;
			move->type = move_list[i].type;
			move->piece_index = move_list[i].piece_index;

			for (p=1;next_line[p-1].type != null;p++) {
				best_line[p] = next_line[p];
			}
			best_line[p+1].type = null;

			best_line[ply].from = move_list[i].from;
			best_line[ply].to = move_list[i].to;
			best_line[ply].type =  move_list[i].type;
			best_line[ply].captured_piece =  move_list[i].captured_piece;

			//PostPV(depth,best,ElapsedTime(0),best_line,false);
            if (best > alpha) {
                alpha = best;
                //beta = -best;
                if (alpha >= beta) {
                    killer[move_num+ply] = move_list[i];
                    //best_line[ply+1].type = null;
                    StoreInTT(move_num+depth-ply,*move,best,alpha,beta,fail_high,turn);
                    //return alpha;
#ifdef TRACESEARCH
                    for (p=0;p<ply;p++) printf(" ");
                    PostPV(depth, best, ElapsedTime(0), best_line, false);
                        
#endif
                    break;

                }
            }
		}

		if (best >= 10000-depth) break;

		if (ElapsedTime(0)/1000 > search_time) flag.timeout = true;
		if (flag.timeout) return best;

		if (Poll()) {
			if (flag.timeout) return best;
		}
	}

	//qsort(move_list,num_moves,sizeof(t_Move),Comp);
    qsort(move_list,i,sizeof(t_Move),Comp);
#ifndef NOTTABLE
	StoreInTT(move_num+depth-ply,*move,best,0,0,normal_move,turn);
#endif
	for (p=0;best_line[p].type != null;p++) {
		PV[p].from = best_line[p].from;
		PV[p].to = best_line[p].to;
		PV[p].type = best_line[p].type;
		PV[p].captured_piece = best_line[p].captured_piece;
	}
	PV[p+1].type = null;

	//PostPV(depth,best,ElapsedTime(0),best_line,true);

	history[best_line[ply].from][best_line[ply].to]++;


	return best;

}

int SelectMove(t_Board pos,t_Move *move,int max_depth,t_Turn turn)
{
	int m, depth, alpha, beta, best, num_moves;
	int timeout, elapsed_time;

	t_Move move_list[100];
	t_Move best_line[MAX_PVSIZE];

	ClearHistory();
	ClearKiller();
	PlaceAttacker(pos,turn,attack[turn]);
	PlaceAttacker(pos,other(turn),attack[other(turn)]);

	flag.draw = false;

	if (RepetitionCheck(0) > 2) {
		flag.draw = true;
		return 0;
	}
	if (IsInCheck(pos,turn)) {
		printf("Is in check\n");
		if ((num_moves = GenerateMoves(pos,turn,move_list,move_num,true)) == 0) {
			return -10000;
		}
	}

	else {
		num_moves = GenerateMoves(pos,turn,move_list,move_num,false);

		if (num_moves == 0) {
			flag.draw = true;
			return 0;
		}
	}



	alpha = -INFINITY;
	beta = INFINITY;

	flag.timeout = false;

	PV[0].type = null;
	searching_pv = true;

	search_time = comp_time/60;
#ifndef XBOARD
    printf("search time: %ld\n",search_time);
#endif
	ElapsedTime(1);

	nodes = 0; quiescense_nodes = 0; tt_hits = 0; tt_probes = 0;
	cutoffs = 0;

	timeout = false;

	for (depth = 1;depth<=max_depth;depth++) {
		ClearHistory();

		/*for (m=0;m<num_moves;m++) {
				printf("%c%d-%c%d:%d ",'a'+rank(move_list[m].from)-1,file(move_list[m].from),
				'a'+rank(move_list[m].to)-1,file(move_list[m].to),move_list[m].score);
			} getchar();*/
		best = SearchMove(pos,move,move_list,num_moves,depth,turn,alpha,beta,best_line);
        PostPV(depth, best, ElapsedTime(0), best_line, false);
		if (best <= alpha) {
			printf("%d-\n",depth);
			if (flag.timeout) {
				flag.timeout = false;
				search_time += (comp_time-ElapsedTime(0)/1000)/20;
				timeout = true;
			}
			best = SearchMove(pos,move,move_list,num_moves,depth,turn,-INFINITY,beta,best_line);
            
		}
        
		if (best >= beta && best < 9900) {
			printf("%d+\n",depth);
			if (flag.timeout) {
				flag.timeout = false;
				search_time += (comp_time-ElapsedTime(0)/1000)/20;
				timeout = true;
			}
			best = SearchMove(pos,move,move_list,num_moves,depth,turn,alpha,INFINITY,best_line);
		}
        
		if (best < -9000 || best > 9000) break;
		//alpha = best-100;
		//beta = best+100;
        elapsed_time =  ElapsedTime(0);
        PostPV(depth, best, elapsed_time, best_line, false);
        PostPV(depth,best,elapsed_time,best_line,true);
		if (elapsed_time/1000 > search_time) flag.timeout = true;
		if (flag.timeout) break;
	}

	return best;
}

unsigned int Random32(void)
{
	/*
	random numbers from Mathematica 2.0.
	SeedRandom = 1;
	Table[Random[Integer, {0, 2^32 - 1}]
 	*/
	static unsigned long x[55] = {
		1410651636UL, 3012776752UL, 3497475623UL, 2892145026UL, 1571949714UL,
		3253082284UL, 3489895018UL, 387949491UL, 2597396737UL, 1981903553UL,
		3160251843UL, 129444464UL, 1851443344UL, 4156445905UL, 224604922UL,
		1455067070UL, 3953493484UL, 1460937157UL, 2528362617UL, 317430674UL,
		3229354360UL, 117491133UL, 832845075UL, 1961600170UL, 1321557429UL,
		747750121UL, 545747446UL, 810476036UL, 503334515UL, 4088144633UL,
		2824216555UL, 3738252341UL, 3493754131UL, 3672533954UL, 29494241UL,
		1180928407UL, 4213624418UL, 33062851UL, 3221315737UL, 1145213552UL,
		2957984897UL, 4078668503UL, 2262661702UL, 65478801UL, 2527208841UL,
		1960622036UL, 315685891UL, 1196037864UL, 804614524UL, 1421733266UL,
		2017105031UL, 3882325900UL, 810735053UL, 384606609UL, 2393861397UL };
	static int init = 1;
	static unsigned long y[55];
	static int j, k;
	unsigned long ul;

	if (init) {
		int i;
		init = 0;
		for (i = 0; i < 55; i++) y[i] = x[i];
		j = 24 - 1;
		k = 55 - 1;
	}
  	ul = (y[k] += y[j]);
	if (--j < 0) j = 55 - 1;
	if (--k < 0) k = 55 - 1;
	return((unsigned int)ul);
}

BITBOARD Random64(void)
{
	BITBOARD result;
	unsigned int r1, r2;

	r1=Random32();
	r2=Random32();
	result=Or(r1,Shiftl((BITBOARD) r2,32));
	return (result);
}

void InitRandomHash(void)
{
	int i;
	for (i=0;i<64;i++) {
		from_random_1[i] = Random64();
		to_random_1[i] = Random64();
		from_random_2[i] = Random64();
		to_random_2[i] = Random64();
	}
}

void InitTTable(void)
{
	int i, log2tsize;
	ptrtoTT[white] = (TABLE_ENTRY *)malloc(sizeof(TABLE_ENTRY)*TTSIZE);
	ptrtoTT[black] = (TABLE_ENTRY *)malloc(sizeof(TABLE_ENTRY)*TTSIZE);

	ClearTT();

	InitRandomHash();
	HashKey=0LL;
	BD=0LL;
	for  (i=0;i<64;i++) {
		HashKey = Xor(HashKey,from_random_1[i]);
		HashKey = Xor(HashKey,to_random_1[i]);
		BD = Xor(HashKey,from_random_2[i]);
		BD = Xor(HashKey,from_random_2[i]);
	}
	log2tsize = (int) (log(TTSIZE)/log(2));
	hash_mask = (BITBOARD)Shiftl(1,log2tsize)-1;
}

void FreeTTable(void)
{
	free(ptrtoTT[white]);
	free(ptrtoTT[black]);
}

void Initialize(t_Board pos,t_Turn *turn)
{

	int i, j;

	flag.mate = false;
	flag.draw = false;
	move_num = 1;
	Game50 = 1;
	for (i=0;i<144;i++) {
		pos[i].piece = init_pos_pieces[i];
		pos[i].color = init_pos_color[i];
		piece_index_map[i] = null;
		for (j=0;j<144;j++)
			history[i][j] = 0;
	}
#ifndef ARRAY_PIECELIST
	piece_list = (t_SquareIndex **) malloc(2*sizeof(t_SquareIndex *));
	piece_list[0] = (t_SquareIndex *) malloc(17*sizeof(t_SquareIndex));
	piece_list[1] = (t_SquareIndex *) malloc(17*sizeof(t_SquareIndex));
#endif
	for (i=0;i<17;i++) {
		piece_list[white][i] = init_piece_list_white[i];
		piece_list[black][i] = init_piece_list_black[i];
		piece_index_map[piece_list[white][i]] = i;
		piece_index_map[piece_list[black][i]] = i;
	}
	enpassant_square = (t_SquareIndex **) malloc(MAX_MOVES*sizeof(t_SquareIndex *));
	for (i=0;i<MAX_MOVES+1;i++) {
		king_moved[i][white] = 0;
		king_moved[i][black] = 0;
		rook_ks_moved[i][white] = 0;
		rook_ks_moved[i][black] = 0;
		rook_qs_moved[i][white] = 0;
		rook_qs_moved[i][black] = 0;
		enpassant_square[i] =  (t_SquareIndex *) malloc(2*sizeof(t_SquareIndex));
		enpassant_square[i][white] = null;
		enpassant_square[i][black] = null;
	}
	for (i=0;i<MAX_PVSIZE;i++) PV[i].type = null;

	for (i=0;i<8;i++) {
		pawn_struct[white][i] = 1;
		pawn_struct[black][i] = 1;
	}
	castled[white] = castled[black] = false;
	*turn = white;


	material_score = 0;
	material[white] = 0;
	for (i=0;i<17;i++)
		if (pos[piece_list[white][i]].piece != king) material[white] += piece_val[pos[piece_list[white][i]].piece];
	material[black] = material[white];

	/* Initialize random stuff */
	InitTTable();

	ClearKiller();
	ClearHistory();
	PlaceAttacker(pos,white,attack[white]);
	PlaceAttacker(pos,black,attack[black]);

	srand((int)GetTimeInMiliSeconds());
}

void Finish(void)
{
	int i;
	for (i=0;i<MAX_MOVES+1;i++) {
		free(enpassant_square[i]);
	}
	free(enpassant_square);
	free(piece_list[0]);
	free(piece_list[1]);
	free(piece_list);

	FreeTTable();
}
void MakeMove(t_Board pos,t_Move *move,int depth,t_Turn turn)
{
	t_SquareIndex from, to, offset, r_from, r_to;
	int sign;
	t_Turn other_turn;
	t_Piece piece, other_piece, capt_piece;
	t_Color color;
	signed char type = move->type;

	move_hist[depth-1].from = move->from;
	move_hist[depth-1].to = move->to;
	move_hist[depth-1].captured_piece = move->captured_piece;
	move_hist[depth-1].type = move->type;
	move_hist[depth-1].piece_index = move->piece_index;
	move_hist[depth-1].prom_piece = move->prom_piece;

	from = move->from;
	to = move->to;
	piece = pos[from].piece;
	other_piece = pos[to].piece;
	color = pos[from].color;

   	other_turn = other(turn);

	/* Update 50 move rule info */
	if (type == capture || type == short_castle || type == long_castle || piece == pawn) Game50 = depth+1;

	/* Update hash keys */
	HashKey = Xor(HashKey,from_random_1[unmap[from]]);
	HashKey = Xor(HashKey,to_random_1[unmap[to]]);
	BD = Xor(BD,from_random_2[unmap[from]]);
	BD = Xor(BD,to_random_2[unmap[to]]);

	king_moved[depth][white] = king_moved[depth-1][white];
	king_moved[depth][black] = king_moved[depth-1][black];
	rook_ks_moved[depth][white] = rook_ks_moved[depth-1][white];
	rook_qs_moved[depth][black] = rook_qs_moved[depth-1][black];
	enpassant_square[depth][white] = enpassant_square[depth-1][white];
	enpassant_square[depth][black] = enpassant_square[depth-1][black];

	/* Update king and rook status for castling */
	if (piece == king)
		king_moved[depth][turn] = true;
	else if ((piece == rook) && ((from == 33) || (from == 117)))
		rook_ks_moved[depth][turn] = true;
  	else if ((piece == rook) && ((from == 26) || (from == 110)))
		rook_qs_moved[depth][turn] = true;
	else if (pos[to].piece == rook && ((to == 33) || (to == 117)))
		rook_ks_moved[depth][other_turn] = true;
	else if (pos[to].piece == rook && ((to == 26) || (to == 110)))
		rook_qs_moved[depth][other_turn] = true;

	/* Update pawn structure */
	if (piece == pawn) {
		if (move->captured_piece != null) {
			pawn_struct[turn][rank(from)-1]--;
			if (move->type != promotion)
				pawn_struct[turn][rank(to)-1]++;
		}
		else if (move->type == promotion) {

			pawn_struct[turn][rank(from)-1]--;
		}
	}
	if (other_piece == pawn) {
		pawn_struct[other_turn][rank(to)-1]--;
	}

	/* Update piece list array */
    //if (piece_index_map[from] == -1) {
    //    printf("piece_index_map is -1");
    //}
    //else if (piece_index_map[from] == 16)
    //    printf("piece_index_map is 16");
    
        
	piece_list[(int)turn][(int)piece_index_map[from]] = to;


	/* Update enpassant status and piece information if move is capture */
	enpassant_square[depth][other_turn] = null;

	if (move->type == capture || (move->type == promotion && move->captured_piece != null)) {
		capt_piece = move->captured_piece;
		piece_list[(int)other_turn][(int)piece_index_map[to]] = empty;
		move->piece_index = piece_index_map[to];
		sign = (pos[to].color == black ? 1 : -1);
		/*if (depth != MAX_MOVES && (capt_piece < pawn || capt_piece >= king))
			printf("telluser str: Potential error: captured illegal piece: %d %s quiescent generate\n",capt_piece,(depth == MAX_MOVES?"":"not"));*/
		material_score += (piece_val[capt_piece]*sign);
		material[other_turn]-=piece_val[capt_piece];
	}
	else if (move->type == enpassant) {
		sign = (pos[from].color == white ? 1 : -1);
		piece_list[(int)other_turn][(int)piece_index_map[to-12*sign]] = empty;
		move->piece_index = piece_index_map[to-12*sign];
		piece_index_map[to-12*sign] = null;
		material_score += (piece_val[pawn]*sign);
		material[other_turn]-=piece_val[pawn];
		pos[to-12*sign].color = empty;
		pos[to-12*sign].piece = empty;
	}
	else if ((to-from) == 24 && file(from) == 2 && piece == pawn && turn == white) {
		enpassant_square[depth][black] = from+12;
	}
	else if ((from-to) == 24 && file(from) == 7 && piece == pawn && turn == black) {
		enpassant_square[depth][white] = from-12;
	}


	/* Update piece location index map (contains in each element
       the index in the piece_list array where the piece in the square
	   corresponding to that element is located) */
	piece_index_map[to] = piece_index_map[from];
	piece_index_map[from] = null;

	/* Update board position */
	pos[to].color = pos[from].color;
	pos[to].piece = pos[from].piece;
	pos[from].color = empty;
	pos[from].piece = empty;

	/* Update information in case of castling move */
	if (move->type == short_castle) {
		king_moved[depth][turn] = true;
		castled[turn] = true;
		offset = 0;
		if (turn == black) offset = 84;
		r_from = 33+offset;
		r_to = 31+offset;
		piece_list[(int)turn][(int)piece_index_map[r_from]] = r_to;
		piece_index_map[r_to] = piece_index_map[r_from];
		piece_index_map[r_from] = null;
		pos[r_to].color = pos[r_from].color;
		pos[r_to].piece = pos[r_from].piece;
		pos[r_from].color = empty;
		pos[r_from].piece = empty;
		rook_ks_moved[depth][turn] = true;

	}
	else if (move->type == long_castle) {
		king_moved[depth][turn] = true;
		castled[turn] = true;
		offset = 0;
		if (turn == black) offset = 84;
		r_from = 26+offset;
		r_to = 29+offset;
		piece_list[(int)turn][(int)piece_index_map[r_from]] = r_to;
		piece_index_map[r_to] = piece_index_map[r_from];
		piece_index_map[r_from] = null;
		pos[r_to].color = pos[r_from].color;
		pos[r_to].piece = pos[r_from].piece;
		pos[r_from].color = empty;
		pos[r_from].piece = empty;
		rook_qs_moved[depth][turn] = true;
	}
	/* Update information in case of promotion move */
	else if (move->type == promotion) {
		sign = (pos[to].color == white ? 1 : -1);
		pos[to].piece = move->prom_piece;
		material_score += sign*(piece_val[move->prom_piece]-piece_val[pawn]);
		material[turn] += piece_val[move->prom_piece]-piece_val[pawn];
	}

}

void UnMakeMove(t_Board pos,t_Move *move,int depth,t_Turn turn)
{
	t_SquareIndex from, to, offset, r_from, r_to;
	int sign;
	t_Turn other_turn;

	/*move_hist[depth-1].from = null;
	move_hist[depth-1].to = null;
	move_hist[depth-1].captured_piece = null;
	move_hist[depth-1].type = null;
	move_hist[depth-1].piece_index = null;*/

	from = move->from;
	to = move->to;

	/* Undo hash keys updates */
	HashKey = Xor(HashKey,from_random_1[unmap[from]]);
	HashKey = Xor(HashKey,to_random_1[unmap[to]]);
	BD = Xor(BD,from_random_2[unmap[from]]);
	BD = Xor(BD,to_random_2[unmap[to]]);

	king_moved[depth][white] = 0;
	king_moved[depth][black] = 0;
	rook_ks_moved[depth][white] = 0;
	rook_ks_moved[depth][black] = 0;
	rook_qs_moved[depth][white] = 0;
	rook_qs_moved[depth][black] = 0;
	enpassant_square[depth][white] = null;
	enpassant_square[depth][black] = null;


	other_turn = other(turn);
	piece_list[(int)turn][(int)piece_index_map[to]] = from;
	if (move->type == capture || (move->type == promotion && move->captured_piece != null)) {
		piece_list[(int)other_turn][(int)move->piece_index] = to;
		sign = (pos[to].color == white ? 1 : -1);
		material_score -= (piece_val[move->captured_piece]*sign);
		material[other_turn]+=piece_val[move->captured_piece];
	}
	else if (move->type == enpassant) {
		sign = (pos[to].color == white ? 1 : -1);
		piece_list[(int)other_turn][(int)move->piece_index] = to-12*sign;
		material_score -= (piece_val[pawn]*sign);
		material[other_turn]+=piece_val[pawn];
		piece_index_map[to-12*sign] = move->piece_index;
		pos[to-12*sign].color = other_turn;
		pos[to-12*sign].piece = pawn;
	}

	piece_index_map[from] = piece_index_map[to];
	if (move->captured_piece == null)
		piece_index_map[to] = null;
	else
		piece_index_map[to] = move->piece_index;

	pos[from].color = pos[to].color;
	pos[from].piece = pos[to].piece;
	if (move->captured_piece == null) {
		pos[to].color = empty;
		pos[to].piece = empty;
	}
	else {
		pos[to].color = other_turn;
		pos[to].piece = move->captured_piece;
	}


	if (move->type == short_castle) {
		castled[turn] = false;
		offset = 0;
		if (turn == black) offset = 84;
		r_from = 33+offset;
		r_to = 31+offset;
		piece_list[(int)turn][(int)piece_index_map[r_to]] = r_from;
		piece_index_map[r_from] = piece_index_map[r_to];
		piece_index_map[r_to] = null;
		pos[r_from].color = pos[r_to].color;
		pos[r_from].piece = pos[r_to].piece;
		pos[r_to].color = empty;
		pos[r_to].piece = empty;


	}
	else if (move->type == long_castle) {
		castled[turn] = false;
		offset = 0;
		if (turn == black) offset = 84;
		r_from = 26+offset;
		r_to = 29+offset;
		piece_list[(int)turn][(int)piece_index_map[r_to]] = r_from;
		piece_index_map[r_from] = piece_index_map[r_to];
		piece_index_map[r_to] = null;
		pos[r_from].color = pos[r_to].color;
		pos[r_from].piece = pos[r_to].piece;
		pos[r_to].color = empty;
		pos[r_to].piece = empty;
	}
	else if (move->type == promotion) {
		sign = (pos[from].color == white ? 1 : -1);
		pos[from].piece = pawn;
		material_score -= sign*(piece_val[move->prom_piece]-piece_val[pawn]);
		material[turn]-=(piece_val[move->prom_piece]-piece_val[pawn]);
	}

	if (pos[from].piece == pawn) {
		if (move->captured_piece != null && move->type != promotion) {
			pawn_struct[turn][rank(from)-1]++;
			pawn_struct[turn][rank(to)-1]--;
		}
		else if (move->type == promotion) {
			pawn_struct[turn][rank(from)-1]++;
		}
	}
	if (pos[to].piece == pawn) {
		pawn_struct[other_turn][rank(to)-1]++;
	}
}

/*****************
*
* IsLegalMove returns true if a given move in a given position is
* legal and false if it is illegal.
*
******************/
int IsLegalMove(t_Board pos,t_Move move,t_Turn last_turn)
{
	t_Move move_list[100];
	t_Turn turn;
	int num_moves, i, j, pseudolegal_move;
	signed char sign;
	t_SquareIndex to, sqaure;
	t_Piece piece;

	/* Check if the move is in the generated move list for the given position
	   If not, the move is illegal. If it is, other checks must be done like
	   the fact that the king may be left in check or if the move is a castle,
	   check if it legal. A move that is in the generated move list is called
	   a pseudo legal move */
	pseudolegal_move = false;
	num_moves = GenerateMoves(pos,last_turn,move_list,move_num,false);
	for (i=0;i<num_moves;i++) {
		if (move_list[i].type == short_castle || move_list[i].type == long_castle) {
			if (move.type == move_list[i].type) return true;

		}
		if (move.from == move_list[i].from && move.to == move_list[i].to) {
			pseudolegal_move = true;
			break;
		}
	}
	if (!pseudolegal_move) return false;

	/* Now we're gonna check if the king can be captured by the opponent's move
	   If this is so the move is illegal because the corresponding piece was pinned */

	MakeMove(pos,&move,move_num,last_turn);
	turn = other(last_turn);

	num_moves = GenerateMoves(pos,turn,move_list,move_num+1,false);
	for (i=0;i<num_moves;i++) {
		if (move_list[i].captured_piece == king) {
			UnMakeMove(pos,&move,move_num,last_turn);
			return false;
		}
	}
	UnMakeMove(pos,&move,move_num,last_turn);

	return true;
}

/*************
*
* ParseMove parses the move in the string s and codes it into the internal move
* structure t_Move move. If the parse was successful the function returns nonzero.
* If it wasn't successful it returns false, meaning that the entered text was not
* a move.
*
**************/
int ParseMove(char *s,t_Move *move,t_Board pos,t_Turn turn)
{
	t_SquareIndex from, to;
	int type, castle_type;
	char prom_str[] = "nbrqNBRQ", *ptr, ch;

	while (*s == ' ') s++;

	if (*s >= 'a' && *s <= 'h') {
		from = 26+*s-'a';
		s++;
		if (*s >= '1' && *s <= '8') {
			from += 12*(*s - '1');
			type = normal;
			s++;
		}
		else return false;
	}

	else if (*s == '0' || *s == 'o' || *s == 'O') {
		type = short_castle;
		s++;
	}

	else return false;

	switch (type) {

		case normal:
			while (*s == ' ' || *s == '-') s++;
			if (*s >= '1' && *s <= 'h') {
				to = 26+*s-'a';
				s++;
				if (*s >= '1' && *s <= '8') {
					to += 12*(*s - '1');
					move->from = from;
					move->to = to;
					if (from == 30 && to == 32 && pos[from].piece == king && pos[from].color == white) {
						move->captured_piece = null;
						move->type = short_castle;
						return true;
					}
					else if (from == 30 && to == 28 && pos[from].piece == king && pos[from].color == white) {
						move->captured_piece = null;
						move->type = long_castle;
						return true;
					}
					else if (from == 114 && to == 116 && pos[from].piece == king && pos[from].color == black) {
						move->captured_piece = null;
						move->type = short_castle;
						return true;
					}
					else if (from == 114 && to == 112 && pos[from].piece == king && pos[from].color == black) {
						move->captured_piece = null;
						move->type = long_castle;
						return true;
					}



					if (pos[to].color == other(turn)) {
						move->captured_piece = pos[to].piece;
						move->type = capture;
					}
					else {
						move->captured_piece = null;
						move->type = normal;
					}
					if (file(from) == (turn == white ? 7 : 2) && pos[from].piece == pawn) {
						move->type = promotion;
						s++;
						while (*s == ' ') s++;
						if (*s != '\0' && (ptr = strchr(prom_str,*s)) != NULL) {
							s=prom_str;
							move->prom_piece = 2+(ptr-s)%4;
						}
						else
							move->prom_piece = queen;
					}

					if (((to-from) == 11 || (to-from) == 13 || (to-from) == -11 || (to-from) == -13) && pos[from].piece == pawn && pos[to].color == empty) {
						move->type = enpassant;


					}
					return true;
				}
			}
			else return false;
			break;

		case short_castle:
			while (*s == ' ') s++;
			if (*s == '-') s++;
			while (*s == ' ') s++;
			if (*s == 'o' || *s == 'O' || *s == '0') {
				castle_type = short_castle;
				s++;
				while (*s == ' ') s++;
				if (*s == '-') s++;
				while (*s == ' ') s++;
				if (*s == 'o' || *s == 'O' || *s == '0')
					castle_type = long_castle;

				move->type = castle_type;
				if (castle_type == short_castle) {
					move->from = (turn == white ? 30 : 114);
					move->to = (turn == white ? 32 : 116);
					move->captured_piece = null;
					return true;
				}
				else if (castle_type == long_castle) {
					move->from = (turn == white ? 30 : 114);
					move->to = (turn == white ? 28 : 112);
					move->captured_piece = null;
					return true;
				}
			}
			break;
	}
	return false;

}

void PrintBoard(t_Board pos, t_Turn side)
{

	int i, j;
	t_Color color;

	printf("+---+---+---+---+---+---+---+---+\n");
	for (i=110;i>24;i-=12) {

		for (j=0;j<8;j++) {
			printf("| ");
			color = pos[i+j].color;
			switch (pos[i+j].piece) {
				case empty:
					printf("  ");
					break;
				case pawn:
					printf("%s ",(color == white ? "P" : "p"));
					break;
				case knight:
					printf("%s ",(color == white ? "N" : "n"));
					break;
				case bishop:
					printf("%s ",(color == white ? "B" : "b"));
					break;
				case rook:
					printf("%s ",(color == white ? "R" : "r"));
					break;
				case queen:
					printf("%s ",(color == white ? "Q" : "q"));
					break;
				case king:
					printf("%s ",(color == white ? "K" : "k"));
					break;
				default:
					break;
			}

		}

		printf("|\n+---+---+---+---+---+---+---+---+\n");
	}
	printf("\n");

}

void SetPosition(t_Board setpos,t_Turn turn,int *kcastle,int *qcastle,int ep_square,int rule_50_move,int m_num)
{
	int i, wi, bi;
	t_SquareIndex sq, x, y;
	t_Board pos;
	t_Turn *t, color;
	Finish();

	pos = GetCurrentPos();
	t = &turn;
	Initialize(pos,t);
	memcpy(pos,setpos,144*sizeof(t_Square));
	SetTurn(turn);
	move_num = m_num;
	if (!kcastle[white]) for (i=0;i<MAX_MOVES;i++) rook_ks_moved[i][white] = true;
	else for (i=0;i<m_num;i++) rook_ks_moved[i][white] = false;
	if (!kcastle[black]) for (i=0;i<MAX_MOVES;i++) rook_ks_moved[i][black] = true;
	else for (i=0;i<m_num;i++) rook_ks_moved[i][black] = false;
	if (!qcastle[white]) for (i=0;i<MAX_MOVES;i++) rook_qs_moved[i][white] = true;
	else for (i=0;i<m_num;i++) rook_qs_moved[i][white] = false;
	if (!qcastle[black]) for (i=0;i<MAX_MOVES;i++) rook_qs_moved[i][black] = true;
	else for (i=0;i<m_num;i++) rook_qs_moved[i][black] = false;
	if (ep_square > 25) enpassant_square[move_num][turn] = ep_square;
	Game50 = rule_50_move;
	flag.opponent = turn;

	PrintBoard(pos,white);

	wi = 1;
	bi = 1;
	for (i=0;i<8;i++) {
		pawn_struct[white][i] = 0;
		pawn_struct[black][i] = 0;
	}
	for (y=0;y<8;y++) for (x=0;x<8;x++) {
		sq = 26+x+12*y;
		color = pos[sq].color;
		if (pos[sq].piece != empty) {
			if (pos[sq].piece == king) {
				piece_list[color][0] = sq;
				piece_index_map[sq] = 0;
			}
			else {
				i = (color == white ? wi : bi);
				piece_list[color][i] = sq;
				piece_index_map[sq] = i;
				if (pos[sq].piece == pawn) {
					pawn_struct[color][rank(sq)-1]++;
				}

				if (color == white) wi++; else bi++;

			}
		}
		else {
			/*piece_list[white][wi] = empty;
			piece_list[black][bi] = empty;*/
			piece_index_map[sq] = null;
		}
	}
	piece_list[white][wi] = null;
	piece_list[black][bi] = null;

	for (y=0;y<12;y++) {
		for (x=0;x<12;x++) {
			printf("%d%d ",pos[12*y+x].piece,pos[12*y+x].color);
		}
		printf("\n");
	}
	piece_list[white][wi] = null;
	piece_list[black][bi] = null;
	for (i=0;i<17;i++) printf("%d ",piece_list[white][i]); printf("\n");
	for (i=0;i<17;i++) printf("%d ",piece_list[black][i]); printf("\n");
	for (y=0;y<8;y++) {
		for (x=0;x<8;x++) {
			sq = 26+12*y+x;
			printf("%d ",piece_index_map[sq]);
		}
		printf("\n");
	}
	PlaceAttacker(pos,white,attack[white]);
	PlaceAttacker(pos,black,attack[white]);

	ClearHistory();
	ClearKiller();
}


typedef unsigned long long u64;

DWORD Perft(t_Board pos, t_Turn turn, int depth)
{
    t_Move move_list[256];
    int n_moves, i;
    u64 nodes = 0;
    
    if (depth == 0) return 1;
    
    n_moves = GenerateMoves(pos,turn,move_list,move_num,IsInCheck(pos,turn));
    for (i = 0; i < n_moves; i++) {
        MakeMove(pos,&move_list[i],move_num,turn);
        nodes += Perft(pos, other(turn), depth-1);
        UnMakeMove(pos,&move_list[i],move_num,turn);
    }
    return nodes;
}