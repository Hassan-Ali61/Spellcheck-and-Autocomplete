# Trie-Based Autocomplete and Spell Checker (C++)

A console application written in C++ that stores a dictionary of 370,105 English words in a Trie, suggests completions for a prefix, corrects misspelled words using edit distance, and learns from use by ranking words according to how often they are chosen.

Built as a Data Structures course project at FAST-NUCES Karachi, without using STL containers such as `vector` or `pair`. Everything is built from arrays, structs, pointers, and a hand-written linked list.

**Author:** Hassan Ali Khan, BS Artificial Intelligence, FAST-NUCES Karachi

## Features

- **Insert and search** words in a Trie, case-insensitive, letters a to z only.
- **Autocomplete:** the top 5 completions of a prefix, ranked by usage frequency.
- **Spell check:** the closest dictionary words within 2 edits, ranked by edit distance first and frequency second. A correctly spelled word is reported as such.
- **Learns from use:** a word's frequency goes up when it is searched or picked from a suggestion list, so it moves higher in future results.
- **Recent searches:** shows the last 5 words you used (circular queue).
- **Undo:** reverses the most recent use and rolls that word's frequency back (stack).
- **Large dictionary:** loads `words_alpha.txt` at start-up and reports how many words were loaded and skipped, and how long it took.
- **Query timing:** every autocomplete and spell check query prints how long it took.
- **Input validation:** letters typed where a number is expected, extra words on one line, and invalid words do not crash the program or break the menu.
- **Colored output:** menus, results, and messages are colored with ANSI escape codes. Colors turn off automatically when the output is not a terminal, or when the `NO_COLOR` environment variable is set.

## Sample session

Autocomplete:

```
Enter a prefix: aj

5 results match your search (Query Duration: 0.0169 ms)
Word 1: ajaja | Frequency: 1
Word 2: ajangle | Frequency: 1
Word 3: ajar | Frequency: 1
Word 4: ajari | Frequency: 1
Word 5: ajatasatru | Frequency: 1
Select your search (1 - 5 , 0 to skip):
```

Spell check:

```
Enter a word: intrnational

Query Duration: 248.632 ms.
Word 1: international | Distance: 1 | Frequency: 1
Word 2: intonational | Distance: 1 | Frequency: 1
Word 3: intranational | Distance: 1 | Frequency: 1
Word 4: antinational | Distance: 2 | Frequency: 1
Word 5: incarnational | Distance: 2 | Frequency: 1
Select your search (1 - 5 , 0 to skip):
```

## Getting started

**Requirements:** a C++ compiler such as `g++` (C++11 or later).

**1. Get the dictionary.** Download `words_alpha.txt` (for example from the `dwyl/english-words` repository on GitHub) and place it in the folder you will run the program from. If the file is not found, the program prints a message and falls back to 19 built-in sample words.

**2. Compile:**

```
g++ -O2 -Wall -Wextra main.cpp -o trie.exe
```

**3. Run (Windows PowerShell):**

```
.\trie.exe
```

On Linux or macOS, compile to `trie` and run `./trie`.

## Menu

| Option | What it does |
| --- | --- |
| a. Insert a word | Adds a word to the dictionary |
| b. Search a word | Checks whether a word exists; a successful search counts as a use |
| c. Autocomplete | Shows up to 5 ranked completions and lets you select one |
| d. Spell Check | Reports correct spelling, or lists the closest words with distance and frequency |
| e. Recent Searches | Shows the last 5 used words, oldest to newest |
| f. Undo Search | Undoes the most recent use and lowers that word's frequency by one (never below 1) |
| g. exit | Closes the program |

A "use" is a successful search or a word selected from the autocomplete or spell check list. Showing suggestions alone does not change any frequency.

## Data structures and algorithms

| Component | Data structure or algorithm | Purpose |
| --- | --- | --- |
| Dictionary | Trie (26 child pointers per node) | Insert, search, and prefix lookup in time proportional to word length |
| Autocomplete traversal | Depth-first search (recursion) | Visit every word below a prefix |
| Ranking | Fixed-size array kept sorted by insertion sort | Keep only the best K results without storing every match |
| Spell check | Dynamic programming (edit distance) | Measure how different two words are |
| Spell suggestions | DFS over the Trie with pruning | Find close words without comparing against every word |
| Recent searches | Circular queue on an array of 5 | The oldest search drops out when the queue is full |
| Undo | Stack as a singly linked list | The latest use is undone first |

