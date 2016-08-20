#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "command.h"
#include "mychess.h"
#include "engine.h"

extern t_Flag flag;
extern t_Square pos[144];
extern int material_score, material[2];
extern int Game50;
int RepetitionCheck(int);
int uci = false;



void WriteGame(FILE *fp, t_Move *game, int moves)
{
	int i;
	char buff[20000], *s, *s0;

	s0 = s = buff;
	for (i=0;i<moves;i++) {
		if (!(i%2)) {
			s+=sprintf(s,"%d. ",i/2+1);
		}
		s+=sprintf(s,"%c%d-%c%d ",'a'+rank(game[i].from)-1,
				file(game[i].from),'a'+rank(game[i].to)-1,
				file(game[i].to));

	}
	s+=sprintf(s,"\n\n");
	printf("%s",buff);
	fwrite(buff,s-s0,sizeof(char),fp);
}

int GetMove(FILE *fp,char *str)
{
	char c;

	while ((c = getc(fp)) != ' ' && c != EOF && c != '\n') {
		*str = c;
		str++;
	}
	*str = '\0';
	printf("%s",str);
	if (c == ' ') return 1;
	else return -1;
}

int ReadGame(t_Board pos,FILE *fp, t_Move *move_hist)
{
	t_Move move;
	char move_str[10];
	t_Turn turn;
	int move_num;
	Finish();
	Initialize(pos,&turn);
	move_num = 1;
	while (GetMove(stdin,move_str) > 0) {

		if (ParseMove(move_str,&move,pos,turn) && IsLegalMove(pos,move,turn)) {
			MakeMove(pos,&move,move_num,turn);
			move_hist[move_num-1].from = move.from;
			move_hist[move_num-1].to = move.to;
			move_hist[move_num-1].captured_piece = move.captured_piece;
			move_hist[move_num-1].type = move.type;
			move_hist[move_num-1].piece_index = move.piece_index;
			move_num++;
			turn = other(turn);
		}
		else {
			printf("Invalid move\n");
			return -1;
		}
	}
	return move_num;
}

void ParseFEN(char *fen)
{
	char *p, *t, str[10];
	t_SquareIndex i, sq, x, x0, y, inc, ep_rank = 0, ep_file = 0;
    char piece_str[12] = "PNBRQKpnbrqk";
    char color_str[2] = "wb";
	t_Piece piece;
	t_Color color;
	t_Turn turn;
	t_Square pos[144];
	int m_num, rule_50_move = 0;
	int kcastle[2] = {0,0};
	int qcastle[2] = {0,0};

	for (i=0;i<144;i++) {
		pos[i].piece = null;
		pos[i].color = null;
	}

	p = piece_str;

	x = 0; y = 7;

	while (*fen != '\0' && *fen != ' ' && y >= 0) {
		x0 = x;
		if ((t = strchr(piece_str,*fen)) != NULL) {
			piece = (int)(t-p);
			color = piece/6;
			piece %= 6;
			piece++;
			sq = 26+12*y+x;
			pos[sq].piece = piece;
			pos[sq].color = color;
			x++;
			/*if (x >= 8) {
				x = 0;
				y--;
			}*/
		}
		else if (*fen >= '1' && *fen <= '8') {
			inc = *fen - '0';
			for (;x-x0<inc;x++) {
				sq = 26+12*y+x;
				pos[sq].piece = empty;
				pos[sq].color = empty;
			}
		}
		else if (*fen == '/') {
			x = 0;
			y--;
		}

		fen++;

	}
	while (*fen == ' ') fen++;

	p = color_str;
	if ((t = strchr(color_str,*fen)) != NULL) {
		turn = t-p;
	}
	else turn = white;
	fen++;
	while (*fen == ' ') fen++;
	printf("starting castling rights\n");
	while (*fen != ' ' && *fen != '\0') {
		switch (*fen) {
			case 'K':
				kcastle[white] = true;
				break;
			case 'Q':
				qcastle[white] = true;
				break;
			case 'k':
				kcastle[black] = true;
				break;
			case 'q':
				qcastle[black] = true;
				break;
			default:
				break;
		}
		fen++;
	}
	printf("done castling rights\n");
	while (*fen == ' ') fen++;
	printf("starting ep\n");
	if (*fen >= 'a' && *fen <= 'h') {
		ep_rank = *fen - 'a';
		fen++;
		ep_file = *fen - '1';
		fen++;
	}
	else if (*fen == '-') {
		ep_rank = -1;
		ep_file = -1;
		fen++;
	}

	while (*fen == ' ') fen++;
	printf("starting rule 50 move\n");
	if (isdigit(*fen)) {
		p = fen;
		while (*p != ' ' && *p != '\0') p++;
		strncpy(str,fen,(int)(p-fen)+1);
		rule_50_move = atoi(str);
		fen += (int)(p-fen)+1;
	}

	while (*fen == ' ') fen++;
	printf("starting move number\n");
	if (isdigit(*fen)) {
		p = fen;
		while (*p != ' ' && *p != '\0') p++;
		strncpy(str,fen,(p-fen)+1);
		m_num = atoi(str);
		fen += (int)(p-fen)+1;
	}

	/*PrintBoard(pos,white);
	printf("Turn: %s\n",turn == black?"black":"white");
	printf("Castling rights for white:\n");
	if (kcastle[white]) printf("king side ok\n");
	if (qcastle[white]) printf("queen side ok\n");
	printf("Castling rights for black:\n");
	if (kcastle[black]) printf("king side ok\n");
	if (qcastle[black]) printf("queen side ok\n");
	if (ep_file >= 0) {
		printf("En passant square: %c%d\n",'a'+ep_rank,1+ep_file);
	}
	else printf("No en passant square\n");
	printf("rule 50 move: %d\n",rule_50_move);
	printf("move number: %d\n",m_num);*/

	SetPosition(pos,turn,kcastle,qcastle,ep_file >= 0 ? (26+ep_file*12+ep_rank) : null,rule_50_move,m_num);
}

