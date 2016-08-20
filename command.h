#ifndef _COMMAND_H
#define _COMMAND_H

#define GAMES_FILE_NAME "games"

typedef struct {
	int opponent;
	int mate;
	int draw;
	int quit;
	int timeout;
	int computer_plays;
	int autosave;
} t_Flag;

int CheckCommand(char *);

#endif
