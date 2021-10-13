#include <cerrno>
#include <cstring>
#include <cstdlib> // free
#include <cstdarg> // va_start va_end

#include <fstream>
#include <string>

#if defined(__linux__)
#include <unistd.h>
#endif

#include "constants.hpp"
#include "Editor.hpp"
#include "Terminal.hpp"

#define CTRL_KEY(k) ((k)&0x1f)

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

const char* C_HL_extensions[] = { ".c", ".h", ".cpp", nullptr };
const char* C_HL_keywords[] = { "switch",    "if",       "while",   "for",
	                              "break",     "continue", "return",  "else",
	                              "struct",    "union",    "typedef", "static",
	                              "enum",      "class",    "case",    "int|",
	                              "long|",     "double|",  "float|",  "char|",
	                              "unsigned|", "signed|",  "void|",   nullptr };

// highlight database
struct editorSyntax HLDB[] = {
	{ "c",
	  C_HL_extensions,
	  C_HL_keywords,
	  "//",
	  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS },
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

int
is_separator(int c)
{
	return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

int
Editor::SyntaxToColor(int hl)
{
	switch (hl) {
		case HL_COMMENT:
			return 36;
		case HL_KEYWORD1:
			return 33;
		case HL_KEYWORD2:
			return 32;
		case HL_STRING:
			return 35;
		case HL_NUMBER:
			return 31;
		case HL_MATCH:
			return 34;
		default:
			return 37;
	}
}

void
Editor::SelectSyntaxHighlight()
{
	config.syntax = nullptr;

	if (config.filename.empty()) {
		return;
	}

	std::string::size_type filePos = config.filename.rfind('.');

	std::string extension{};
	std::string fileName{};

	if (filePos != std::string::npos) {
		extension = config.filename.substr(filePos, std::string::npos);
	} else {
		filePos = 0;
	}

	fileName = config.filename.substr(0, filePos + 1);

	for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
		struct editorSyntax* s = &HLDB[j];
		unsigned int         i = 0;
		while (s->filematch[i] != nullptr) {
			bool is_ext = (s->filematch[i][0] == '.');
			if ((is_ext && !extension.empty() &&
			     !(extension != std::string(s->filematch[i]))) ||
			    (!is_ext &&
			     config.filename.find(s->filematch[i]) != std::string::npos)) {
				config.syntax = s;

				for (int filerow = 0; filerow < config.numrows; filerow++) {
					UpdateSyntax(&config.row[filerow]);
				}

				return;
			}
			i++;
		}
	}
}

int
Editor::Init(std::shared_ptr<Terminal> term)
{
	terminal = term;

	config.cx = 0;
	config.cy = 0;
	config.rx = 0;
	config.rowoff = 0;
	config.coloff = 0;
	config.numrows = 0;
	config.row = {};
	config.dirty = 0;
	config.filename.clear();
	config.statusmsg = "";
	config.statusmsg_time = 0;
	config.syntax = nullptr;

	config.screenrows = terminal->GetRows();
	config.screencols = terminal->GetColumns();

	// Adjust for the status prompt
	config.screenrows -= 2;

	return 0;
}

void
Editor::UpdateSyntax(erow* row) const
{
	row->hl = (unsigned char*)realloc(row->hl, row->render.size());
	memset(row->hl, HL_NORMAL, row->render.size());

	if (config.syntax == nullptr) {
		return;
	}

	const char** keywords = config.syntax->keywords;

	const char* scs = config.syntax->singleline_comment_start;
	int         scs_len = scs != nullptr ? static_cast<int>(strlen(scs)) : 0;

	int prev_sep = 1;
	int in_string = 0;

	size_t i = 0;
	while (i < row->render.size()) {
		char          c = row->render[i];
		unsigned char prev_hl =
		  (i > 0) ? row->hl[i - 1] : static_cast<int>(HL_NORMAL);

		if (static_cast<bool>(scs_len) && !static_cast<bool>(in_string)) {
			if (!strncmp(&row->render[i], scs, scs_len)) {
				memset(&row->hl[i], HL_COMMENT, row->render.size() - i);
				break;
			}
		}

		if (static_cast<bool>(config.syntax->flags & HL_HIGHLIGHT_STRINGS)) {
			if (static_cast<bool>(in_string)) {
				row->hl[i] = HL_STRING;
				if (c == '\\' && i + 1 < row->render.size()) {
					row->hl[i + 1] = HL_STRING;
					i += 2;
					continue;
				}
				if (c == in_string) {
					in_string = 0;
				}
				i++;
				prev_sep = 1;
				continue;
			}

			if (c == '"' || c == '\'') {
				in_string = c;
				row->hl[i] = HL_STRING;
				i++;
				continue;
			}
		}

		if (config.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
			if ((static_cast<bool>(isdigit(c)) &&
			     (static_cast<bool>(prev_sep) || prev_hl == HL_NUMBER)) ||
			    (c == '.' && prev_hl == HL_NUMBER)) {
				row->hl[i] = HL_NUMBER;
				i++;
				prev_sep = 0;
				continue;
			}
		}

		if (static_cast<bool>(prev_sep)) {
			int j;
			for (j = 0; keywords[j]; j++) {
				int klen = static_cast<int>(strlen(keywords[j]));
				int kw2 = keywords[j][klen - 1] == '|';
				if (static_cast<bool>(kw2)) {
					klen--;
				}

				if (!static_cast<bool>(strncmp(&row->render[i], keywords[j], klen)) &&
				    static_cast<bool>(is_separator(row->render[i + klen]))) {
					memset(&row->hl[i],
					       static_cast<bool>(kw2) ? HL_KEYWORD2 : HL_KEYWORD1,
					       klen);
					i += klen;
					break;
				}
			}
			if (keywords[j] != nullptr) {
				prev_sep = 0;
				continue;
			}
		}

		prev_sep = is_separator(c);
		i++;
	}
}

void
Editor::DelRow(int at)
{
	if (at < 0 || at >= config.numrows)
		return;
	FreeRow(&config.row[at]);
	std::vector<erow>::iterator it = config.row.begin() + at;
	config.row.erase(it);
	config.numrows--;
	config.dirty++;
}

void
Editor::RowInsertChar(erow* row, int at, char c)
{
	if (at < 0 || at > static_cast<int>(row->chars.size())) {
		at = row->chars.size();
	}

	row->chars.insert(static_cast<size_t>(at), &c);

	UpdateRow(row);
	config.dirty++;
}

void
Editor::RowAppendString(erow* row, char* s, size_t len)
{
	row->chars.append(s, len);
	UpdateRow(row);
	config.dirty++;
}

void
Editor::RowDelChar(erow* row, int at)
{
	if (at < 0 || at >= static_cast<int>(row->chars.size())) {
		return;
	}
	row->chars.erase(at, 1);
	UpdateRow(row);
	config.dirty++;
}

void
Editor::InsertChar(int c)
{
	if (config.cy == config.numrows) {
		InsertRow(config.numrows, "\0");
	}
	RowInsertChar(&config.row[config.cy], config.cx, c);
	config.cx++;
}

void
Editor::InsertNewline()
{
	if (config.cx == 0) {
		InsertRow(config.cy, "");
	} else {
		erow* row = &config.row[config.cy];
		InsertRow(config.cy + 1, &row->chars[config.cx]);
		row = &config.row[config.cy];
		row->chars.erase(config.cx, std::string::npos);
		UpdateRow(row);
	}
	config.cy++;
	config.cx = 0;
}

void
Editor::DelChar()
{
	if (config.cy == config.numrows) {
		return;
	}
	if (config.cx == 0 && config.cy == 0) {
		return;
	}

	erow* row = &config.row[config.cy];
	if (config.cx > 0) {
		RowDelChar(row, config.cx - 1);
		config.cx--;
	} else {
		config.cx = config.row[config.cy - 1].chars.size();
		RowAppendString(&config.row[config.cy - 1],
		                const_cast<char*>(row->chars.c_str()),
		                row->chars.size());
		DelRow(config.cy);
		config.cy--;
	}
}

std::string
Editor::RowsToString() const
{
	std::string buf{};

	for (int j = 0; j < config.numrows; j++) {
		buf.append(config.row[j].chars);
		buf.append("\n");
	}

	return buf;
}

int
Editor::ReadKey()
{
	int  nread;
	char c;
	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		// in Cygwin, when read() times out it returns -1 with an errno
		// of EAGAIN, instead of just returning 0
		if (nread == -1 && errno != EAGAIN) {
			// die("read");
		}
	}