### Trie

Each node holds 26 child pointers, a flag that marks the end of a word, and a usage frequency. Inserting a word follows one child per letter and creates missing nodes, then marks the last node as a word end. Search follows the same path and succeeds only if the final node is a word end. Both take time proportional to the length of the word, not the size of the dictionary.

Every word is normalized first: it is lowercased, and anything containing a non-letter is rejected, so invalid characters can never index outside the child array.

### Autocomplete

1. Walk the prefix from the root. If a letter is missing, there are no suggestions.
2. From the prefix node, run a depth-first search in alphabetical order. The word built so far is passed down, so every word-end node knows its full word.
3. Each word found is placed into a sorted array of the best 5 using an insertion step: stronger words shift the weaker ones down, and a word that does not beat the current last place is ignored.

Because the search runs in alphabetical order and only strictly better words move ahead, words with equal frequency stay in alphabetical order.

### Edit distance

Edit distance is the smallest number of single-letter insertions, deletions, and replacements needed to turn one word into another. A table `dp[i][j]` holds the answer for the first `i` letters of word A and the first `j` letters of word B:

- The first row and column are `0, 1, 2, ...` (turning a word into or from an empty word).
- If the last letters match, the cell copies the diagonal value.
- Otherwise the cell is `1 + min(above, left, diagonal)`.
- The bottom-right cell is the answer.

Example, "cat" to "cut":

| A \ B | (empty) | c | u | t |
| --- | --- | --- | --- | --- |
| (empty) | 0 | 1 | 2 | 3 |
| c | 1 | 0 | 1 | 2 |
| a | 2 | 1 | 1 | 2 |
| t | 3 | 2 | 2 | 1 |

The distance is 1. The table has a fixed size of 51 by 51, so words longer than 50 letters are not compared.

### Spell suggestions

A depth-first search walks the whole Trie. At each word-end node it computes the edit distance to the typed word and keeps the best 5 within a distance of 2, ordered by smaller distance first, then higher frequency, then alphabetical order. Two checks avoid unnecessary work:

- A branch is abandoned once the word built so far is more than 2 letters longer than the typed word, since every word below it is even longer.
- The edit distance is skipped for words that are more than 2 letters shorter than the typed word, since their distance must be at least 3.

### Recent searches and undo

Every use is recorded in two places. The circular queue keeps the last 5 words for the "Recent Searches" display. The stack records every use in order. Undo pops the latest word from the stack and lowers its frequency by one, never below 1. Undo does not change the recent-searches list, which is a log of what happened.

## Complexity

Let `L` be the length of a word, `P` the length of a prefix, `M` the number of words below the prefix, `K` the number of results kept (5), and `m`, `n` the lengths of two words.

| Operation | Time |
| --- | --- |
| Insert, search, record use, undo use | O(L) |
| Autocomplete | O(P) to reach the prefix, plus O(M × K) to rank the words below it |
| Edit distance | O(m × n) |
| Spell check | one edit distance computation per dictionary word of a close length, so O(W × m × n) in the worst case, where W is the number of such words |
| Recent searches enqueue, undo push and pop | O(1) |

Each Trie node uses about 216 bytes on a 64-bit build (8 bytes for the flag and counter plus 26 pointers of 8 bytes each), so memory grows with the number of nodes.

## Benchmarks

**Dictionary:** `words_alpha.txt`, 370,105 words loaded, 0 skipped.
**Compiler:** `g++ -O2`. The figures were measured on the single-file version of the program; splitting it into headers did not change the algorithms.
**Machine:** CPU ____________________, RAM ____________________, Windows ____________________.
**Memory after loading the dictionary:** 225.3 MB (process working set measured with PowerShell while the program waited at the menu).

**Method:** each figure comes from three separate program starts, with one query per start. Timers wrap only the function being measured, not the time spent typing. The "before" build is the version without the shorter-word skip described under Spell suggestions.

