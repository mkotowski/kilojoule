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
