#pragma once

#include <termios.h>

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
	Terminal(/* args */);
	~Terminal() = default;

	TerminalMode SetMode(TerminalMode newMode);
	void         ResetMode();

	int GetWindowSize();
	int GetCursorPosition();

	[[nodiscard]] int GetRows() const { return rows; }
	[[nodiscard]] int GetColumns() const { return columns; }
};
