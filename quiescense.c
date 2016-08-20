int GenerateCapturesToSquare(t_Board pos,t_Turn turn,t_Move *move_list,t_SquareIndex to)
{
	int i, d, sign, num_moves=0;
	t_SquareIndex from;
	t_Piece piece;
	t_Color color;
	sgn = sign[turn];
	t_Turn other_turn = other(turn);

	/* Generate pawn captures to square given by to */
	sign = (turn == white ? 1 : -1);

	/* Check for pawn captures on the to square */

	for (i=11;i<=13;i+=2) {
		from = to-i*sign;
		if (pos[from].piece == pawn && pos[from].color == other_turn) {
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
		if (pos[from].piece == knight && pos[from].color == other_turn) {
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
				if (pos[from].color == other_turn && (pos[from].piece == bishop ||
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
			from = square+d*i;
			if (pos[from].color == other_turn && (pos[from].piece == rook ||
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
			from = square+12*d*i;
			if (pos[from].color == other_turn && (pos[from].piece == rook ||
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

		return num_moves;
	}

}

int SwapSearch(t_Board pos,t_Turn turn)
{
	int num_moves, i, score;
	t_Move move_list[30];

	best = (turn == white ? material_score : -material_score);

	num_moves = GenerateCapturesToSquare(pos,turn,move_list,to);

	for (i=0;i<num_moves;i++) {
		MakeMove(pos,&move_list[i],MAX_MOVES,turn);
		score = -SwapSearch(pos,other(turn));
		UnMakeMove(pos,&move_list[i],MAX_MOVES,turn);
		if (score > best) best = score;
	}
	return best;
}

int SortCaptures(t_Board pos,t_Turn turn,int depth,int ply,t_Move *move_list,int num_moves)
{
	int i, ply = 0;
	t_Piece piece;
	t_SquareIndex from, to;

	for (i=0;i<num_moves;i++) {
		from = move_list[i].from;
		to = move_list[i].to;
		MakeMove(pos,&move_list[i],MAX_MOVES,turn);
		move_list[i].score = -SwapSearch(pos,other(turn));
		UnMakeMove(pos,&move_list[i],MAX_MOVES,turn);
	}

	qsort(move_list,num_moves,sizeof(t_Move),Comp);
}


