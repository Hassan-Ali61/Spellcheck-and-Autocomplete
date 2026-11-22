#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "Console.h"
#include "Dictionary.h"
#include "HistoryQueue.h"
#include "LiveSearch.h"
#include "Trie.h"
#include "UndoStack.h"

using namespace std;

namespace
{
    constexpr int TOP_K = 5; // how many suggestions are shown
    const char *const DICTIONARY_FILE = "words_alpha.txt";

    // Everything the menu works with.
    struct App
    {
        Trie trie;
        HistoryQueue history;
        UndoStack undo;
    };

    // Measures how long something takes. Create it right before the call you want to time.
    class Timer
    {
    public:
        Timer() : start(chrono::steady_clock::now()) {}

        double elapsedMs() const
        {
            return chrono::duration<double, milli>(chrono::steady_clock::now() - start).count();
        }

    private:
        chrono::steady_clock::time_point start;
    };

    string formatNumber(double value)
    {
        ostringstream text;
        text << value;
        return text.str();
    }

    // ---- input ----

    char readChoice()
    {
        char choice;
        cout << console::prompt("Enter your choice (a-h): ");
        if (!(cin >> choice))
            return 'h'; // input ended (Ctrl+Z / Ctrl+D): leave the program cleanly
        clearInput();
        return static_cast<char>(tolower(static_cast<unsigned char>(choice)));
    }

    string readWord(const string &label)
    {
        string word;
        cout << "\n"
             << console::prompt(label + ": ");
        cin >> word;
        clearInput();
        return word;
    }

    // Asks which listed result to use. Returns 1 to count, or 0 if skipped or the input was invalid.
    int readSelection(int count)
    {
        int selection = 0;
        cout << console::prompt("Select your search (1 - " + to_string(count) + " , 0 to skip): ");
        cin >> selection;
        const bool notANumber = cin.fail();
        clearInput();

        if (notANumber)
        {
            cout << "\n"
                 << console::error("Enter a number!") << "\n";
            return 0;
        }
        if (selection < 0 || selection > count)
        {
            cout << "\n"
                 << console::error("Enter a valid number!") << "\n";
            return 0;
        }
        return selection;
    }

    // ---- output ----

    void showMenu()
    {
        const string line = "=========================================================";
        cout << "\n"
             << console::heading(line) << "\n"
             << console::heading("               AUTOCOMPLETE AND SPELL CHECK                ") << "\n"
             << console::heading(line) << "\n";

        cout << console::key("a.") << " Insert a word\n";
        cout << console::key("b.") << " Search a word\n";
        cout << console::key("c.") << " Autocomplete\n";
        cout << console::key("d.") << " Spell Check\n";
        cout << console::key("e.") << " Recent Searches\n";
        cout << console::key("f.") << " Undo Search\n";
        cout << console::key("g.") << " Live Search\n";
        cout << console::key("h.") << " exit\n";
    }

    void showSuggestion(int rank, const string &word, const string &details)
    {
        cout << console::info("Word " + to_string(rank) + ":") << " "
             << console::bold(word) << console::dim(" \t" + details) << endl;
    }

    // ---- actions ----

    // A "use" = a successful search, or a word picked from a suggestion list.
    void registerUse(App &app, const string &word)
    {
        app.trie.RecordUse(word);
        app.history.enqueue(word);
        app.undo.push(word);
    }

    void loadWords(App &app)
    {
        int loaded = 0;
        int skipped = 0;

        Timer timer;
        const bool ok = loadDictionary(DICTIONARY_FILE, app.trie, loaded, skipped);
        const double ms = timer.elapsedMs();

        if (ok)
        {
            cout << "\n"
                 << console::success("Loaded " + to_string(loaded) + " words, skipped " +
                                     to_string(skipped) + " words in " + formatNumber(ms) + " milliseconds.")
                 << "\n";
        }
        else
        {
            cout << "\n"
                 << console::warning("Dictionary not found, using sample words.") << "\n";
            loadSampleWords(app.trie);
        }
    }

    void insertWord(App &app)
    {
        const string word = readWord("Enter word to insert");

        if (app.trie.Insert(word))
            cout << "\n"
                 << console::success("Word inserted successfully!");
        else
            cout << "\n"
                 << console::error("Enter valid string literals!");

        cout << endl;
    }