int CheckCommand(char *str)
{
	int m, n, num_moves, at, temp;
	char *p;
	t_Move move_list[100];
	//t_Board *position;
	FILE *fp;

	if (strcmp(str,"black") == 0) {
			flag.opponent = white;
			return 1;
		}
		else if (strcmp(str,"white") == 0) {
			flag.opponent = black;
			return 1;
		}
        else if (strcmp(str,"uci") == 0) {
            uci = true;
            printf("id MyChess\n");
            printf("options \n");
            printf("uciok\n");
            return 1;
        }
		else if (strcmp(str,"set") == 0) {
			gets(str);
			ParseFEN(str);
			printf("done parse\n");
			return 1;
		}
		else if (strcmp(str,"undo") == 0) {
			if (move_num > 1) {
				flag.mate = false;
				flag.draw = false;
				move_num--;
				turn = other(turn);
				flag.opponent = other(flag.opponent);
				UnMakeMove(pos,&move_hist[move_num-1],move_num,turn);
			}
			return 1;
		}
		else if (strcmp(str,"comp") == 0) {
			flag.computer_plays ^= 1;
			return 1;
		}
        else if (strncmp(str,"perft",5) == 0) {
            if (sscanf(str,"perft(%d)",&temp)) {
                printf("Number of nodes for perft(%d): %ld\n",temp,Perft(pos, turn, temp));
                return 1;
            }
            return 0;
            
        }
		else if (strncmp(str,"attacks",7) == 0) {
			if (strcmp(str,"attacks white") == 0)
				at = white;
			else
				at = black;

			for (m=7;m>=0;m--) {
				for (n=0;n<8;n++) {
					printf("%d ",attack[at][26+12*m+n]);
				}
				printf("\n");
			}
			printf("Num attackers:\n");
			for (m=7;m>=0;m--) {
				for (n=0;n<8;n++) {
					printf("%d ",num_attackers[at][26+12*m+n]);
				}
				printf("\n");
			}
			printf("Press return to continue...\n");
			getchar();
			return 1;
		}
		else if (strcmp(str,"board") == 0) {
			PrintBoard(pos,white);
			return 1;
		}
		else if (strcmp(str,"moves") == 0) {
			num_moves = GenerateMoves(pos,turn,move_list,move_num,IsInCheck(pos,turn));
			for (m=0;m<num_moves;m++) {
				printf("%c%d-%c%d:%d ",'a'+rank(move_list[m].from)-1,file(move_list[m].from),
				'a'+rank(move_list[m].to)-1,file(move_list[m].to),move_list[m].score);
			}
			printf("\n");
			return 1;
		}
		else if (strcmp(str,"force2") == 0) {
			int legal_move;
			char s[100];
			t_Move move;
			flag.draw = false;
			flag.mate = false;
			flag.opponent = white;
			flag.timeout = false;
			Finish();
			Initialize(pos,&turn);
			while (1) {
				printf("Enter move or . to quit: ");
				gets(s);
				printf("telluser str: %s\n",str);
				if (strcmp(s,".") == 0) break;
				if (!ParseMove(s,&move,pos,turn)) {
					printf("Syntax error\n\n");
					continue;
				}
				else if ((legal_move = IsLegalMove(pos,move,turn)) == 0) {
					printf("Illegal move: %c%d-%c%d\n",'a'+rank(move.from)-1,file(move.from),
					'a'+rank(move.to)-1,file(move.to));

					continue;
				}
				if (legal_move) {
					MakeMove(pos,&move,move_num,turn);
					PlaceAttacker(pos,turn,attack[turn]);
					PlaceAttacker(pos,other(turn),attack[other(turn)]);
					move_num++;

					turn = other(turn);
					flag.opponent = turn;
					legal_move = false;
				}
				continue;
			}
		}
		else if (strncmp(str,"pawn structure",strlen(str)) == 0 && strlen(str) > 0) {
			printf("White pawn structure\n");
			for (m=0;m<8;m++) {
				printf("%d ",pawn_struct[white][m]);
			}
			printf("\n");
			printf("Black pawn structure\n");
			for (m=0;m<8;m++) {
				printf("%d ",pawn_struct[black][m]);
			}
			printf("\n");
			return 1;
		}
		else if (strncmp(str,"go",2) == 0) {
			if (!flag.mate) {
				flag.opponent = other(flag.opponent);
                printf("opponent: %d\n",flag.opponent);
			}
			return 1;
		}
		else if (strcmp(str,"remove") == 0) {
			flag.mate = false;
			flag.draw = false;
			move_num--;
			turn = other(turn);
			UnMakeMove(pos,&move_hist[move_num-1],move_num,turn);
			move_num--;
			turn = other(turn);
			UnMakeMove(pos,&move_hist[move_num-1],move_num,turn);

			return 1;
		}
		else if (strncmp(str,"depth",5) == 0) {
			while (1) {
				printf("Enter search depth: ");
				gets(str);
				search_depth = atoi(str);
				break;
			}
			return 1;

		}
		else if (strcmp(str,"repetition") == 0) {
			printf("Rule 50 move: %d\n",Game50);
			printf("Repetitions: %d\n",RepetitionCheck(0));
			return 1;

		}
		else if (strncmp(str,"time",4) == 0) {
			p = str;
			p+=5;
			if ((temp = atoi(p)) > 0) {
				comp_time = temp;
#ifdef XBOARD
				comp_time /= 100;
#endif
			}
			else printf("Invalid time\n");
			return 1;
		}
        else if (strncmp(str,"wtime",5) == 0) {
            p = str;
            p+=6;
            if ((temp = atoi(p)) > 0) {
                comp_time = temp;
            }
        }
		else if (strcmp(str,"quit") == 0) {
			Finish();
			flag.quit = 1;
			return 1;
		}
		else if (strcmp(str,"new") == 0 || strcmp(str,"ucinewgame") == 0) {
			if (0*flag.autosave) {
				fp = fopen(GAMES_FILE_NAME,"a");
				if (!fp) {
					printf("Impossible to open file %s for writing\n",GAMES_FILE_NAME);
					close(fp);
                    return 1;
				}
				
                WriteGame(fp,move_hist,move_num-1);
                fclose(fp);
            }
			
			flag.draw = false;
			flag.mate = false;
			flag.opponent = white;
			flag.timeout = false;
			Finish();
			Initialize(pos,&turn);
			return 1;
		}
		else if (strcmp(str,"material") == 0) {
			printf("Material score: %d\n",material_score);
			printf("White material: %d\n",material[white]);
			printf("Black material: %d\n",material[black]);
			return 1;
		}
		else if (strcmp(str,"save") == 0) {
			fp = fopen(GAMES_FILE_NAME,"a");
			if (!fp) {
				printf("Impossible to open file %s for writing\n",GAMES_FILE_NAME);
				return 1;
			}
			WriteGame(fp,move_hist,move_num-1);
			fclose(fp);
			return 1;
		}
		else if (strcmp(str,"open") == 0) {
			temp = ReadGame(pos,stdin,move_hist);
			if (temp > 0) move_num = temp;
			return 1;
		}
		else if (strncmp(str,"delete saved games\n",strlen(str)) == 0 && strlen(str) > 2) {
			fp = fopen(GAMES_FILE_NAME,"w");
			if (!fp) {
				printf("Impossible to open file %s for writing\n",GAMES_FILE_NAME);
				return 1;
			}
			fclose(fp);
			return 1;
		}
        else if (strncmp(str,"protover",8) == 0) {
            printf("feature sigint=0 sigterm=0 reuse=0 done=1\n");
            return 1;
        }
	return 0;
}
