#pragma once

#include <base_queue.hpp>

#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>

class BlockingBoundedQueue : public BaseQueue {
private:
    std::deque<Message> queue_;
    size_t capacity_;
    bool is_closed_ = false;

    mutable std::mutex mutex_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;

    QueueStats stats_{0, 0, 0, 0, 0};
public:
    explicit BlockingBoundedQueue(size_t capacity)
        : capacity_(capacity) {}

    ~BlockingBoundedQueue() override = default;

    size_t Size() const override;

    QueueStats GetStats() const override;

    PushStatus Push(Message message) override;

    std::optional<Message> TryPop() override;

    std::optional<Message> WaitPop() override;

    void Close() override;
};
