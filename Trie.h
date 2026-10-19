#pragma once

#include <cctype>
#include <iostream>
#include <string>

#include "EditDistance.h"

constexpr int ALPHABET_SIZE = 26;
constexpr int MAX_EDITS = 2; // spell suggestions only accept words within this many edits

// One autocomplete suggestion.
struct Suggestion
{
    std::string word;
    int freq = 0;
};

// One spell check suggestion.
struct Result
{
    std::string word;
    int freq = 0;
    int dist = 0;
};

struct TrieNode
{
    bool isEnd;
    int frequency;
    TrieNode *children[ALPHABET_SIZE];

    TrieNode() : isEnd(false), frequency(0)
    {
        for (int i = 0; i < ALPHABET_SIZE; i++)
            children[i] = nullptr;
    }
};

class Trie
{
public:
    Trie() : root(new TrieNode()) {}
    ~Trie() { destroy(root); }

    Trie(const Trie &) = delete;
    Trie &operator=(const Trie &) = delete;

    // Adds a word (or raises its frequency). Returns false if the word has non-letters.
    bool Insert(const std::string &w)
    {
        const std::string word = normalize(w);
        if (word.empty())
            return false;

        TrieNode *current = root;
        for (char ch : word)
        {
            const int index = ch - 'a';
            if (current->children[index] == nullptr)
                current->children[index] = new TrieNode();
            current = current->children[index];
        }

        current->isEnd = true;
        current->frequency++;
        return true;
    }

    bool Search(const std::string &w) const
    {
        const std::string word = normalize(w);
        if (word.empty())
            return false;

        const TrieNode *node = findNode(word);
        return node != nullptr && node->isEnd;
    }

    // Counts one use of the word (raises its frequency). Returns false if the word is not stored.
    bool RecordUse(const std::string &w)
    {
        const std::string word = normalize(w);
        if (word.empty())
            return false;

        TrieNode *node = findNode(word);
        if (node == nullptr || !node->isEnd)
            return false;

        node->frequency++;
        return true;
    }

    // Reverses one use. The frequency never drops below 1. Returns false if the word is not stored.
    bool UndoUse(const std::string &w)
    {
        const std::string word = normalize(w);
        if (word.empty())
            return false;

        TrieNode *node = findNode(word);
        if (node == nullptr || !node->isEnd)
            return false;

        if (node->frequency > 1)
            node->frequency--;
        return true;
    }

    // Fills top[] with up to k words that start with the prefix, most frequent first.
    // Returns how many were found.
    int Autocomplete(const std::string &pre, Suggestion top[], int k) const
    {
        if (k <= 0)
            return 0;

        const std::string prefix = normalize(pre);
        if (prefix.empty())
            return 0;

        const TrieNode *node = findNode(prefix);
        if (node == nullptr)
            return 0;

        int count = 0;
        collect(node, prefix, top, k, count);
        return count;
    }

    // Fills results[] with up to k dictionary words within MAX_EDITS of the typed word,
    // closest first (ties: higher frequency first). Returns how many were found.
    int SpellSuggest(const std::string &typeword, Result results[], int k) const
    {
        if (k <= 0)
            return 0;

        const std::string word = normalize(typeword);
        if (word.empty())
            return 0;

        int count = 0;
        spellCollect(root, "", word, results, k, count);
        return count;
    }

    // Prints every stored word in alphabetical order (handy for debugging).
    void Display() const
    {
        std::cout << "-----Stored words in Trie-----\n";
        printTrie(root, "");
        std::cout << std::endl;
    }

private:
    TrieNode *root;

    // Lowercases the word. Returns "" if it contains anything other than letters.
    static std::string normalize(const std::string &w)
    {
        std::string result;
        for (char c : w)
        {
            if (!std::isalpha(static_cast<unsigned char>(c)))
                return "";
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }

    // Follows the letters of an already normalized word. Returns nullptr if the path does not exist.
    TrieNode *findNode(const std::string &word) const
    {
        TrieNode *current = root;
        for (char ch : word)
        {
            current = current->children[ch - 'a'];
            if (current == nullptr)
                return nullptr;
        }
        return current;
    }

    static void destroy(TrieNode *node)
    {
        if (node == nullptr)
            return;
        for (int i = 0; i < ALPHABET_SIZE; i++)
            destroy(node->children[i]);
        delete node;
    }

    static void printTrie(const TrieNode *node, const std::string &word)
    {
        if (node == nullptr)
            return;
        if (node->isEnd)
            std::cout << word << std::endl;

        for (int i = 0; i < ALPHABET_SIZE; i++)
            printTrie(node->children[i], word + static_cast<char>('a' + i));
    }

    // ---- autocomplete ----

    // Inserts the word into top[], which is kept sorted by frequency (one step of insertion sort).
    static void addSuggestion(Suggestion top[], int k, int &count, const std::string &word, int freq)
    {
        // Full and not better than the last place: ignore it.
        if (count == k && freq <= top[count - 1].freq)
            return;

        int pos = (count < k) ? count : k - 1;
        while (pos > 0 && top[pos - 1].freq < freq)
        {
            top[pos] = top[pos - 1]; // shift weaker entries down
            pos--;
        }

        top[pos].word = word;
        top[pos].freq = freq;
        if (count < k)
            count++;
    }

    // Depth-first search in alphabetical order over everything below node.
    static void collect(const TrieNode *node, const std::string &word, Suggestion top[], int k, int &count)
    {
        if (node->isEnd)
            addSuggestion(top, k, count, word, node->frequency);

        for (int i = 0; i < ALPHABET_SIZE; i++)
        {
            if (node->children[i])
                collect(node->children[i], word + static_cast<char>('a' + i), top, k, count);
        }
    }

    // ---- spell check ----

    // Smaller distance wins; on equal distance, higher frequency wins.
    static bool isBetter(const Result &a, const Result &b)
    {
        if (a.dist < b.dist)
            return true;
        if (a.dist > b.dist)
            return false;
        return a.freq > b.freq;
    }

    static void addResult(Result results[], int k, int &count, const std::string &word, int freq, int dist)
    {
        Result candidate;
        candidate.word = word;
        candidate.freq = freq;
        candidate.dist = dist;

        // Full and not better than the last place: ignore it.
        if (count == k && !isBetter(candidate, results[count - 1]))
            return;

        int pos = (count < k) ? count : k - 1;
        while (pos > 0 && isBetter(candidate, results[pos - 1]))
        {
            results[pos] = results[pos - 1]; // shift weaker entries down
            pos--;
        }

        results[pos] = candidate;
        if (count < k)
            count++;
    }

    // Walks the whole Trie. "built" is the word spelled by the path from the root to node.
    static void spellCollect(const TrieNode *node, const std::string &built, const std::string &typed,
                             Result results[], int k, int &count)
    {
        const int diff = static_cast<int>(built.size()) - static_cast<int>(typed.size());

        // Every word below this node is even longer, so none can be close enough.
        if (diff > MAX_EDITS)
            return;

        // A word much shorter than the typed one needs more than MAX_EDITS edits, so skip the DP.
        if (node->isEnd && diff >= -MAX_EDITS)
        {
            const int dist = editDistance(built, typed);
            if (dist <= MAX_EDITS)
                addResult(results, k, count, built, node->frequency, dist);
        }

        for (int i = 0; i < ALPHABET_SIZE; i++)
        {
            if (node->children[i])
                spellCollect(node->children[i], built + static_cast<char>('a' + i), typed, results, k, count);
        }
    }
};
