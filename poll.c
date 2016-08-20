#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ncurses.h>
//#include <windows.h>
//#include <conio.h>
//#include <io.h>

#include "engine.h"
#include "command.h"

extern t_Flag flag;

static char cmd_buffer[512];

#ifdef WINDOWS
static HANDLE inh;
#endif
static FILE *input_stream;

int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

int ReadInput(void) {
  char buffer[512], *end;
  long bytes;

  do
    bytes=read(fileno(stdin),buffer,512);
  while (bytes<0);
  if (bytes == 0) {
    if (input_stream != stdin) fclose(input_stream);
    input_stream=stdin;
    return(0);
  }
  end=cmd_buffer+strlen(cmd_buffer);
  memcpy(end,buffer,bytes);
  *(end+bytes)=0;

  return(1);
}

int CheckInput(void) {
	int i;
#ifdef XBOARD
	static int init = 0, pipe;
	DWORD dw;
#endif
	static int xboard=0;
#ifdef XBOARD
	//xboard = 1;
    xboard = 0;
#endif

	if (!xboard && !isatty(fileno(stdin))) return(0);
	if (strchr(cmd_buffer,'\n')) return(1);

	if (xboard) {
#ifdef WINDOWS
    	if (!init) {
			init = 1;
			inh = GetStdHandle(STD_INPUT_HANDLE);
			pipe = !GetConsoleMode(inh, &dw);
			if (!pipe) {
				SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
				FlushConsoleInputBuffer(inh);
			}
		}
		if (pipe) {
			if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) {
				return 1;
			}
			return dw;
    	}
		else {
			GetNumberOfConsoleInputEvents(inh, &dw);
			return dw <= 1 ? 0 : dw;
		}
#endif
	}
	else {
		i=kbhit();
	}

	return(i);
}

void ClearBuffer(void)
{
	cmd_buffer[0] = '\0';
}

int Poll(void)
{
	int num_bytes;
	char *p;

	if (CheckInput()) {
		while (!strchr(cmd_buffer,'\n')) ReadInput();
#ifdef WINDOWS
		FlushConsoleInputBuffer(inh);
#endif
		fflush(stdin);
		p = strchr(cmd_buffer,'\n');
		*p = '\0';
	}

	/*if (!strchr(cmd_buffer,'\n')) return 0;*/

	if (strcmp(cmd_buffer,"quit") == 0) {
		flag.timeout = true;
		flag.quit = true;
		ClearBuffer();
		return 1;
	}
	else if (strcmp(cmd_buffer,"?") == 0) {
		ClearBuffer();
		flag.timeout = true;
		return 1;
	}

	return 0;
}
