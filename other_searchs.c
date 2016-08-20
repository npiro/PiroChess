
int AlphaBetaWithMemory(t_Board pos,t_Turn turn,int ply,int depth,int alpha,int beta,t_Move best_line[])
{
	int i, p, num_moves, score, best=-INFINITY, alpha_new = alpha, beta_new = beta;
	int ttDepth;
	int g, a, b;
	t_Move ttMove;
	int ttScore, ttUpperBound, ttLowerBound, ttType;
	t_Turn other_turn = other(turn);
	t_Move move_list[200];
	t_Move next_line[MAX_PVSIZE];

	/*best_line[ply+1].type = null;*/

	nodes++;


	if (LookupTT(&ttDepth,&ttMove,&ttScore,&ttUpperBound,&ttLowerBound,&ttType,turn)) {
		tt_probes++;
		if (ttDepth >= depth-ply ) {
			tt_hits++;

			if (ttLowerBound >= beta) return ttLowerBound;
			if (ttUpperBound <= alpha) return ttUpperBound;
			alpha_new = MAX(alpha,ttLowerBound);
			beta_new = MIN(beta,ttUpperBound);
		}
	}

	if (IsInCheck(pos,turn)) {
		if ((num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,true)) == 0) return -10000+ply;
	}
	else {
		if (ply >= depth) {
			/*return QuiescenseSearch(pos,turn,ply,depth,alpha_new,beta_new,best_line);*/
			return Evaluate(pos,other_turn);

		}

		num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,false);

	}
	/*score = -AlphaBeta (pos,other_turn,ply,depth-1,-beta_new,-alpha_new,next_line);
	if (score >= beta) {


		return score;
	}*/

	i = 0;
	if (turn == computer_player) {
		g = -INFINITY;
		a = alpha_new;
		while (g < beta_new && i < num_moves) {
			MakeMove(pos,&move_list[i],move_num+ply,turn);
			if (IsKingAttacked(pos,turn)) {
				move_list[i].score = -INFINITY;
				UnMakeMove(pos,&move_list[i],move_num+ply,turn);
				i++;
				continue;
			}

			score = AlphaBetaWithMemory(pos,other_turn,ply+1,depth, a, beta_new,next_line);
			UnMakeMove(pos,&move_list[i],move_num+ply,turn);
			g = MAX(g,score);
			a = MAX(a,g);
			i++;
		}
	}
	else {
		g = +INFINITY;
		b = beta_new;
		while (g > alpha_new && i < num_moves) {
			MakeMove(pos,&move_list[i],move_num+ply,turn);

			if (IsKingAttacked(pos,turn)) {
				move_list[i].score = -INFINITY;
				UnMakeMove(pos,&move_list[i],move_num+ply,turn);
				i++;
				continue;
			}

			score = AlphaBetaWithMemory(pos,other_turn,ply+1,depth, alpha_new,b,next_line);
			UnMakeMove(pos,&move_list[i],move_num+ply,turn);
			g = MIN(g,score);
			b = MIN(b,g);

			i++;
		}
	}
	if (g <= alpha_new)
		StoreInTT(depth-ply,best_line[ply],best,g,beta_new,normal_move,turn);
	/* Found an accurate minimax value - will not occur if called with zero window */
	if (g > alpha_new && g < beta_new)
		StoreInTT(depth-ply,best_line[ply],best,g,g,normal_move,turn);

	/* Fail high result implies a lower bound */

	if (g >= beta_new)
		StoreInTT(depth-ply,best_line[ply],best,alpha_new,g,normal_move,turn);

	return g;
}


int MTDF(t_Board pos,t_Turn turn,int ply,int depth,int f,t_Move best_line[])
{
	int upperbound, lowerbound, g, beta;

	g = f;

	upperbound = +INFINITY;
	lowerbound = -INFINITY;

	while (lowerbound < upperbound) {
		if (g == lowerbound)
			beta = g + 5;
		else
			beta = g;

		g = AlphaBetaWithMemory(pos,turn,ply,depth,beta - 10,beta,best_line);

		if (g < beta)
			upperbound = g;
		else lowerbound = g;
	}

	return g;
}

