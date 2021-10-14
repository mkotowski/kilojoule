#pragma once

#include <ctime> // time_t
#include <memory>
#include <array>
#include <string>
#include <functional>

class Terminal;

class erow
{
public:
	std::string chars{};
	std::string render{};
	std::string hl{};
	erow() = default;
	~erow() = default;
};

struct editorSyntax
{
	const char*  filetype;
	const char** filematch;
	const char** keywords;
	const char*  singleline_comment_start;
	int          flags;
};

class Editor
{
private:
	size_t cursorColumn{ 0 }; // the column position - x
	size_t cursorRow{ 0 };    // the row position - y

	size_t cursorRenderColumn{ 0 }; // rx

	size_t columnOffset{ 0 };
	size_t rowOffset{ 0 };

	bool dirtyFlag{ false };
	int  dirtyLevel{ 0 };

	std::vector<erow> rows{};

	std::string filename{};

	size_t screenRows{ 0 };
	size_t screenCols{ 0 };

	std::string statusmsg{};
	time_t      statusmsg_time{};

	struct editorSyntax* syntax;

public:
	Editor() = default;
	~Editor() = default;

	std::shared_ptr<Terminal> terminal = nullptr;

	bool shouldClose{ false };

	int Init(std::shared_ptr<Terminal> term);

	void RefreshScreen();

	// Filesystem operations
	void Open(const char* filename);
	// void        Save();

	// Text buffer manipulation
	void UpdateRow(size_t at);
	void InsertRow(size_t at, const char* s);

	void InsertNewline();

	// User input
	void ProcessKeypress();

	// Interface
	void DrawRows(std::string& ab) const;
	void DrawStatusBar(std::string& ab) const;
	void DrawMessageBar(std::string& ab);

	void SetStatusMessage(const char* fmt, ...);
	// std::string Prompt(const char*                           prompt,
	//                    std::function<void(const char*, int)> callback);

	// Cursor and view offset
	void Scroll();
	void MoveCursor(int key);

	// static int  RowCxToRx(erow* row, size_t cx);
	// static int  RowRxToCx(erow* row, int rx);
	// void        DelRow(size_t at);
	// void        RowInsertChar(erow* row, size_t at, char c);
	// void        RowAppendString(erow* row, char* s, size_t len);
	// void        RowDelChar(erow* row, size_t at);
	// void        InsertChar(int c);
	// void        DelChar();
	// void        Find();
	// void        SelectSyntaxHighlight();
	// static int  SyntaxToColor(int hl);
	// void        FindCallback(const char* query, int key);
	// void        UpdateSyntax(erow* row) const;

	// [[nodiscard]] std::string RowsToString() const;

	// enum editorHighlight
	// {
	// 	HL_NORMAL = 0,
	// 	HL_COMMENT,
	// 	HL_KEYWORD1,
	// 	HL_KEYWORD2,
	// 	HL_STRING,
	// 	HL_NUMBER,
	// 	HL_MATCH
	// };
};
