//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "command.h"

extern int pawn_struct[2][8], attack[2][144];
extern int num_attackers[2][144];
extern int comp_time, uci;

int move_num;
int search_depth = DEFAULT_MAX_DEPTH;
t_Turn turn;
t_Move move_hist[400];
t_Square pos[144];
t_Flag flag;

void Usage(char *programName)
{
	fprintf(stderr,"%s usage:\n",programName);
	/* Modify here to add your usage message when the program is
	 * called without arguments */
}

/* returns the index of the first argument that is not an option; i.e.
   does not start with a dash or a slash
*/
int HandleOptions(int argc,char *argv[])
{
	int i,firstnonoption=0;

	for (i=1; i< argc;i++) {
		if (argv[i][0] == '/' || argv[i][0] == '-') {
			switch (argv[i][1]) {
				/* An argument -? means help is requested */
				case '?':
					Usage(argv[0]);
					break;
				case 'h':
				case 'H':
					if (!strcmp(argv[i]+1,"help")) {
						Usage(argv[0]);
						break;
					}
					/* If the option -h means anything else
					 * in your application add code here
					 * Note: this falls through to the default
					 * to print an "unknow option" message
					*/
				/* add your option switches here */
				default:
					fprintf(stderr,"unknown option %s\n",argv[i]);
					break;
			}
		}
		else {
			firstnonoption = i;
			break;
		}
	}
	return firstnonoption;
}

int main(int argc,char *argv[])
{
	int num_moves, i, temp, score, legal_move = false;
	int m, n, at;
	DWORD elapsed_time, initial_time;
#define COMMAND_MAX_LENGTH 1024
	char str[COMMAND_MAX_LENGTH], *p;

	t_Move move_list[200], move;

	if (argc > 1) {
		/* If there are arguments we handle them */
		HandleOptions(argc,argv);
	}


	flag.quit = false;
	flag.timeout = false;
	flag.computer_plays = true;
	flag.opponent = white;
	flag.autosave = true;
	flag.draw = false;
	flag.mate = false;

	/* Initialize chess engine */

	Initialize(pos,&turn);

	comp_time = 300;

	setbuf(stdout, NULL);
	setbuf(stdin, NULL);

	while (1) {
        //fflush(stdin);
#ifndef XBOARD

		if (!flag.mate && !flag.draw && !uci)
			PrintBoard(pos,white);

#endif
		if (flag.quit) {
            printf("telluser quiting 1");
			Finish();
			return 0;
		}


		if (turn == flag.opponent) {
#ifndef XBOARD
            if (!uci)
                printf("mychess> ");
#endif
			
            fgets(str, COMMAND_MAX_LENGTH, stdin);
            //gets(str);
			//fflush(stdin);
			//printf("telluser str: %s\n",str);

			if (CheckCommand(str)) {
                
                if (flag.quit) {
                    printf("telluser quitting 2\n");
                    return 0;
                }
				continue;
			}

			if (flag.mate || flag.draw) continue;

			if (!ParseMove(str,&move,pos,turn)) {
#ifndef XBOARD
				printf("Syntax error\n\n");
#endif
				continue;
			}
			else if ((legal_move = IsLegalMove(pos,move,turn)) == 0) {
#ifndef XBOARD
				printf("Illegal move: %c%d-%c%d\n",'a'+rank(move.from)-1,file(move.from),
				'a'+rank(move.to)-1,file(move.to));
#endif
				continue;
			}
			if (legal_move) {
				MakeMove(pos,&move,move_num,turn);
				PlaceAttacker(pos,turn,attack[turn]);
				PlaceAttacker(pos,other(turn),attack[other(turn)]);
				move_num++;

				turn = other(turn);
				if (!flag.computer_plays) flag.opponent = turn;
				legal_move = false;
			}
			continue;
		}

		else {			/* turn != opponent */
            initial_time = GetTimeInMiliSeconds();
			score = SelectMove(pos,&move,search_depth,turn);
			if (move.type == promotion)
				printf("promotion: from: %d to: %d turn: %d promote piece: %d\n",move.from,move.to,turn,move.prom_piece);
			if (score == -10000) {
				printf("Checkmate: You win!!\n");
				flag.mate = true;
				turn = other(turn);
				continue;
			}
			else if (score == 9999) {
				printf("Checkmate: Computer wins\n");
				flag.mate = true;
			}
			else if (flag.draw) {
				printf("Draw!!\n");
				printf("flag.draw: %d\n",flag.draw);
				turn = other(turn);
				continue;
			}
			elapsed_time = GetTimeInMiliSeconds()-initial_time;
			comp_time -= elapsed_time/1000;
#ifndef XBOARD
			printf("Computer time: %d\n",comp_time);
#endif
			MakeMove(pos,&move,move_num,turn);
			PlaceAttacker(pos,turn,attack[turn]);
			PlaceAttacker(pos,other(turn),attack[other(turn)]);
			/*move_hist[move_num-1].from = move.from;
			move_hist[move_num-1].to = move.to;
			move_hist[move_num-1].captured_piece = move.captured_piece;
			move_hist[move_num-1].type = move.type;
			move_hist[move_num-1].piece_index = move.piece_index;
			move_hist[move_num-1].prom_piece = move.prom_piece*/
			move_num++;

			turn = other(turn);

#ifdef XBOARD
			printf("move %c%d%c%d\n",'a'+rank(move.from)-1,file(move.from),'a'+rank(move.to)-1,file(move.to));
            //fflush(stdout);
#else
			printf("Move: %c%d-%c%d\n",'a'+rank(move.from)-1,file(move.from),'a'+rank(move.to)-1,file(move.to));
#endif
#ifndef XBOARD
			printf("Score: %d\n",score);
#endif
		}
	}

	return 0;
}

void SetTurn(t_Turn t)
{
	turn = t;
}

t_Board GetCurrentPos(void)
{
	t_Board p_pos;

	p_pos = pos;

	return p_pos;
}
t_Turn *GetTurnPtr(void)
{
	t_Turn *t;

	t = &turn;
	return t;
}
