#pragma once

#include <vector>
#include <utility>

/***************************************************************************
 Faster priority queue than std::priority_queue and std::[operation]_heap.
 Based on the d-ary heap from: https://github.com/skarupke/heap (for d = 2)
 I also tried the min-max heap from ^, but it was slower for my use.
****************************************************************************/
template<class T>
class PriorityQueue
{
    typedef std::vector<T>::size_type uint_t;

public:
    PriorityQueue(uint_t reserved = 64) { H.reserve(reserved); }

    void push(const T& value)
    {
        H.push_back(value);
        uint_t index = H.size() - 1;
        while (index > 0)
        {
            uint_t parent = parentIndex(index);
            if (!(H[parent] < value)) break;
            H[index] = H[parent];
            index = parent;
        }
        H[index] = value;
    }

    void pop()
    {
        if (H.size() > 1)
        {
            T value = H.back();
            H.pop_back();
            shiftDown(value, 0);
        }
        else
        {
            H.pop_back();
        }
    }

    void pop_push(const T& value)
    {
        shiftDown(value, 0);
    }

    void push_unordered(const T& value)
    {
        H.push_back(value);
    }

    void make_heap()
    {
        if (H.size() <= 1) return;

        const uint_t last_index = H.size() - 1;
        uint_t index = parentIndex(last_index);

        if (last_index % 2)
        {
            uint_t left = leftIndex(index);
            if (H[index] < H[left]) std::swap(H[index], H[left]);
            if (index == 0) return;
            index--;
        }

        if (index)
        {
            uint_t lowest_index_with_no_grandchildren = grandparentIndex(last_index) + 1;
            do
            {
                uint_t left = leftIndex(index);
                uint_t max_child = left + (H[left] < H[left + 1]);
                if (H[index] < H[max_child]) std::swap(H[index], H[max_child]);
            } while (index-- != lowest_index_with_no_grandchildren);
        }

        do { shiftDown(T(H[index]), index); } while (index--);
    }

    const T& top() const { return H.front(); }
    bool empty() const { return H.empty(); }
    uint_t size() const { return H.size(); }
    void clear() { H.clear(); }
    const std::vector<T>& container() const { return H; }

private:
    uint_t parentIndex(uint_t i) const { return (i - 1) / 2; }
    uint_t leftIndex(uint_t i) const { return 2 * i + 1; }
    uint_t rightIndex(uint_t i) const { return 2 * i + 2; }
    uint_t grandparentIndex(uint_t i) const { return (i - 3) / 4; }

    // Effectively replaces the value at index with the input value,
    // and shifts it down to the correct position in the sub-tree.
    void shiftDown(const T& value, uint_t index)
    {
        while (true)
        {
            uint_t left = leftIndex(index);
            uint_t right = left + 1;

            uint_t max_child;
            if (right < H.size())
                max_child = left + (H[left] < H[right]);
            else if (left < H.size())
                max_child = left;
            else
                break;

            if (!(value < H[max_child])) break;
            H[index] = H[max_child];
            index = max_child;
        }
        H[index] = value;
    }

    std::vector<T> H;
};

/*************************************************************************
 Priority queue using std::[operation]_heap for comparison. Photon mapping 
 is roughly 7% faster and path tracing 4% faster with the new queue.
**************************************************************************/
#include <algorithm>
template<class T>
class PriorityQueue1
{
    typedef std::vector<T>::size_type uint_t;
public:
    PriorityQueue1(uint_t reserved = 64) { H.reserve(reserved); }

    void push(const T& value) { H.push_back(value); std::push_heap(H.begin(), H.end()); }
    void pop() { std::pop_heap(H.begin(), H.end()); H.pop_back(); }
    void push_unordered(const T& value) { H.push_back(value); }
    void make_heap() { std::make_heap(H.begin(), H.end()); }

    // The only way to achieve optimized pop and push with std::[operation]_heap.
    // The overhead compared to the new queue is the push_back, swap and pop_back,
    // which should basically be free if the container has enough reserved space.
    void pop_push(const T& value) 
    {
        // Adds new value last.
        H.push_back(value); 
        // Swaps last (new) and first (max) value, and moves the new value down to correct position.
        std::pop_heap(H.begin(), H.end());
        // Removes max value.
        H.pop_back();
    }

    const T& top() const { return H.front(); }
    bool empty() const { return H.empty(); }
    uint_t size() const { return H.size(); }
    void clear() { H.clear(); }
    const std::vector<T>& container() const { return H; }

private:
    std::vector<T> H;
};