// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue<T>&) = delete;

    ThreadSafeQueue(ThreadSafeQueue<T>&& other)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue = std::move(other._queue);
    }

    virtual ~ThreadSafeQueue() { }

    unsigned long size() const
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.size();
    }

    std::optional<T> pop()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            return {};
        }
        T tmp = _queue.front();
        _queue.pop();
        return tmp;
    }

    void push(const T& item)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(item);
    }

    T front()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _queue.front();
    }

private:
    // Moved out of public interface to prevent races between this
    // and pop().
    bool empty() const
    {
        return _queue.empty();
    }

    std::queue<T> _queue;
    mutable std::mutex _mutex;
};
