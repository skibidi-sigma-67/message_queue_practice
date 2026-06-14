#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>

#include "../base_queue.hpp"

class CircularDropOldestQueue : public BaseQueue {
private:
    std::vector<Message> buffer_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t current_size_ = 0;
    size_t max_size_;
    bool is_closed_ = false;

    // mutable std::mutex mutex_;
    // std::condition_variable cv_;
    QueueStats stats_{0, 0, 0, 0, 0};

public:
    explicit CircularDropOldestQueue(size_t max_size) : max_size_(max_size) {
        buffer_.resize(max_size_);
    }

    ~CircularDropOldestQueue() override = default;

    size_t Size() const override {
        // TODO
    }

    QueueStats GetStats() const override {
        // TODO        
    }

    PushStatus Push(Message message) override {
        // TODO
    }

    std::optional<Message> TryPop() override {
        // TODO
    }

    std::optional<Message> WaitPop() override {
        // TODO
    }

    void Close() override {
        // TODO
    }
};