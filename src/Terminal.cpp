#include <cstdio> // sscanf

#include <unistd.h>
#include <sys/ioctl.h>

#include "Terminal.hpp"

// Terminal::Terminal(/* args */) {}

// Terminal::~Terminal() {}

int
Terminal::ToggleRawMode(bool state = true)
{
	if (state) {
		if (tcgetattr(STDIN_FILENO, &initTermios) == -1) {
			// die("tcgetattr");
		}
		// std::atexit(disableRawMode);

		currentTermios = initTermios;

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
		currentTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
		currentTermios.c_oflag &= ~(OPOST);
		currentTermios.c_cflag |= (CS8);
		currentTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
		// set the minimum number of bytes of input needed before read() can return
		// setting to 0 means, that read() returns as soon as there is any input
		currentTermios.c_cc[VMIN] = 0;
		// set the maximum amount of time to wait before read() return to 100 ms
		currentTermios.c_cc[VTIME] = 1;

		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &currentTermios) == -1) {
			// die("tcsetattr");
		}
	} else {
		if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &initTermios) == -1) {
			// die("tcsetattr");
		}
	}
	return 0;
}

int
Terminal::GetWindowSize()
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
			return -1;
		}
		return GetCursorPosition();
	} else {
		columns = ws.ws_col;
		rows = ws.ws_row;
		return 0;
	}

	return 0;
}

int
Terminal::GetCursorPosition()
{
	char         buf[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
		return -1;
	}

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) {
			break;
		}
		if (buf[i] == 'R') {
			break;
		}
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[') {
		return -1;
	}
	if (sscanf(&buf[2], "%d;%d", &rows, &columns) != 2) {
		return -1;
	}

	return 0;
}