	if (c == '\x1b') {
		std::array<char, 3> seq;

		if (read(STDIN_FILENO, &seq[0], 1) != 1) {
			return '\x1b';
		}

		if (read(STDIN_FILENO, &seq[1], 1) != 1) {
			return '\x1b';
		}

		if (seq[0] == '[') {
			if (seq[1] >= '0' && seq[1] <= '9') {
				if (read(STDIN_FILENO, &seq[2], 1) != 1) {
					return '\x1b';
				}
				if (seq[2] == '~') {
					switch (seq[1]) {
						case '1':
							return HOME_KEY;
						case '3':
							return DEL_KEY;
						case '4':
							return END_KEY;
						case '5':
							return PAGE_UP;
						case '6':
							return PAGE_DOWN;
						case '7':
							return HOME_KEY;
						case '8':
							return END_KEY;
					}
				}
			} else {
				switch (seq[1]) {
					case 'A':
						return ARROW_UP;
					case 'B':
						return ARROW_DOWN;
					case 'C':
						return ARROW_RIGHT;
					case 'D':
						return ARROW_LEFT;
					case 'H':
						return HOME_KEY;
					case 'F':
						return END_KEY;
				}
			}
		} else if (seq[0] == 'O') {
			switch (seq[1]) {
				case 'H':
					return HOME_KEY;
				case 'F':
					return END_KEY;
			}
		}
		return '\x1b';
	}
	return c;
}

