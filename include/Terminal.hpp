#pragma once

#if defined(__linux__)
#include <termios.h>
#endif

enum class TerminalMode
{
	Raw,
	Cbreak, // "Rare"
	Cooked,
};

using TerminalFlags = termios;

class Terminal
{
private:
	TerminalFlags initFlags{};
	TerminalFlags currentFlags{};

	int rows{ 0 };
	int columns{ 0 };

	TerminalMode currentMode = TerminalMode::Cooked;

public:
	Terminal();
	~Terminal() = default;

	TerminalMode SetMode(TerminalMode newMode);

	static bool isCookedModeRestoredProperly;
	static void ForceCookedMode();

	int GetWindowSize();
	int GetCursorPosition();

	[[nodiscard]] int GetRows() const { return rows; }
	[[nodiscard]] int GetColumns() const { return columns; }
};
