#pragma once

#include <vector>

/**********************************************************************************
 Priority queue that is faster than std::priority_queue and has some more features.
 Based on the d-ary heap from: https://github.com/skarupke/heap
 I also tried the min-max heap from ^, but it was slightly slower for my use.
***********************************************************************************/

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
            if (H[parent] < value)
            {
                H[index] = H[parent];
                index = parent;
            }
            else
            {
                break;
            }
        }
        H[index] = value;
    }

    void pop()
    {
        if (H.size() <= 1)
        {
            H.pop_back();
            return;
        }
        T value = H.back();
        H.pop_back();
        pop_push(value);
    }

    // Replaces the top value with the input value 
    // and shifts it down to the correct position.
    void pop_push(const T& value)
    {
        uint_t index = 0;
        while (true)
        {
            uint_t right = rightIndex(index);
            uint_t left = right - 1;
            if (right < H.size())
            {
                uint_t max_child = left + (H[left] < H[right]);
                if (value < H[max_child])
                {
                    H[index] = H[max_child];
                    index = max_child;
                }
                else
                {
                    break;
                }
            }
            else
            {
                if (left < H.size() && value < H[left])
                {
                    H[index] = H[left];
                    index = left;
                }
                break;
            }
        }
        H[index] = value;
    }

    const T& top() const { return H.front(); }
    bool empty() const { return H.empty(); }
    uint_t size() const { return H.size(); }
    void clear() { H.clear(); }
    const std::vector<T>& container() const { return H; }

private:
    static constexpr uint_t parentIndex(uint_t i) { return (i - 1) / 2; }
    static constexpr uint_t leftIndex(uint_t i) { return 2 * i + 1; }
    static constexpr uint_t rightIndex(uint_t i) { return 2 * i + 2; }

    std::vector<T> H;
};

/*******************************************************************
 std::priority_queue than can be used for comparison. Photon mapping 
 is roughly 7% faster and path tracing 2% faster with the new queue.
********************************************************************/
#include <queue>
template<class T>
struct AccessiblePQ : public std::priority_queue<T, std::vector<T>, std::less<T>>
{
    void pop_push(const T& value)
    {
        this->pop();
        this->push(value);
    }
    void clear() { this->c.clear(); }
    const std::vector<T>& container() const { return this->c; }
};