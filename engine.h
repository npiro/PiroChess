#ifndef _ENGINE_H
#define _ENGINE_H

/* Constants and macros */

#ifndef DWORD
#define DWORD unsigned long
#endif

#ifdef __MACH__
#include <mach/mach.h>
#include <mach/mach_time.h>
#define GetTickCount() mach_absolute_time()
#endif

#undef INFINITY
#define INFINITY 32767
#define MAX_QUIESCENSE_DEPTH 15
#define MAX_MOVES 400
#define MAX_PVSIZE 400
#define DEFAULT_MAX_DEPTH 15

//#define TTSIZE	131072
#define TTSIZE  1048576

#define KILLERMV1_SCORE	25			/* old 150 */
#define	KILLERMV2_SCORE	5			/* old 75 */
#define KILLERMV3_SCORE 0			/* old 50 */
/*#define HISTORY_BONUS (float)1.25/((float)ply*ply+1.)*/		/* old 2 */
#define HISTORY_BONUS 500./(float)nodes;
#define PV_SCORE 	30000
#define TTMOVE_SCORE	18000

#define a1	26
#define b1	27
#define c1	28
#define d1	29
#define e1	30
#define f1	31
#define g1	32
#define h1	33
#define a2	38
#define b2	39
#define c2	40
#define d2	41
#define e2	42
#define f2	43
#define g2	44
#define h2	45
#define a3	50
#define b3	51
#define c3	52
#define d3	53
#define e3	54
#define f3	55
#define g3	56
#define h3	57
#define a4	62
#define b4	63
#define c4	64
#define d4	65
#define e4	66
#define f4	67
#define g4	68
#define h4	69
#define a5	74
#define b5	75
#define c5	76
#define d5	77
#define e5	78
#define f5	79
#define g5	80
#define h5	81
#define a6	86
#define b6	87
#define c6	88
#define d6	89
#define e6	90
#define f6	91
#define g6	92
#define h6	93
#define a7	98
#define b7	99
#define c7	100
#define d7	101
#define e7	102
#define f7	103
#define g7	104
#define h7	105
#define a8	110
#define b8	111
#define c8	112
#define d8	113
#define e8	114
#define f8	115
#define g8	116
#define h8	117

#define pawn 1
#define knight 2
#define bishop 3
#define rook 4
#define queen 5
#define king 6
#define null -1
#define empty 8

#define white 0
#define black 1

#define true 1
#define false 0

#define normal_move 1
#define fail_high 2
#define fail_low 3

/* Move type */
#define short_castle 1    /* Binary 0001 */
#define long_castle 2     /* Binary 0010 */
#define normal 3
#define capture 4
#define enpassant 5
#define promotion 6

#define rank(sq) ((sq % 12) - 1)
#define file(sq) ((int)(sq / 12) - 1)
#define other(turn) (turn == white ? black : white)
#define MAX(a,b) (a > b ? a : b)
#define MIN(a,b) (a < b ? a : b)

#define		And(a,b)        (a & b)
#define		Or(a,b)         (a | b)
#define		Xor(a,b)        (a ^ b)
#define		Shiftl(a,b)     (a << b)
#define		Shiftr(a,b)     (a >> b)

/* Data type definitions */
typedef int t_Piece;
typedef int t_Color;
typedef unsigned char t_Turn;

#ifdef GCC
typedef long long BITBOARD;
#else
typedef __int64_t BITBOARD;
#endif

typedef  int t_SquareIndex;
typedef t_SquareIndex *t_PieceList;

typedef struct {
	t_Piece piece;
	t_Color color;
	unsigned char is_enpassant_square;
} t_Square;
typedef t_Square *t_Board;


typedef struct {
	unsigned char from;
	unsigned char to;
	t_Piece captured_piece;
	signed char type;
	t_Piece prom_piece;
	t_SquareIndex piece_index; /* This field is used by MakeMove and UnMakeMove */
	int score; /* This is used by the search algorithm to sort the moves by score */
} t_Move;

typedef struct _TABLE_ENTRY {
	int depth;
	int from;
	int to;
	int score;
	int upperbound, lowerbound;
	unsigned short flag;
	int type;
	BITBOARD bd;			/* Position secondary hash key
							   to avoid collition */
	BITBOARD hash_key;		/* Table entry hash key */
} TABLE_ENTRY;

/* Declaration of global functions */


void PlaceAttacker(t_Board,t_Turn ,t_SquareIndex *);


uint64_t GetTimeInMiliSeconds(void);
int IsInCheck(t_Board,t_Turn);
void Initialize(t_Board,t_Turn *);
void Finish(void);
int GenerateMoves(t_Board,t_Turn,t_Move *,int,int);
void MakeMove(t_Board,t_Move *,int,t_Turn);
void UnMakeMove(t_Board,t_Move *,int,t_Turn);
int Evaluate(t_Board,t_Turn);
int SelectMove(t_Board,t_Move *,int,t_Turn);
int IsLegalMove(t_Board,t_Move,t_Turn);
DWORD Perft(t_Board pos, t_Turn turn, int depth);

int ParseMove(char *,t_Move *,t_Board,t_Turn);
void PrintBoard(t_Board,t_Turn);

void SetPosition(t_Board,t_Turn,int *,int *,int,int,int);
t_Board GetCurrentPos(void);
t_Turn *GetTurnPtr(void);
void SetTurn(t_Turn);
#endif