void
Editor::Open(const char* filename)
{
	config.filename = filename;

	SelectSyntaxHighlight();

	std::string   line{};
	std::ifstream file(filename);

	if (file.is_open()) {
		while (getline(file, line)) {
			InsertRow(config.numrows, line.c_str());
		}
		file.close();
	} else {
		// TODO
	}

	config.dirty = 0;
}

void
Editor::InsertRow(int at, const char* s)
{
	if (at < 0 || at > config.numrows) {
		return;
	}

	erow tmp{};
	tmp.hl = nullptr;
	tmp.chars = s;
	tmp.render = "";

	auto it = config.row.begin() + at;
	config.row.insert(it, tmp);

	UpdateRow(&config.row[at]);

	config.numrows++;
	config.dirty++;
}

void
Editor::UpdateRow(erow* row) const
{
	row->render = "";

	int idx = 0;
	for (int j = 0; j < static_cast<int>(row->chars.size()); j++) {
		if (row->chars[j] == '\t') {
			row->render += ' ';
			idx++;
			while (idx % kilojoule::defaults::tabStop != 0) {
				row->render += ' ';
				idx++;
			}
		} else {
			row->render += row->chars[j];
			idx++;
		}
	}

	UpdateSyntax(row);
}

void
Editor::MoveCursor(int key)
{
	erow* row = (config.cy >= config.numrows) ? nullptr : &config.row[config.cy];

	switch (key) {
		case ARROW_LEFT:
			if (config.cx != 0) {
				config.cx--;
			}
			// Move up when moving left at the start of a line
			else if (config.cy > 0) {
				config.cy--;
				config.cx = config.row[config.cy].chars.size();
			}
			break;
		case ARROW_RIGHT:
			// Limit scrolling to the right
			if (row != nullptr && config.cx < static_cast<int>(row->chars.size())) {
				config.cx++;
			}
			// Move down when moving right at the end of a line
			else if (row != nullptr &&
			         config.cx == static_cast<int>(row->chars.size())) {
				config.cy++;
				config.cx = 0;
			}
			break;
		case ARROW_UP:
			if (config.cy != 0) {
				config.cy--;
			}
			break;
		case ARROW_DOWN:
			if (config.cy != config.numrows - 1) {
				config.cy++;
			}
			break;
	}

	// Snap cursor to end of line
	row = (config.cy >= config.numrows) ? nullptr : &config.row[config.cy];
	int rowlen = row != nullptr ? row->chars.size() : 0;
	if (config.cx > rowlen) {
		config.cx = rowlen;
	}
}

