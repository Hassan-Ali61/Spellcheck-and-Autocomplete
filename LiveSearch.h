#pragma once

#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Console.h"
#include "EditDistance.h"
#include "Keyboard.h"
#include "Trie.h"

constexpr int LIVE_MAX_SUGGESTIONS = 9; // selected with the keys 1 to 9
constexpr int LIVE_LIST_WIDTH = 24;     // column where the frequency is printed

struct LiveSearchOutcome
{
    enum Type
    {
        CANCELLED,   // Esc, or Enter on an empty line
        SELECTED,    // a suggestion was picked with a number key
        SUBMITTED,   // Enter on typed text
        UNAVAILABLE  // not an interactive terminal
    };

    Type type = CANCELLED;
    std::string word;
};

namespace live
{
    inline std::string formatMs(double ms)
    {
        std::ostringstream text;
        text << std::fixed << std::setprecision(3) << ms;
        return text.str();
    }

    // Draws the prompt and the suggestion list. The cursor starts and ends on the prompt line,
    // right after the typed text, so the next draw can simply start from there.
    inline void draw(const std::string &typed, const Suggestion top[], int count, int k, double ms)
    {
        const std::string label = "Search: ";
        std::cout << "\r" << console::CLEAR_BELOW;
        std::cout << console::prompt(label) << console::bold(typed) << "\n";

        for (int i = 0; i < k; i++)
        {
            if (i < count)
            {
                const std::string &word = top[i].word;
                const std::string matched = word.substr(0, typed.size()); // the part the user typed
                const std::string rest = word.substr(typed.size());
                const int padding = (static_cast<int>(word.size()) < LIVE_LIST_WIDTH)
                                        ? LIVE_LIST_WIDTH - static_cast<int>(word.size())
                                        : 1;

                std::cout << "  " << console::key(std::to_string(i + 1) + ".") << " "
                          << console::success(matched) << rest << std::string(padding, ' ')
                          << console::dim("freq " + std::to_string(top[i].freq));
            }
            std::cout << "\n";
        }

        if (typed.empty())
            std::cout << console::dim("Start typing a word...");
        else if (count == 0)
            std::cout << console::warning("No suggestions. Press Enter to run spell check.");
        else
            std::cout << console::dim(std::to_string(count) + (count == 1 ? " suggestion (" : " suggestions (") +
                                      formatMs(ms) + " ms)");
        std::cout << "\n";

        std::cout << console::dim("1-" + std::to_string(k) + " select   Enter search   Backspace delete   Esc cancel");

        // Go back up to the prompt line, right after the typed text.
        std::cout << console::cursorUp(k + 2)
                  << console::cursorColumn(static_cast<int>(label.size() + typed.size()) + 1)
                  << std::flush;
    }
}

// Interactive search box: suggestions update after every letter you type.
// Number keys pick a suggestion, Enter submits the typed text, Esc cancels.
inline LiveSearchOutcome runLiveSearch(const Trie &trie, int k)
{
    LiveSearchOutcome outcome;

    if (!console::ansiSupported() || !keyboard::isInteractive())
    {
        outcome.type = LiveSearchOutcome::UNAVAILABLE;
        return outcome;
    }

    if (k > LIVE_MAX_SUGGESTIONS)
        k = LIVE_MAX_SUGGESTIONS;

    Suggestion top[LIVE_MAX_SUGGESTIONS];
    std::string typed;
    int count = 0;
    double ms = 0;

    keyboard::RawMode rawMode; // restores the terminal when this function ends
    std::cout << "\n";
    live::draw(typed, top, count, k, ms);

    while (true)
    {
        const int key = keyboard::readKey();

        if (key == keyboard::ESCAPE)
        {
            outcome.type = LiveSearchOutcome::CANCELLED;
            break;
        }
        if (key == keyboard::ENTER)
        {
            if (!typed.empty())
            {
                outcome.type = LiveSearchOutcome::SUBMITTED;
                outcome.word = typed;
            }
            break;
        }

        if (key == keyboard::BACKSPACE)
        {
            if (typed.empty())
                continue;
            typed.erase(typed.size() - 1);
        }
        else if (key >= '1' && key <= '9')
        {
            const int index = key - '1';
            if (index >= count)
                continue; // no suggestion with that number
            outcome.type = LiveSearchOutcome::SELECTED;
            outcome.word = top[index].word;
            break;
        }
        else if (key > 0 && key < 128 && std::isalpha(key) && static_cast<int>(typed.size()) < MAX_WORD_LENGTH)
        {
            typed += static_cast<char>(std::tolower(key));
        }
        else
        {
            continue; // ignore every other key
        }

        if (typed.empty())
        {
            count = 0;
            ms = 0;
        }
        else
        {
            const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            count = trie.Autocomplete(typed, top, k);
            ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        }
        live::draw(typed, top, count, k, ms);
    }

    // Remove the search box from the screen; the cursor is already on its first line.
    std::cout << "\r" << console::CLEAR_BELOW << std::flush;
    return outcome;
}
