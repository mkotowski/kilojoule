#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios orig_termios;

void die(const char *s)
{
	perror(s);
	exit(1);
}

void disableRawMode()
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
	{
		die("tcsetattr");
	}
}

void enableRawMode()
{
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
	{
		die("tcgetattr");
	}
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
	//
	// Miscellaneous flags, mostly legacy or turned off by default
	//   BRKINT -- a break condition will cause a SIGINT signal
	//   INPCK  -- parity checking
	//   ISTRIP -- causes the 8th bit of each input byte to be stripped
	//   CS8    -- a bit mask setting the character size (CS)
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	// set the minimum number of bytes of input needed before read() can return
	// setting to 0 means, that read() returns as soon as there is any input
	raw.c_cc[VMIN] = 0;
	// set the maximum amount of time to wait before read() return to 100 ms
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
	{
		die("tcsetattr");
	}
}

int main()
{
	enableRawMode();

	while (1)
	{
		char c = '\0';
		// in Cygwin, when read() times out it returns -1 with an errno
		// of EAGAIN, instead of just returning 0
		if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
		{
			die("read");
		}
		if (iscntrl(c))
		{
			printf("%d\r\n", c);
		}
		else
		{
			printf("%d ('%c')\r\n", c, c);
		}
		if (c == CTRL_KEY('q')) break;
	}
	return 0;
}
