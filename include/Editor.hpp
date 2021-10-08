#pragma once

#include <ctime> // time_t
#include <memory>

class Terminal;

typedef struct erow
{
	int            size;
	int            rsize;
	char*          chars;
	char*          render;
	unsigned char* hl;
} erow;

struct editorSyntax
{
	const char*  filetype;
	const char** filematch;
	const char** keywords;
	const char*  singleline_comment_start;
	int          flags;
};

struct editorConfig
{
	int                  cx, cy;
	int                  rx;
	int                  rowoff;
	int                  coloff;
	int                  screenrows;
	int                  screencols;
	int                  numrows;
	erow*                row;
	int                  dirty;
	std::string          filename;
	char                 statusmsg[80];
	time_t               statusmsg_time;
	struct editorSyntax* syntax;
};

// append buffer

class Editor
{
private:
	editorConfig config;

public:
	Editor(/* args */) = default;
	~Editor() = default;

	std::shared_ptr<Terminal> terminal = nullptr;

	bool shouldClose = false;

	int Init(std::shared_ptr<Terminal> term);

	void        SetStatusMessage(const char* fmt, ...);
	void        RefreshScreen();
	const char* Prompt(const char* prompt, void (*callback)(char*, int));
	int         ReadKey();
	void        Open(const char* filename);
	void        InsertRow(int at, const char* s, size_t len);
	void        UpdateRow(erow* row);
	void        ProcessKeypress();
	void        DrawMessageBar(std::string& ab);
	void        DrawStatusBar(std::string& ab);
	void        DrawRows(std::string& ab);
	void        FreeRow(erow* row);
	void        Scroll();
	int         RowCxToRx(erow* row, int cx);
	int         RowRxToCx(erow* row, int rx);
	void        MoveCursor(int key);
	void        DelRow(int at);
	void        RowInsertChar(erow* row, int at, int c);
	void        RowAppendString(erow* row, char* s, size_t len);
	void        RowDelChar(erow* row, int at);
	void        InsertChar(int c);
	void        InsertNewline();
	void        DelChar();
	char*       RowsToString(int* buflen);
	void        Save();
	void        Find();
	void        SelectSyntaxHighlight();
	static int  SyntaxToColor(int hl);
	static void FindCallback(char* query, int key);

	void UpdateSyntax(erow* row);

	enum editorHighlight
	{
		HL_NORMAL = 0,
		HL_COMMENT,
		HL_KEYWORD1,
		HL_KEYWORD2,
		HL_STRING,
		HL_NUMBER,
		HL_MATCH
	};

	enum editorKey
	{
		BACKSPACE = 127,
		ARROW_LEFT = 1000,
		ARROW_RIGHT,
		ARROW_UP,
		ARROW_DOWN,
		DEL_KEY,
		HOME_KEY,
		END_KEY,
		PAGE_UP,
		PAGE_DOWN
	};
};
