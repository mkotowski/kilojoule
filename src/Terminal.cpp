#include <cstdio> // sscanf
#include <array>

#if defined(__linux__)
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include "Terminal.hpp"

Terminal::Terminal()
{
	if (tcgetattr(STDIN_FILENO, &initFlags) == -1) {
		throw("tcgetattr: Could not get the current terminal mode.");
	}
}

bool Terminal::isCookedModeRestoredProperly = true;

TerminalMode
Terminal::SetMode(TerminalMode newMode)
{
	std::atexit(Terminal::ForceCookedMode);

	if (currentMode == newMode) {
		return currentMode;
	}

	currentFlags = initFlags;

	switch (newMode) {
		case TerminalMode::Raw:
			// turn off:
			//   ECHO   -- printing keypresses
			//   ICANON -- cannonical mode
			//   ISIG   -- SIGINT (Ctrl-C) and SIGTSTP (Ctrl-Z)
			//   IXON   -- software flow control (Ctrl-S and Ctrl-Q)
			//   IEXTEN -- literal character sending (Ctrl-V)
			//   ICRNL  -- automatic \r (13) into \n (10) translation (fix for
			//   Ctrl-M) OPOST  -- output processing
			//
			// Miscellaneous flags, mostly legacy or turned off by default
			//   BRKINT -- a break condition will cause a SIGINT signal
			//   INPCK  -- parity checking
			//   ISTRIP -- causes the 8th bit of each input byte to be stripped
			//   CS8    -- a bit mask setting the character size (CS)
			currentFlags.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
			currentFlags.c_oflag &= ~(OPOST);
			currentFlags.c_cflag |= (CS8);
			currentFlags.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
			// set the minimum number of bytes of input needed before read() can
			// return setting to 0 means, that read() returns as soon as there is any
			// input
			currentFlags.c_cc[VMIN] = 0;
			// set the maximum amount of time to wait before read() return to 100 ms
			currentFlags.c_cc[VTIME] = 1;

			if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &currentFlags) == -1) {
				throw("tcsetattr: Could not set the raw terminal mode.");
			}
			isCookedModeRestoredProperly = false;
			break;
		case TerminalMode::Cbreak:
			// TODO
			if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &initFlags) == -1) {
				throw("tcsetattr: Could not set the cbreak terminal mode.");
			}
			isCookedModeRestoredProperly = false;
			break;
		case TerminalMode::Cooked:
			// Return to the original mode
			if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &initFlags) == -1) {
				throw("tcsetattr: Could not set the cooked terminal mode.");
			}
			isCookedModeRestoredProperly = true;
			break;
		default:
			throw("Unknown mode.");
			break;
	}

	currentMode = newMode;
	return currentMode;
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
	std::array<char, 32> buf;
	unsigned int         i = 0;

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

void
Terminal::ForceCookedMode()
{
	// In case of an improperly closed program force the pre-configured cooked
	// mode:
	if (!isCookedModeRestoredProperly) {
		// TODO: Compare the default values of the most popular emulators.

		TerminalFlags tmp;

		tmp.c_cc[0] = 3;
		tmp.c_cc[1] = 28;
		tmp.c_cc[2] = 127;
		tmp.c_cc[3] = 21;
		tmp.c_cc[4] = 4;
		tmp.c_cc[5] = 0;
		tmp.c_cc[6] = 1;
		tmp.c_cc[7] = 0;
		tmp.c_cc[8] = 17;
		tmp.c_cc[9] = 19;
		tmp.c_cc[10] = 26;
		tmp.c_cc[11] = 0;
		tmp.c_cc[12] = 18;
		tmp.c_cc[13] = 15;
		tmp.c_cc[14] = 23;
		tmp.c_cc[15] = 22;

		for (size_t i = 16; i < 32; i++) {
			tmp.c_cc[i] = 0;
		}

		tmp.c_iflag = 1280;
		tmp.c_oflag = 5;
		tmp.c_cflag = 191;
		tmp.c_lflag = 35387;
		tmp.c_ispeed = 15;
		tmp.c_ospeed = 15;
		tmp.c_line = 0;

		tcsetattr(STDIN_FILENO, TCSAFLUSH, &tmp);
	}
}
