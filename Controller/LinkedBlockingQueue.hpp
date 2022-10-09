//
//  LinkedBlockingQueue.hpp
//  Controller
//
//  Created by FireWolf on 2/21/22.
//

#ifndef LinkedBlockingQueue_hpp
#define LinkedBlockingQueue_hpp

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>

template <typename Element>
struct LinkedBlockingQueue
{
private:
    /// The queue under protection
    std::queue<Element> queue;

    /// The mutex that protects the queue
    std::mutex mutex;

    /// The condition variable that notifies waiters
    std::condition_variable nonempty;

    //
    // MARK: - Query Properties
    //

public:
    ///
    /// Check whether the queue is empty
    ///
    /// @return `true` if the queue is empty, `false` otherwise.
    /// @note This function is thread-safe.
    ///
    [[nodiscard]]
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lockGuard(this->mutex);

        return this->queue.empty();
    }

    ///
    /// Get the number of elements in the queue
    ///
    /// @return The element count.
    /// @note This function is thread-safe.
    ///
    [[nodiscard]]
    size_t getCount() const
    {
        std::lock_guard<std::mutex> lockGuard(this->mutex);

        return this->queue.size();
    }

    //
    // MARK: - Manage the Queue
    //

public:
    ///
    /// Append the given element to the end of the queue
    ///
    /// @param element The element to be enqueued
    ///
    void offer(Element element)
    {
        std::lock_guard<std::mutex> lockGuard(this->mutex);

        this->queue.push(std::move(element));

        this->nonempty.notify_all();
    }

    ///
    /// Construct an element at the end of the queue
    ///
    /// @param args Arguments to forward to the constructor of `Element`
    ///
    template <typename... Args>
    void emplace(Args&&... args)
    {
        std::lock_guard<std::mutex> lockGuard(this->mutex);

        this->queue.template emplace(std::forward<Args>(args)...);

        this->nonempty.notify_all();
    }

    ///
    /// Remove the head of the queue
    ///
    /// @return The head element.
    ///
    Element poll()
    {
        // Acquire the mutex lock
        std::unique_lock<std::mutex> lock(this->mutex);

        // Wait until the queue is non-empty
        // The mutex lock is released while the caller is blocked.
        // When `wait` returns, the mutex lock is acquired again.
        this->nonempty.template wait(lock, [&]() -> bool
        {
            return !this->queue.empty();
        });

        // Get the queue head while the mutex lock is acquired
        Element element = std::move(this->queue.front());

        this->queue.pop();

        // When this function returns, the mutex lock is released by the destructor of std::unique_lock.
        return element;
    }

    ///
    /// Wait up to the specified amount of time to retrieve the head element and remove it from the queue
    ///
    /// @param timeout The amount of time to wait until the queue is non-empty
    /// @return The head element on success, `std::nullopt` on timed out.
    ///
    template <typename Representation, typename Period>
    std::optional<Element> pollWithTimeout(const std::chrono::duration<Representation, Period>& timeout)
    {
        std::unique_lock<std::mutex> lock(this->mutex);

        auto predicate = [&]() -> bool { return !this->queue.empty(); };

        if (this->nonempty.template wait_for(lock, timeout, predicate))
        {
            Element element = std::move(this->queue.front());

            this->queue.pop();

            return std::make_optional(std::move(element));
        }
        else
        {
            return std::nullopt;
        }
    }
};

#endif /* LinkedBlockingQueue_hpp */
