#pragma once

#include <termios.h>

class Terminal
{
private:
	termios initTermios{};
	termios currentTermios{};

	int rows{ 0 };
	int columns{ 0 };

public:
	Terminal(/* args */) = default;
	~Terminal() = default;

	int ToggleRawMode(bool state);

	int GetWindowSize();
	int GetCursorPosition();

	[[nodiscard]] int GetRows() const { return rows; }
	[[nodiscard]] int GetColumns() const { return columns; }
};
