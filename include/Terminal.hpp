#pragma once

#if defined(__linux__)
#include <termios.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// #include <winsock2.h>
// #pragma comment(lib, "Ws2_32.lib")

#include <io.h>

#define read   _read
#define write  _write
#define sscanf sscanf_s
#define strdup _strdup
// #define fopen  fopen_s

/* Standard file descriptors.  */
#define STDIN_FILENO  0 /* Standard input.  */
#define STDOUT_FILENO 1 /* Standard output.  */
#define STDERR_FILENO 2 /* Standard error output.  */

using TerminalFlags = DWORD;

#endif

#if defined(__linux__)
using TerminalFlags = termios;
#endif

#include "constants.hpp"

enum class TerminalMode
{
	Raw,
	Cbreak, // "Rare"
	Cooked,
};

enum Key : int
{
	Backspace = 127,
	ArrowLeft = 1000,
	ArrowRight,
	ArrowUp,
	ArrowDown,
	Del,
	Home,
	End,
	PageUp,
	PageDown
};

class Terminal
{
private:
	TerminalFlags initFlags{};
	TerminalFlags currentFlags{};

	unsigned int rows{ 0 };
	unsigned int columns{ 0 };

	TerminalMode currentMode = TerminalMode::Cooked;

public:
	Terminal();
	~Terminal() = default;

	TerminalMode SetMode(TerminalMode newMode);

	static bool isCookedModeRestoredProperly;
	static void ForceCookedMode();

	static std::string SetCursorPositionEscapeSequence(unsigned int row,
	                                                   unsigned int column);

	int GetWindowSize();
	int GetCursorPosition();
	// void SetCursorPosition(unsigned int row, unsisgned int column);

	[[nodiscard]] int GetRows() const { return rows; }
	[[nodiscard]] int GetColumns() const { return columns; }

	static void Write(const std::string& content);
	static void Write(const char* content, size_t length);
	static void Write(const char* content);

	static int Read();
};
