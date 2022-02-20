#pragma once

#include <utility> // std::swap

// Statically allocated priority queue. Breaks if inserted elements exceeds capacity.
// Based on: https://www.geeksforgeeks.org/priority-queue-using-binary-heap/
template<class T, int CAPACITY>
class ConstantPriorityQueue
{
public:
    ConstantPriorityQueue() { }

    bool insert(const T& data)
    {
        H[++size] = data;
        shiftUp(size);
        return true;
    }

    void pop()
    {
        H[0] = H[size--];
        shiftDown(0);
    }

    const T& top() const
    {
        return H[0];
    }

    bool empty() const
    {
        return size == -1;
    }

private:
    int parent(int i) { return (i - 1) / 2; }
    int left(int i) { return 2 * i + 1; }
    int right(int i) { return 2 * i + 2; }

    void shiftUp(int i)
    {
        while (i > 0 && H[parent(i)] < H[i]) 
        {
            std::swap(H[parent(i)], H[i]);
            i = parent(i);
        }
    }

    void shiftDown(int i)
    {
        int max_i = i;

        int l = left(i);
        if (l <= size && H[max_i] < H[l])
        {
            max_i = l;
        }

        int r = right(i);
        if (r <= size && H[max_i] < H[r])
        {
            max_i = r;
        }

        if (i != max_i) 
        {
            std::swap(H[i], H[max_i]);
            shiftDown(max_i);
        }
    }

    int size = -1;
    T H[CAPACITY];
};