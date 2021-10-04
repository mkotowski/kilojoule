#include <iostream>

#include "constants.hpp"

#include "Terminal.hpp"
#include "Editor.hpp"

int
main(int argc, char* argv[])
{
	std::cout << argc << " " << *argv << std::endl;

	Terminal terminal{};
	Editor   editor{};

	terminal.ToggleRawMode(true);
	if (terminal.GetWindowSize() == -1) {
		throw("GetWindowSize");
	}

	editor.Init(terminal.GetRows(), terminal.GetColumns());

	if (argc >= 2) {
		editor.Open(argv[1]);
	}

	editor.SetStatusMessage(
	  "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

	while (!editor.shouldClose) {
		editor.RefreshScreen();
		editor.ProcessKeypress();
	}

	terminal.ToggleRawMode(false);

	return 0;
}
