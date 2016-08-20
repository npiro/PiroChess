#ifndef _MYCHESS_H
#define _MYCHESS_H
#include "engine.h"

	extern t_Move move_hist[];
	extern t_Turn turn;
	extern int move_num;
	extern t_SquareIndex attack[2][144];
	extern int num_attackers[2][144];
	extern int pawn_struct[2][8];
	extern int comp_time;
	extern int search_depth;

#endif
