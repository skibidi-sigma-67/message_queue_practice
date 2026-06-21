#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cstddef>

#include <message_queue/base_queue.hpp>
#include <message_queue/stats.hpp>
#include <message_queue/base_message.hpp>

class CircularDropOldestQueue : public BaseQueue {
private:
    std::vector<Message> buffer_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t size_ = 0;
    size_t capacity_;
    bool is_closed_ = false;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    QueueStats stats_{0, 0, 0, 0, 0};

public:
    explicit CircularDropOldestQueue(size_t capacity) : buffer_(capacity), capacity_(capacity) {};

    ~CircularDropOldestQueue() override = default;

    size_t Size() const override;

    QueueStats GetStats() const override;

    PushStatus Push(Message message) override;

    std::optional<Message> TryPop() override;

    std::optional<Message> WaitPop() override;

    void Close() override;
    
};