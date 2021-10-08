#pragma once

#include <string>

namespace kilojoule {
inline const char* version{ "0.1.0" };
namespace defaults {
inline constexpr int tabStop{ 3 };
inline constexpr int quitTimes{ 3 };
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
}
namespace deviceStatusReport {
inline const char* cursorPosition = "\x1b[6n";
}
}
