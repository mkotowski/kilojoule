#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disableRawMode()
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode()
{
	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(disableRawMode);

	struct termios raw = orig_termios;
	
	// turn off:
	//   ECHO   -- printing keypresses
	//   ICANON -- cannonical mode
	//   ISIG   -- SIGINT (Ctrl-C) and SIGTSTP (Ctrl-Z)
	//   IXON   -- software flow control (Ctrl-S and Ctrl-Q)
	//   IEXTEN -- literal character sending (Ctrl-V)
	//   ICRNL  -- automatic \r (13) into \n (10) translation (fix for Ctrl-M)
	//   OPOST  -- output processing
	raw.c_iflag &= ~(ICRNL | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
	enableRawMode();

	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
	{
		if (iscntrl(c))
		{
			printf("%d\r\n", c);
		}
		else
		{
			printf("%d ('%c')\r\n", c, c);
		}
	}
	return 0;
}
