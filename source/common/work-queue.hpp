
/***************************************************
A thread safe queue that can be used to store work 
that different threads can un-queue concurrently.
***************************************************/

#pragma once

#include <queue>
#include <mutex>
#include <vector>

template <class T>
class WorkQueue
{
public:
    WorkQueue(const std::vector<T> items) : m()
    {
        original_size = items.size();
        for (const auto& item : items)
        {
            queue.push(item);
        }
    }

    bool getWork(T& item)
    {
        std::unique_lock<std::mutex> lock(m);

        bool empty = queue.empty();
        if (!empty)
        {
            item = queue.front();
            queue.pop();
        }
        lock.unlock();

        return !empty;
    }

    bool empty()
    {
        std::unique_lock<std::mutex> lock(m);
        bool empty = queue.empty();
        lock.unlock();
        return empty;
    }

    double progress()
    {
        std::unique_lock<std::mutex> lock(m);
        double p = static_cast<double>(original_size - queue.size()) * 100.0 / original_size;
        lock.unlock();
        return p;
    }

private:
    size_t original_size;
    std::queue<T> queue;
    mutable std::mutex m;
};