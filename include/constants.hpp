#pragma once

#include <string>
#include <array>

namespace kilojoule {
inline const char* version{ "0.1.0" };
namespace defaults {
inline constexpr int tabStop{ 3 };
inline constexpr int quitTimes{ 3 };
inline constexpr int messageWaitDuration{ 5 };
}
}

enum class Platform
{
	Linux,
	Windows,
	Android,
};

#if defined(__linux__) && !defined(__ANDROID__)
constexpr Platform BuildPlatform = Platform::Linux;
#elif defined(__ANDROID__)
constexpr Platform BuildPlatform = Platform::Android;
#elif defined(_WIN32) || defined(_WIN64)
constexpr Platform BuildPlatform = Platform::Windows;
#endif

namespace escapeSequences {
inline const char* eraseInLine = "\x1b[K";
inline const char* hideCursor = "\x1b[?25l";
inline const char* showCursor = "\x1b[?25h";
inline const char* cursorMaxForward = "\x1b[999C";
inline const char* cursorMaxDown = "\x1b[999B";
inline const char* cursorMaxForwardAndDown = "\x1b[999C\x1b[999B";
inline const char* cursorRepositionLeftmostTop = "\x1b[H";
inline const char* clearEntireScreen = "\x1b[2J";
namespace color {
inline const char* reset = "\x1b[m";
inline const char* reverse = "\x1b[7m";
inline const char* defaultForeground = "\x1b[39m";
[[nodiscard]] inline std::string
getEscapeSequence(int color)
{
	return std::string("\x1b[" + std::to_string(color) + "m");
}
}
namespace deviceStatusReport {
inline const char* cursorPosition = "\x1b[6n";
}
}

#if defined(__linux__)
namespace defaultTermios {
inline constexpr std::array<unsigned char, 32> c_cc = {
	3, 28, 127, 21, 4, 0, 1, 0, 17, 19, 26, 0, 18, 15, 23, 22,
	0, 0,  0,   0,  0, 0, 0, 0, 0,  0,  0,  0, 0,  0,  0,  0
};
inline constexpr int c_iflag = 1280;
inline constexpr int c_oflag = 5;
inline constexpr int c_cflag = 191;
inline constexpr int c_lflag = 35387;
inline constexpr int c_ispeed = 15;
inline constexpr int c_ospeed = 15;
inline constexpr int c_line = 0;
}
#endif
