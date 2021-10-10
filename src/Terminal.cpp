#include <array>
#include <cstdio> // sscanf

#if defined(__linux__) || defined(__ANDROID__)
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include "Terminal.hpp"

Terminal::Terminal()
{
#if defined(_WIN32)
#else
	if (tcgetattr(STDIN_FILENO, &initFlags) == -1) {
		throw("tcgetattr: Could not get the current terminal mode.");
	}
#endif
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

#if defined(_WIN32)
	HANDLE G_in_bufhdl;
	HANDLE G_out_bufhdl;
	DWORD  newOut;
#endif

	switch (newMode) {
		case TerminalMode::Raw:
#if defined(_WIN32)
			G_in_bufhdl = GetStdHandle(STD_INPUT_HANDLE);
			G_out_bufhdl = GetStdHandle(STD_OUTPUT_HANDLE);

			GetConsoleMode(G_in_bufhdl, &initFlags);
			currentFlags = initFlags;
			currentFlags &= ~ENABLE_ECHO_INPUT;
			currentFlags &= ~ENABLE_LINE_INPUT;
			currentFlags &= ~ENABLE_PROCESSED_INPUT;
			currentFlags |= ENABLE_VIRTUAL_TERMINAL_INPUT;
			// currentFlags &= ~ENABLE_INSERT_MODE;
			SetConsoleMode(G_in_bufhdl, currentFlags);

			GetConsoleMode(G_out_bufhdl, &newOut);
			newOut &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
			newOut |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			newOut |= DISABLE_NEWLINE_AUTO_RETURN;
			// newOut &= ~ENABLE_PROCESSED_OUTPUT;
			SetConsoleMode(G_out_bufhdl, newOut);
#else
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
#endif
			isCookedModeRestoredProperly = false;
			break;
		case TerminalMode::Cbreak:
#if defined(_WIN32)
#else
			// TODO
			if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &initFlags) == -1) {
				throw("tcsetattr: Could not set the cbreak terminal mode.");
			}
#endif
			isCookedModeRestoredProperly = false;
			break;
		case TerminalMode::Cooked:
			// Return to the original mode
#if defined(_WIN32)
#else
			if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &initFlags) == -1) {
				throw("tcsetattr: Could not set the cooked terminal mode.");
			}
#endif
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
#if defined(_WIN32)
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi) == 0) {
		if (write(STDOUT_FILENO,
		          escapeSequences::cursorMaxForwardAndDown,
		          sizeof(escapeSequences::cursorMaxForwardAndDown)) !=
		    sizeof(escapeSequences::cursorMaxForwardAndDown)) {
			return -1;
		}
		return GetCursorPosition();
	}
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO,
		          escapeSequences::cursorMaxForwardAndDown,
		          sizeof(escapeSequences::cursorMaxForwardAndDown)) !=
		    sizeof(escapeSequences::cursorMaxForwardAndDown)) {
			return -1;
		}
		return GetCursorPosition();
	}

	columns = ws.ws_col;
	rows = ws.ws_row;

#endif
	return 0;
}

int
Terminal::GetCursorPosition()
{
	std::string buf{};
	char        tmp{};

	if (write(STDOUT_FILENO,
	          escapeSequences::deviceStatusReport::cursorPosition,
	          sizeof(escapeSequences::deviceStatusReport::cursorPosition)) !=
	    sizeof(escapeSequences::deviceStatusReport::cursorPosition)) {
		return -1;
	}

	while (true) {
		if (read(STDIN_FILENO, &tmp, 1) != 1) {
			break;
		}

		buf.append(&tmp);

		if (buf.back() == 'R') {
			break;
		}
	}

	if (buf[0] != '\x1b' || buf[1] != '[') {
		return -1;
	}
	if (sscanf(&buf.c_str()[2], "%d;%d", &rows, &columns) != 2) {
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

#if defined(__linux__)
		TerminalFlags tmp;

		for (size_t i = 0; i < defaultTermios::c_ccSize; i++) {
			tmp.c_cc[i] = defaultTermios::c_cc.at(i);
		}

		tmp.c_iflag = defaultTermios::c_iflag;
		tmp.c_oflag = defaultTermios::c_oflag;
		tmp.c_cflag = defaultTermios::c_cflag;
		tmp.c_lflag = defaultTermios::c_lflag;
#if not defined(__ANDROID__)
		tmp.c_ispeed = defaultTermios::c_ispeed;
		tmp.c_ospeed = defaultTermios::c_ospeed;
#endif
		tmp.c_line = defaultTermios::c_line;

		tcsetattr(STDIN_FILENO, TCSAFLUSH, &tmp);
#endif
	}
}
