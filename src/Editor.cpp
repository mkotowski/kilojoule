#include <cerrno>
#include <cstring>
#include <cstdlib> // free
#include <cstdarg> // va_start va_end
// uncomment to disable assert()
#define NDEBUG
#include <cassert>
#include <fstream>
#include <string>

#if defined(__linux__)
#include <unistd.h>
#endif

#include "constants.hpp"
#include "Editor.hpp"
#include "Terminal.hpp"

#define CTRL_KEY(k) ((k)&0x1f)

int
Editor::Init(std::shared_ptr<Terminal> term)
{
	terminal = term;

	if (terminal == nullptr) {
		screenRows = 0;
		screenCols = 0;
	} else {
		screenRows = terminal->GetRows();
		screenCols = terminal->GetColumns();
	}

	// Adjust for the status prompt
	screenRows -= 2;

	return 0;
}

void
Editor::RefreshScreen()
{
	if (terminal == nullptr) {
		return;
	}

	// Scroll();

	std::string textBuffer{};

	textBuffer.append(escapeSequences::hideCursor);
	textBuffer.append(escapeSequences::cursorRepositionLeftmostTop);

	DrawRows(textBuffer);
	// DrawStatusBar(ab);
	// DrawMessageBar(ab);

	textBuffer.append(Terminal::SetCursorPositionEscapeSequence(
	  (cursorRow - rowOffset) + 1, (cursorRenderColumn - columnOffset) + 1));
	textBuffer.append(escapeSequences::showCursor);

	terminal->Write(textBuffer);
}

void
Editor::ProcessKeypress()
{
	if (terminal == nullptr) {
		return;
	}

	static int quit_times = kilojoule::defaults::quitTimes;

	int c = Terminal::Read();

	switch (c) {
		case CTRL_KEY('q'):
			if (dirtyFlag && quit_times > 0) {
				// SetStatusMessage("WARNING!!! File has unsaved changes. "
				//                  "Press Ctrl-Q %d more times to quit.",
				//                  quit_times);
				quit_times--;
				return;
			}

			terminal->Write(escapeSequences::clearEntireScreen, 4);
			terminal->Write(escapeSequences::cursorRepositionLeftmostTop, 3);

			shouldClose = true;
			break;

		default:

			break;
	}

	quit_times = kilojoule::defaults::quitTimes;
}

std::vector<std::string> logo = { " _    _ ", "| |  (_)", "| | ___ ",
	                                "| |/ / |", "|   <| |", "|_|\\_\\ |",
	                                "    _/ |", "   |__/ " };

void
Editor::DrawRows(std::string& ab) const
{
	int logoPadding = (screenRows / 2) - logo.size() - 2;

	for (size_t y = 0; y < screenRows; y++) {
		size_t filerow = y + rowOffset;

		if (filerow >= rows.size()) {
			if (rows.empty() && y == screenRows / 2) {
				// Version info
				std::string welcome{ "KiloJoule editor -- version " };
				welcome.append(kilojoule::version);

				size_t welcomelen = welcome.size();
				if (welcomelen > screenCols) {
					welcomelen = screenCols;
				}

				int padding = (screenCols - welcomelen) / 2;
				if (padding > 0) {
					ab.append("~");
					padding--;
				}
				while (padding-- > 0) {
					ab.append(" ");
				}

				// Clip the string if necessary
				ab.append(welcome.c_str(), welcomelen);
			} else if (rows.empty() && y - logoPadding - 1 < logo.size() &&
			           y - logoPadding > 0) {
				// Logo
				ab.append("~");
				int padding = (screenCols - logo[y - logoPadding - 1].size()) / 2;
				while (padding-- > 0) {
					ab.append(" ");
				}
				ab.append(logo[y - logoPadding - 1]);
			} else {
				// Vim style: lines not belonging to the file == tilde
				ab.append("~");
			}
		}
		// else {
		// 	int len = config.row[filerow].render.size() - config.coloff;

		// 	if (len < 0) {
		// 		len = 0;
		// 	}

		// 	if (len > config.screencols) {
		// 		len = config.screencols;
		// 	}

		// 	const char* c = &config.row[filerow].render[config.coloff];
		// 	const char* hl = &config.row[filerow].hl[config.coloff];
		// 	int         current_color = -1;

		// 	for (int j = 0; j < len; j++) {
		// 		if (hl[j] == HL_NORMAL) {
		// 			if (current_color != -1) {
		// 				ab.append(escapeSequences::color::defaultForeground);
		// 				current_color = -1;
		// 			}
		// 			ab.append(&c[j], 1);
		// 		} else {
		// 			int color = SyntaxToColor(hl[j]);
		// 			if (color != current_color) {
		// 				current_color = color;
		// 				std::string buf =
		// escapeSequences::color::getEscapeSequence(color); ab.append(buf);
		// 			}
		// 			ab.append(&c[j], 1);
		// 		}
		// 	}
		// 	ab.append(escapeSequences::color::defaultForeground);
		// }

		ab.append(escapeSequences::eraseInLine);
		ab.append("\r\n");
	}
}
