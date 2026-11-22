#pragma once

#include <cstdio>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

// Reads single key presses without waiting for Enter and without echoing them.
namespace keyboard
{
    constexpr int ENTER = 10;
    constexpr int ESCAPE = 27;
    constexpr int BACKSPACE = 8;
    constexpr int OTHER = -1; // arrow keys, function keys, ...

    // False when input comes from a file or a pipe instead of a keyboard.
    inline bool isInteractive()
    {
#ifdef _WIN32
        return _isatty(_fileno(stdin)) != 0;
#else
        return isatty(STDIN_FILENO) != 0;
#endif
    }

#ifdef _WIN32

    // Nothing to set up: _getch() already reads one key at a time.
    // (The empty constructor only exists so every platform can create a RawMode object.)
    class RawMode
    {
    public:
        RawMode() {}
    };

    inline int readKey()
    {
        const int c = _getch();
        if (c == 0 || c == 224) // arrow and function keys send two codes
        {
            _getch();
            return OTHER;
        }
        if (c == 13)
            return ENTER;
        if (c == 8)
            return BACKSPACE;
        if (c == 3) // Ctrl+C
            return ESCAPE;
        return c;
    }

#else

    // While alive, the terminal delivers every key press immediately and does not echo it.
    // The old settings are restored when the object is destroyed.
    class RawMode
    {
    public:
        RawMode() : active(false)
        {
            if (tcgetattr(STDIN_FILENO, &saved) != 0)
                return;

            termios raw = saved;
            raw.c_lflag &= ~(ICANON | ECHO | ISIG); // ISIG off: Ctrl+C arrives as a key, so we can restore the terminal
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            active = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }

        ~RawMode()
        {
            if (active)
                tcsetattr(STDIN_FILENO, TCSANOW, &saved);
        }

        RawMode(const RawMode &) = delete;
        RawMode &operator=(const RawMode &) = delete;

    private:
        termios saved;
        bool active;
    };

    inline int readKey()
    {
        unsigned char c;
        if (read(STDIN_FILENO, &c, 1) != 1)
            return ESCAPE; // input closed
        if (c == '\r' || c == '\n')
            return ENTER;
        if (c == 127 || c == 8)
            return BACKSPACE;
        if (c == 3) // Ctrl+C
            return ESCAPE;
        if (c != 27)
            return c;

        // Escape: either the Esc key alone, or the start of an arrow-key sequence like ESC [ A.
        struct pollfd input = {STDIN_FILENO, POLLIN, 0};
        if (poll(&input, 1, 50) <= 0)
            return ESCAPE;

        unsigned char next;
        if (read(STDIN_FILENO, &next, 1) != 1)
            return ESCAPE;
        if (next == '[' || next == 'O')
        {
            // Swallow the rest of the sequence (it ends with a byte between 0x40 and 0x7E).
            while (poll(&input, 1, 5) > 0 && read(STDIN_FILENO, &c, 1) == 1)
            {
                if (c >= 0x40 && c <= 0x7E)
                    break;
            }
        }
        return OTHER;
    }

#endif
}