void
Editor::ProcessKeypress()
{
	static int quit_times = kilojoule::defaults::quitTimes;

	int c = ReadKey();

	switch (c) {
		case CTRL_KEY('q'):
			if (static_cast<bool>(config.dirty) && quit_times > 0) {
				SetStatusMessage("WARNING!!! File has unsaved changes. "
				                 "Press Ctrl-Q %d more times to quit.",
				                 quit_times);
				quit_times--;
				return;
			}
			write(STDOUT_FILENO, escapeSequences::clearEntireScreen, 4);
			write(STDOUT_FILENO, escapeSequences::cursorRepositionLeftmostTop, 3);
			shouldClose = true;
			break;
		case '\r':
			InsertNewline();
			break;

		case CTRL_KEY('s'):
			Save();
			break;
		case HOME_KEY:
			config.cx = 0;
			break;
		case END_KEY:
			if (config.cy < config.numrows) {
				config.cx = config.row[config.cy].chars.size();
			}
			break;
		case CTRL_KEY('f'):
			Find();
			break;
		case BACKSPACE:
		case CTRL_KEY('h'):
		case DEL_KEY:
			if (c == DEL_KEY) {
				MoveCursor(ARROW_RIGHT);
			}
			DelChar();
			break;
		case PAGE_UP:
		case PAGE_DOWN: {
			if (c == PAGE_UP) {
				config.cy = config.rowoff;
			} else if (c == PAGE_DOWN) {
				config.cy = config.rowoff + config.screenrows - 1;
				if (config.cy > config.numrows) {
					config.cy = config.numrows;
				}
			}

			int times = config.screenrows;
			while (static_cast<bool>(times--)) {
				MoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
			}
		} break;
		case ARROW_UP:
		case ARROW_DOWN:
		case ARROW_LEFT:
		case ARROW_RIGHT:
			MoveCursor(c);
			break;
		case CTRL_KEY('l'):
		case '\x1b':
			break;
		default:
			InsertChar(c);
			break;
	}

	quit_times = kilojoule::defaults::quitTimes;
}

void
Editor::SetStatusMessage(const char* fmt, ...)
{
	char*   tmp = new char[config.screencols];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(tmp, config.screencols, fmt, ap);
	va_end(ap);
	config.statusmsg = tmp;
	config.statusmsg_time = time(nullptr);
	delete[] tmp;
}