int NegaScout2(t_Board pos,t_Turn turn,int ply,int depth,int alpha,int beta,int do_null_move,t_Move best_line[])
{
	int i, num_moves, p;
	int current, score, in_check, eval_is_exact, from, to;
	int ttDepth, ttScore, ttAlpha, ttBeta, ttType;
	t_Move ttMove;
	t_Move move_list[200];
	t_Turn other_turn = other(turn);
	t_Move next_line[MAX_PVSIZE];

	try_tt_move = false;
	if (LookupTT(&ttDepth,&ttMove,&ttScore,&ttAlpha,&ttBeta,&ttType,turn)) {
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
	}

	if (in_check = IsInCheck(pos,turn)) {
		if ((num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,true)) == 0) return -10000+ply;
	}
	else {
		if (ply >= depth) {
			return QuiescenseSearch(pos,turn,ply,depth,-beta,-alpha,best_line);

		}
		num_moves = GenerateMoves(pos,turn,move_list,move_num+ply,false);

	}

	eval_is_exact = false;

	from = move_list[0].from;
	to = move_list[0].to;
	if (pos[to].color == other_turn && pos[to].piece == king) {
		return piece_val[king];
	}
	MakeMove(pos,&move_list[0],move_num+ply,turn);
	PlaceAttacker(pos,turn,attack[turn]);
	PlaceAttacker(pos,other_turn,attack[other_turn]);
	current = -NegaScout2(pos,other_turn,ply+1,depth,-beta,-alpha,true,next_line);
	UnMakeMove(pos,&move_list[0],move_num+ply,turn);

	searching_pv = false;

	for (i=1;i<num_moves;i++) {
		from = move_list[i].from;
		to = move_list[i].to;
		if (pos[to].color == other_turn && pos[to].piece == king) {
			return piece_val[king];
		}

		MakeMove(pos,&move_list[i],move_num+ply,turn);
		PlaceAttacker(pos,turn,attack[turn]);
		PlaceAttacker(pos,other_turn,attack[other_turn]);
		memcpy(old_attack,attack,288*sizeof(t_SquareIndex));
		memcpy(old_nattacks,num_attackers,288*sizeof(int));

		score = -NegaScout2(pos,other_turn,ply+1,depth,-alpha-1,-alpha,true,next_line);
		if (score > alpha && score < beta) {
			memcpy(attack,old_attack,288*sizeof(t_SquareIndex));
			memcpy(num_attackers,old_nattacks,288*sizeof(int));
			score = -NegaScout2(pos,other_turn,ply+1,depth,-beta,-alpha,true,next_line);
		}
		UnMakeMove(pos,&move_list[i],move_num+ply,turn);
		if (score >= current) {
			current = score;
			for (p=ply;next_line[p-1].type != null;p++) {
				best_line[p] = next_line[p];
			}
			best_line[p+1].type = null;

			best_line[ply] = move_list[i];

			if (killer2[move_num+ply].from != move_list[i].from &&
				killer2[move_num+ply].to != move_list[i].to) {
				killer3[move_num+ply] = killer2[move_num+ply];
				killer2[move_num+ply] = move_list[i];
			}
			if (score >= alpha) {
				eval_is_exact = true;
				alpha = score;
			}
			if (score >= beta) {
				killer[move_num+ply] = move_list[i];
				StoreInTT(move_num+depth-ply,best_line[ply],current,alpha,beta,fail_high,turn);
				break;
			}
        }
	}
	history[best_line[ply].from][best_line[ply].to]++;
	if (score >= beta) return ( score );

	StoreInTT(move_num+depth-ply,best_line[ply],current,alpha,beta,eval_is_exact?normal_move:fail_low,turn);

	if (current == -piece_val[king]) return -10000+ply;

	return current;
}