| Measurement | Run 1 | Run 2 | Run 3 | Mean |
| --- | --- | --- | --- | --- |
| Load time, before build (ms) | 391.769 | 391.090 | 356.916 | 379.9 |
| Load time, after build (ms) | 340.621 | 413.996 | 417.129 | 390.6 |
| Autocomplete "aj", before build (ms) | 0.0181 | 0.0251 | 0.0178 | 0.0203 |
| Autocomplete "aj", after build (ms) | 0.0169 | 0.0299 | 0.0295 | 0.0254 |
| Spell check "intrnational", before build (ms) | 388.286 | 343.569 | 366.019 | 366.0 |
| Spell check "intrnational", after build (ms) | 248.632 | 307.393 | 258.407 | 271.5 |

**Results:**

- **Load:** about 340 to 417 ms across all six runs (mean 385 ms) for 370,105 words. The loading code did not change between the builds, and the difference between the two means is within the run-to-run spread.
- **Autocomplete:** roughly 17 to 30 microseconds. It only visits the small part of the Trie below the prefix.
- **Spell check:** the shorter-word skip reduced the mean time for "intrnational" from 366.0 ms to 271.5 ms, about 26% faster (about 29% by the median). The slowest "after" run (307.4 ms) was still faster than the fastest "before" run (343.6 ms), so the improvement is larger than the run-to-run noise. Both builds returned the same five suggestions with the same distances.

**Observations:**

- Autocomplete is thousands of times faster than spell check because it only explores the subtree under the prefix, while spell check has to look at much of the Trie.
- Spell check is still slow, around 270 ms, because the search visits a large number of nodes and builds a string at each one.
- Memory is the cost of this speed: about 225 MB for 370,105 words, roughly 640 bytes per word, because every Trie node reserves 26 pointers.
- Only one spell check query was timed, so these numbers show the effect of the optimization on that query and should not be read as typical for every input.

## Testing

Tests run by hand through the menu:

| Test | Result |
| --- | --- |
| Spell check `cat` and `CAT` | Reported as spelled correctly (case-insensitive) |
| Spell check `cet` | `cat`, `cot`, `cut` at distance 1, then words at distance 2 |
| Spell check `xyzxyz` | "No suggestions found!" |
| Spell check `123` | "No suggestions found!" (invalid word rejected) |
| Spell check `intrnational` | `international`, `intonational`, `intranational` at distance 1 |
| A letter typed at a number prompt | "Enter a number!" and the menu continues normally |
| Search `apple`, then pick `apple` from autocomplete and from spell check | Frequency rose to 4 and `apple` appeared three times in Recent Searches |
| Undo once after that | Frequency dropped from 4 to 3 and the autocomplete order reflected it |
| Recent Searches and Undo before any use | "No history yet!" and "Nothing to undo!" |
| Load `words_alpha.txt` | 370,105 words loaded, 0 skipped |

## Limitations and future work

- Only the letters a to z are supported. Words with apostrophes, hyphens, or digits are rejected.
- Frequencies live in memory only and reset when the program exits. Saving words and frequencies to a file on exit is a natural next step.
- Spell check searches most of the Trie. A faster approach is to compute the edit distance one row at a time while walking down the Trie and stop a branch as soon as every value in the row exceeds the limit. This has not been implemented or measured.
- Each node reserves 26 pointers even when most are empty, which is memory-heavy for a large dictionary. Storing only the children that exist would reduce memory.
- Recent Searches records every use, so repeated searches for the same word appear more than once.
- The program is a console application. A graphical interface could be added on top of the same Trie.

## Project structure

```
main.cpp          menu, user input, output, and query timing
Trie.h            Trie (insert, search, autocomplete, spell check)
EditDistance.h    edit distance with a dynamic programming table
HistoryQueue.h    circular queue for recent searches
UndoStack.h       linked list stack for undo
Dictionary.h      dictionary file loader and built-in sample words
Console.h         console colors and input helpers
words_alpha.txt   dictionary file (not included, see Getting started)
README.md         this file
```

Every class lives in its own header file with its code written in the header, so the whole project still compiles with a single `g++` command on `main.cpp`.
