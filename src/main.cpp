#include <memory>

#include "Terminal.hpp"
#include "Editor.hpp"

int
main(int argc, char* argv[])
{
	Terminal terminal{};
	Editor   editor{};

	terminal.SetMode(TerminalMode::Raw);

	if (terminal.GetWindowSize() == -1) {
		throw("GetWindowSize");
	}

	editor.Init(std::make_shared<Terminal>(terminal));

	editor.SetStatusMessage(
	  "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

	if (argc >= 2) {
		editor.Open(argv[1]);
	}

	while (!editor.shouldClose) {
		editor.RefreshScreen();
		editor.ProcessKeypress();
	}

	terminal.SetMode(TerminalMode::Cooked);
	return 0;
}