    void searchWord(App &app)
    {
        const string word = readWord("Enter word to search");

        if (app.trie.Search(word))
        {
            cout << "\n"
                 << console::success("Word present!");
            registerUse(app, word);
        }
        else
            cout << "\n"
                 << console::error("word not found!");

        cout << endl;
    }

    void autocomplete(App &app)
    {
        const string prefix = readWord("Enter a prefix");
        Suggestion top[TOP_K];

        Timer timer;
        const int count = app.trie.Autocomplete(prefix, top, TOP_K);
        const double ms = timer.elapsedMs();

        cout << "\n"
             << count << " results match your search "
             << console::dim("(Query Duration: " + formatNumber(ms) + " ms)") << "\n";

        if (count == 0)
            return;

        for (int i = 0; i < count; i++)
            showSuggestion(i + 1, top[i].word, "| Frequency: " + to_string(top[i].freq));

        const int selection = readSelection(count);
        if (selection > 0)
            registerUse(app, top[selection - 1].word);
    }

    void spellCheckWord(App &app, const string &word)
    {
        Result results[TOP_K];

        Timer timer;
        const int count = app.trie.SpellSuggest(word, results, TOP_K);
        const double ms = timer.elapsedMs();

        cout << "\n"
             << console::dim("Query Duration: " + formatNumber(ms) + " ms.") << "\n";

        if (count > 0 && results[0].dist == 0)
        {
            cout << console::success("Word is spelled Correctly!") << "\n";
        }
        else if (count > 0)
        {
            for (int i = 0; i < count; i++)
                showSuggestion(i + 1, results[i].word,
                               "| Distance: " + to_string(results[i].dist) +
                                   " \t| Frequency: " + to_string(results[i].freq));

            const int selection = readSelection(count);
            if (selection > 0)
                registerUse(app, results[selection - 1].word);
        }
        else
        {
            cout << console::warning("No suggestions found!") << "\n";
        }
    }

    void spellCheck(App &app)
    {
        spellCheckWord(app, readWord("Enter a word"));
    }

    // Search box that shows suggestions after every letter you type.
    void liveSearch(App &app)
    {
        const LiveSearchOutcome outcome = runLiveSearch(app.trie, TOP_K);

        switch (outcome.type)
        {
        case LiveSearchOutcome::UNAVAILABLE:
            cout << "\n"
                 << console::warning("Live Search needs an interactive terminal.") << "\n";
            break;
        case LiveSearchOutcome::CANCELLED:
            cout << "\n"
                 << console::dim("Live search cancelled.") << "\n";
            break;
        case LiveSearchOutcome::SELECTED:
            cout << "\n"
                 << console::success("Selected: " + outcome.word) << "\n";
            registerUse(app, outcome.word);
            break;
        case LiveSearchOutcome::SUBMITTED:
            if (app.trie.Search(outcome.word))
            {
                cout << "\n"
                     << console::success("Word present!") << "\n";
                registerUse(app, outcome.word);
            }
            else
                spellCheckWord(app, outcome.word); // not in the dictionary: show corrections
            break;
        }
    }

    void showHistory(const App &app)
    {
        cout << "\n"
             << console::info("Recent searches (oldest to newest): ") << "\n";

        if (app.history.isEmpty())
            cout << console::dim("No history yet!") << "\n";
        else
            app.history.display();
    }

    void undoLastUse(App &app)
    {
        string word;
        if (!app.undo.pop(word))
        {
            cout << console::warning("Nothing to undo!") << "\n";
            return;
        }

        if (app.trie.UndoUse(word))
            cout << console::success("Undid use of: " + word) << endl;
        else
            cout << console::error("Word not found!") << "\n";
    }
}

int main()
{
    console::init();

    App app;
    loadWords(app);

    char choice;
    do
    {
        showMenu();
        choice = readChoice();

        switch (choice)
        {
        case 'a':
            insertWord(app);
            break;
        case 'b':
            searchWord(app);
            break;
        case 'c':
            autocomplete(app);
            break;
        case 'd':
            spellCheck(app);
            break;
        case 'e':
            showHistory(app);
            break;
        case 'f':
            undoLastUse(app);
            break;
        case 'g':
            liveSearch(app);
            break;
        case 'h':
            cout << "\n"
                 << console::info("Good Bye!") << "\n";
            break;
        default:
            cout << console::error("Invalid choice! ");
            break;
        }
    } while (choice != 'h');

    return 0;
}
