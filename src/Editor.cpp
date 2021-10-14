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

	Scroll();

	std::string textBuffer{};

	textBuffer.append(escapeSequences::hideCursor);
	textBuffer.append(escapeSequences::cursorRepositionLeftmostTop);

	DrawRows(textBuffer);
	DrawStatusBar(textBuffer);
	DrawMessageBar(textBuffer);

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
				SetStatusMessage("WARNING!!! File has unsaved changes. "
				                 "Press Ctrl-Q %d more times to quit.",
				                 quit_times);
				quit_times--;
				return;
			}

			terminal->Write(escapeSequences::clearEntireScreen, 4);
			terminal->Write(escapeSequences::cursorRepositionLeftmostTop, 3);

			shouldClose = true;
			break;
		case '\r':
			InsertNewline();
			break;
		case Key::Home:
			cursorColumn = 0;
			break;
		case Key::End:
			if (cursorRow < rows.size()) {
				cursorColumn = rows[cursorRow].chars.size();
			}
			break;
		case Key::ArrowUp:
		case Key::ArrowDown:
		case Key::ArrowLeft:
		case Key::ArrowRight:
			MoveCursor(c);
			break;
		case CTRL_KEY('l'):
		case '\x1b':
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
		} else {
			size_t len = rows[filerow].render.size() - columnOffset;

			// if (len < 0) {
			// 	len = 0;
			// }

			if (len > screenCols) {
				len = screenCols;
			}

			const std::string& c = rows[filerow].render; //[columnOffset];
			// const char* hl = &rows[filerow].hl[columnOffset];
			// int         current_color = -1;
			for (size_t j = 0; j < len; j++) {
				// if (hl[j] == HL_NORMAL) {
				// if (current_color != -1) {
				// ab.append(escapeSequences::color::defaultForeground);
				// 	current_color = -1;
				// }
				if (j + columnOffset < rows[filerow].render.size()) {
					ab.append(&c[j + columnOffset], 1);
				}
				// } else {
				// 	int color = SyntaxToColor(hl[j]);
				// 	if (color != current_color) {
				// 		current_color = color;
				// std::string buf = escapeSequences::color::getEscapeSequence(color);
				// ab.append(buf);
				// 	}
				// 	ab.append(&c[j], 1);
				// }
			}
			ab.append(escapeSequences::color::defaultForeground);
		}

		ab.append(escapeSequences::eraseInLine);
		ab.append("\r\n");
	}
}

void
Editor::DrawStatusBar(std::string& ab) const
{
	ab.append(escapeSequences::color::reverse);

	std::string status{};
	std::string statusRight{};

	if (!filename.empty()) {
		status.append(filename);
	} else {
		status.append("[No Name]");
	}

	status.append(" - ");
	status.append(std::to_string(rows.size()));

	if (dirtyFlag) {
		status.append(" (modified)");
	}

	size_t len = status.size();

	if (len > screenCols) {
		len = screenCols;
	}
	ab.append(status.c_str(), len);

	if (syntax != nullptr) {
		statusRight.append(syntax->filetype);
	} else {
		statusRight.append("no ft");
	}

	statusRight.append(" | ");
	statusRight.append(std::to_string(cursorRow + 1));
	statusRight.append("/");
	statusRight.append(std::to_string(rows.size()));

	size_t rlen = statusRight.size();

	while (len < screenCols) {
		if (screenCols - len == rlen) {
			ab.append(statusRight.c_str(), rlen);
			break;
		}
		ab.append(" ");
		len++;
	}

	ab.append(escapeSequences::color::reset);
	ab.append("\r\n");
}

void
Editor::DrawMessageBar(std::string& ab)
{
	ab.append(escapeSequences::eraseInLine);
	size_t msglen = statusmsg.size();
	if (msglen > screenCols) {
		msglen = screenCols;
	}
	if (msglen > 0 && time(nullptr) - statusmsg_time <
	                    kilojoule::defaults::messageWaitDuration) {
		ab.append(statusmsg);
	}
}

