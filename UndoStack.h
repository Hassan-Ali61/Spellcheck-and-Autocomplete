#pragma once

#include <iostream>
#include <string>

// Remembers every use so it can be undone, newest first (stack on a singly linked list).
class UndoStack
{
public:
    UndoStack() : top(nullptr) {}

    ~UndoStack()
    {
        while (top != nullptr)
        {
            Node *next = top->next;
            delete top;
            top = next;
        }
    }

    UndoStack(const UndoStack &) = delete;
    UndoStack &operator=(const UndoStack &) = delete;

    bool isEmpty() const { return top == nullptr; }

    void push(const std::string &word) { top = new Node(word, top); }

    // Removes the newest word and stores it in out. Returns false if the stack is empty.
    bool pop(std::string &out)
    {
        if (isEmpty())
            return false;

        Node *node = top;
        top = top->next;
        out = node->word;
        delete node;
        return true;
    }

    // Prints the words from newest to oldest. Prints nothing if empty.
    void display() const
    {
        if (isEmpty())
            return;

        for (const Node *node = top; node != nullptr; node = node->next)
            std::cout << " | " << node->word;
        std::cout << " |\n";
    }

private:
    struct Node
    {
        std::string word;
        Node *next;
        Node(const std::string &w, Node *n) : word(w), next(n) {}
    };

    Node *top;
};
