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
        original_size_ = items.size();
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

    double progress()
    {
        std::unique_lock<std::mutex> lock(m);
        double p = static_cast<double>(original_size_ - queue.size()) * 100.0 / original_size_;
        lock.unlock();
        return p;
    }

private:
    size_t original_size_;
    std::queue<T> queue;
    mutable std::mutex m;
};