void
Editor::SetStatusMessage(const char* fmt, ...)
{
	char*   tmp = new char[screenCols];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(tmp, screenCols, fmt, ap);
	va_end(ap);

	statusmsg = tmp;
	statusmsg_time = time(nullptr);

	delete[] tmp;
}

void
Editor::Scroll()
{
	cursorRenderColumn = 0;

	if (cursorRow < rows.size()) {
		cursorRenderColumn = cursorColumn;
		// RowCxToRx(&rows[cursorRow], cursorColumn);
	}

	if (cursorRow < rowOffset) {
		rowOffset = cursorRow;
	}
	if (cursorRow >= rowOffset + screenRows) {
		rowOffset = cursorRow - screenRows + 1;
	}
	if (cursorRenderColumn < columnOffset) {
		columnOffset = cursorRenderColumn;
	}
	if (cursorRenderColumn >= columnOffset + screenCols) {
		columnOffset = cursorRenderColumn - screenCols + 1;
	}
}

void
Editor::MoveCursor(int key)
{
	erow* row = (cursorRow >= rows.size()) ? nullptr : &rows[cursorRow];

	switch (key) {
		case Key::ArrowLeft:
			if (cursorColumn != 0) {
				cursorColumn--;
			}
			// Move up when moving left at the start of a line
			else if (cursorRow > 0) {
				cursorRow--;
				if (!rows.empty()) {
					cursorColumn = rows[cursorRow].chars.size();
				} else {
					cursorColumn = 0;
				}
			}
			break;
		case Key::ArrowRight:
			// Limit scrolling to the right
			if (row != nullptr && cursorColumn < row->chars.size()) {
				cursorColumn++;
			}
			// Move down when moving right at the end of a line
			else {
				cursorRow++;
				cursorColumn = 0;
			}
			break;
		case Key::ArrowUp:
			if (cursorRow != 0) {
				cursorRow--;
			}
			break;
		case Key::ArrowDown:
			if (cursorRow != rows.size() - 1) {
				cursorRow++;
			}
			break;
	}

	// Snap cursor to end of line
	row = (cursorRow >= rows.size()) ? nullptr : &rows[cursorRow];
	size_t rowlen = row != nullptr ? row->chars.size() : 0;
	if (cursorColumn > rowlen) {
		cursorColumn = rowlen;
	}
}

void
Editor::UpdateRow(size_t at)
{
	rows.at(at).render.clear();

	int idx = 0;
	for (size_t j = 0; j < rows.at(at).chars.size(); j++) {
		if (rows.at(at).chars[j] == '\t') {
			rows.at(at).render += ' ';
			idx++;
			while (idx % kilojoule::defaults::tabStop != 0) {
				rows.at(at).render += ' ';
				idx++;
			}
		} else {
			rows.at(at).render += rows.at(at).chars[j];
			idx++;
		}
	}

	// UpdateSyntax(row);
}

void
Editor::InsertRow(size_t at, const char* s)
{
	if (at > rows.size()) {
		return;
	}

	erow tmp{};
	tmp.chars = s;

	auto it = rows.begin() + at;
	rows.insert(it, tmp);

	UpdateRow(at);
}

void
Editor::InsertNewline()
{
	if (cursorColumn == 0) {
		InsertRow(cursorRow, "");
	} else {
		erow* row = &rows[cursorRow];
		InsertRow(cursorRow + 1, &row->chars[cursorColumn]);
		row = &rows[cursorRow];
		row->chars.erase(cursorColumn, std::string::npos);
		UpdateRow(cursorRow);
	}
	cursorRow++;
	cursorColumn = 0;

	dirtyFlag = true;
	dirtyLevel++;
}

void
Editor::Open(const char* filename)
{
	rows.clear();

	this->filename = filename;

	// SelectSyntaxHighlight();

	std::string   line{};
	std::ifstream file(filename);

	if (file.is_open()) {
		while (getline(file, line)) {
			InsertRow(rows.size(), line.c_str());
		}
		file.close();
	} else {
		std::string errorMessage{};
		errorMessage.append(escapeSequences::color::getEscapeSequence(31));
		errorMessage.append("Could not access the selected file.");
		errorMessage.append(escapeSequences::color::reset);
		SetStatusMessage(errorMessage.c_str());
	}
}
