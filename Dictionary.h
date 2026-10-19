#pragma once

#include <fstream>
#include <string>

#include "Trie.h"

// A small built-in word list, used when the dictionary file is missing.
inline void loadSampleWords(Trie &trie)
{
    const std::string words[] = {
        "cat", "cut", "cot", "car", "care", "card", "cart",
        "dog", "dig", "dot", "door",
        "apple", "apply", "ape",
        "hello", "help", "world", "word", "ward"};

    for (const std::string &word : words)
        trie.Insert(word);
}

// Reads one word per line into the Trie. Returns false if the file cannot be opened.
// loaded = words inserted, skipped = lines rejected (empty, digits, symbols, ...).
inline bool loadDictionary(const std::string &filename, Trie &trie, int &loaded, int &skipped)
{
    loaded = 0;
    skipped = 0;

    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    std::string line;
    while (std::getline(file, line))
    {
        // Files saved on Windows end each line with "\r\n".
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (trie.Insert(line))
            loaded++;
        else
            skipped++;
    }
    return true;
}
