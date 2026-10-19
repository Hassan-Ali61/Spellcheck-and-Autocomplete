#pragma once

#include <cstdlib>
#include <ios>
#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // stop windows.h from defining min/max macros
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

// Colored console output using ANSI escape codes.
// Colors are switched off automatically when the output is not a terminal,
// or when the NO_COLOR environment variable is set.
namespace console
{
    const char *const RESET = "\033[0m";
    const char *const RED = "\033[31m";
    const char *const GREEN = "\033[32m";
    const char *const YELLOW = "\033[33m";
    const char *const CYAN = "\033[36m";
    const char *const GRAY = "\033[90m";
    const char *const BOLD = "\033[1m";
    const char *const BOLD_CYAN = "\033[1;36m";

    inline bool &colorEnabled()
    {
        static bool enabled = false;
        return enabled;
    }

    // Call once at the start of main().
    inline void init()
    {
        if (std::getenv("NO_COLOR") != nullptr)
            return;

#ifdef _WIN32
        // Older Windows consoles ignore escape codes until this mode is switched on.
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (out == INVALID_HANDLE_VALUE || !GetConsoleMode(out, &mode))
            return; // output is redirected, not a console
        colorEnabled() = SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
        colorEnabled() = isatty(STDOUT_FILENO) != 0;
#endif
    }

    inline std::string paint(const std::string &text, const char *color)
    {
        if (!colorEnabled())
            return text;
        return std::string(color) + text + RESET;
    }

    inline std::string heading(const std::string &text) { return paint(text, BOLD_CYAN); }
    inline std::string key(const std::string &text) { return paint(text, YELLOW); }
    inline std::string prompt(const std::string &text) { return paint(text, YELLOW); }
    inline std::string success(const std::string &text) { return paint(text, GREEN); }
    inline std::string error(const std::string &text) { return paint(text, RED); }
    inline std::string warning(const std::string &text) { return paint(text, YELLOW); }
    inline std::string info(const std::string &text) { return paint(text, CYAN); }
    inline std::string dim(const std::string &text) { return paint(text, GRAY); }
    inline std::string bold(const std::string &text) { return paint(text, BOLD); }
}

// Clears a failed input state and throws away the rest of the current line.
inline void clearInput()
{
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
