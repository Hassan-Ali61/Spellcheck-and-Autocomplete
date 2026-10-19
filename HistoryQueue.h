#pragma once

#include <iostream>
#include <string>

constexpr int HISTORY_CAPACITY = 5;

// Remembers the last HISTORY_CAPACITY words (circular queue on an array).
class HistoryQueue
{
public:
    HistoryQueue() : front(0), rear(0), count(0) {}

    bool isEmpty() const { return count == 0; }
    bool isFull() const { return count == HISTORY_CAPACITY; }

    // Adds a word. When the queue is full, the oldest word is dropped first.
    void enqueue(const std::string &word)
    {
        if (isFull())
        {
            front = (front + 1) % HISTORY_CAPACITY;
            count--;
        }
        items[rear] = word;
        rear = (rear + 1) % HISTORY_CAPACITY;
        count++;
    }

    // Prints the words from oldest to newest, like " | cat | dog |". Prints nothing if empty.
    void display() const
    {
        if (isEmpty())
            return;

        for (int i = 0; i < count; i++)
            std::cout << " | " << items[(front + i) % HISTORY_CAPACITY];
        std::cout << " |\n";
    }

private:
    int front; // index of the oldest word
    int rear;  // index where the next word goes
    int count;
    std::string items[HISTORY_CAPACITY];
};
