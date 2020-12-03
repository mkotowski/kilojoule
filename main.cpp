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

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main()
{
	enableRawMode();

	while (1)
	{
		char c = '\0';
		read(STDIN_FILENO, &c, 1);
		if (iscntrl(c))
		{
			printf("%d\r\n", c);
		}
		else
		{
			printf("%d ('%c')\r\n", c, c);
		}
		if (c == 'q') break;
	}
	return 0;
}