void
Editor::DrawStatusBar(std::string& ab) const
{
	ab.append(escapeSequences::color::reverse);

	char status[80], rstatus[80];

	int len = snprintf(
	  status,
	  sizeof(status),
	  "%.20s - %d lines %s",
	  config.filename.c_str() != nullptr ? config.filename.c_str() : "[No Name]",
	  config.numrows,
	  static_cast<bool>(config.dirty) ? "(modified)" : "");

	int rlen =
	  snprintf(rstatus,
	           sizeof(rstatus),
	           "%s | %d/%d",
	           config.syntax != nullptr ? config.syntax->filetype : "no ft",
	           config.cy + 1,
	           config.numrows);

	if (len > config.screencols) {
		len = config.screencols;
	}
	ab.append(status, len);

	while (len < config.screencols) {
		if (config.screencols - len == rlen) {
			ab.append(rstatus, rlen);
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
	int msglen = config.statusmsg.size();
	if (msglen > config.screencols) {
		msglen = config.screencols;
	}
	if (static_cast<bool>(msglen) && time(nullptr) - config.statusmsg_time < 5) {
		ab.append(config.statusmsg);
	}
}

void
Editor::DrawRows(std::string& ab) const
{
	int y;
	for (y = 0; y < config.screenrows; y++) {
		int filerow = y + config.rowoff;
		if (filerow >= config.numrows) {
			if (config.numrows == 0 && y == config.screenrows / 3) {
				std::string welcome{ "KiloJoule editor -- version " };
				welcome += kilojoule::version;
				int welcomelen = welcome.size();

				if (welcomelen > config.screencols) {
					welcomelen = config.screencols;
				}
				int padding = (config.screencols - welcomelen) / 2;
				if (static_cast<bool>(padding)) {
					ab.append("~");
					padding--;
				}
				while (static_cast<bool>(padding--)) {
					ab.append(" ");
				}
				ab.append(welcome.c_str(), welcomelen);
			} else {
				ab.append("~");
			}
		} else {
			int len = config.row[filerow].render.size() - config.coloff;

			if (len < 0) {
				len = 0;
			}

			if (len > config.screencols) {
				len = config.screencols;
			}

			const char*    c = &config.row[filerow].render[config.coloff];
			unsigned char* hl = &config.row[filerow].hl[config.coloff];
			int            current_color = -1;

			for (int j = 0; j < len; j++) {
				if (hl[j] == HL_NORMAL) {
					if (current_color != -1) {
						ab.append(escapeSequences::color::defaultForeground);
						current_color = -1;
					}
					ab.append(&c[j], 1);
				} else {
					int color = SyntaxToColor(hl[j]);
					if (color != current_color) {
						current_color = color;
						std::string buf = escapeSequences::color::getEscapeSequence(color);
						ab.append(buf);
					}
					ab.append(&c[j], 1);
				}
			}
			ab.append(escapeSequences::color::defaultForeground);
		}

		ab.append(escapeSequences::eraseInLine);
		ab.append("\r\n");
	}
}

void
Editor::FreeRow(erow* row)
{
	free(row->hl);
}

int
Editor::RowCxToRx(erow* row, int cx)
{
	int rx = 0;

	for (int j = 0; j < cx; j++) {
		if (row->chars[j] == '\t') {
			rx += (kilojoule::defaults::tabStop - 1) -
			      (rx % kilojoule::defaults::tabStop);
		}
		rx++;
	}
	return rx;
}

int
Editor::RowRxToCx(erow* row, int rx)
{
	int    cur_rx = 0;
	size_t cx;
	for (cx = 0; cx < row->chars.size(); cx++) {
		if (row->chars[cx] == '\t') {
			cur_rx += (kilojoule::defaults::tabStop - 1) -
			          (cur_rx % kilojoule::defaults::tabStop);
		}
		cur_rx++;

		if (cur_rx > rx) {
			return cx;
		}
	}
	return static_cast<int>(cx);
}

void
Editor::Scroll()
{
	config.rx = 0;
	if (config.cy < config.numrows) {
		config.rx = RowCxToRx(&config.row[config.cy], config.cx);
	}

	if (config.cy < config.rowoff) {
		config.rowoff = config.cy;
	}
	if (config.cy >= config.rowoff + config.screenrows) {
		config.rowoff = config.cy - config.screenrows + 1;
	}
	if (config.rx < config.coloff) {
		config.coloff = config.rx;
	}
	if (config.rx >= config.coloff + config.screencols) {
		config.coloff = config.rx - config.screencols + 1;
	}
}

void
Editor::RefreshScreen()
{
	Scroll();

	std::string ab{};

	ab.append(escapeSequences::hideCursor);
	ab.append(escapeSequences::cursorRepositionLeftmostTop);

	DrawRows(ab);
	DrawStatusBar(ab);
	DrawMessageBar(ab);

	std::string buf("\x1b[" + std::to_string((config.cy - config.rowoff) + 1) +
	                ";" + std::to_string((config.rx - config.coloff) + 1) + "H");

	ab.append(buf);

	ab.append(escapeSequences::showCursor);

	write(STDOUT_FILENO, ab.c_str(), ab.length());
	ab.clear();
}

void
Editor::Save()
{
	if (config.filename.empty()) {
		config.filename = Prompt("Save as: %s (ESC to cancel)", nullptr);
		if (config.filename.empty()) {
			SetStatusMessage("Save aborted");
			return;
		}
		SelectSyntaxHighlight();
	}

	std::string buf = RowsToString();

	std::ofstream saveFile(config.filename, std::ios_base::out);

	if (saveFile.is_open()) {
		if (saveFile << buf) {
			saveFile.close();
			config.dirty = 0;
			SetStatusMessage("%d bytes written to disk", buf.size());
			return;
		}
		saveFile.close();
	}

	SetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

void
Editor::FindCallback(const char* query, int key)
{
	static int last_match = -1;
	static int direction = 1;

	static int   saved_hl_line;
	static char* saved_hl = nullptr;

	if (saved_hl != nullptr) {
		memcpy(config.row[saved_hl_line].hl,
		       saved_hl,
		       config.row[saved_hl_line].render.size());
		free(saved_hl);
		saved_hl = nullptr;
	}

	if (key == '\r' || key == '\x1b') {
		last_match = -1;
		direction = 1;
		return;
	}

	if (key == ARROW_RIGHT || key == ARROW_DOWN) {
		direction = 1;
	} else if (key == ARROW_LEFT || key == ARROW_UP) {
		direction = -1;
	} else {
		last_match = -1;
		direction = 1;
	}

	if (last_match == -1) {
		direction = 1;
	}
	int current = last_match;
	for (int i = 0; i < config.numrows; i++) {
		current += direction;
		if (current == -1) {
			current = config.numrows - 1;
		} else if (current == config.numrows) {
			current = 0;
		}

		erow*       row = &config.row[current];
		const char* match = strstr(row->render.c_str(), query);
		if (match != nullptr) {
			last_match = current;
			config.cy = current;
			config.cx = RowRxToCx(row, match - row->render.c_str());
			config.rowoff = config.numrows;

			saved_hl_line = current;
			saved_hl = (char*)malloc(row->render.size());
			memcpy(saved_hl, row->hl, row->render.size());
			memset(&row->hl[match - row->render.c_str()], HL_MATCH, strlen(query));
			break;
		}
	}
}

void
Editor::Find()
{
	int saved_cx = config.cx;
	int saved_cy = config.cy;
	int saved_coloff = config.coloff;
	int saved_rowoff = config.rowoff;

	std::string query_str = Prompt("Search: %s (Use Arrows/Enter; ESC to cancel)",
	                               std::bind(&Editor::FindCallback,
	                                         this,
	                                         std::placeholders::_1,
	                                         std::placeholders::_2));

	if (!query_str.empty()) {
		config.cx = saved_cx;
		config.cy = saved_cy;
		config.coloff = saved_coloff;
		config.rowoff = saved_rowoff;
	}
}

std::string
Editor::Prompt(const char*                           prompt,
               std::function<void(const char*, int)> callback)
{
	std::string buf{};

	while (true) {
		SetStatusMessage(prompt, buf.c_str());
		RefreshScreen();

		int c = ReadKey();
		if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
			if (!buf.empty()) {
				buf.pop_back();
			}
		} else if (c == '\x1b') {
			SetStatusMessage("");
			if (callback != nullptr) {
				callback(const_cast<char*>(buf.c_str()), c);
			}
			return {};
		} else if (c == '\r') {
			if (!buf.empty()) {
				SetStatusMessage("");
				if (callback != nullptr) {
					callback(const_cast<char*>(buf.c_str()), c);
				}
				return buf;
			}
		} else if (static_cast<bool>(isprint(c))) {
			buf += static_cast<char>(c);
		}
		if (callback != nullptr) {
			callback(const_cast<char*>(buf.c_str()), c);
		}
	}
}
