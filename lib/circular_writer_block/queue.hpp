#pragma once

#include <base_queue.hpp>

#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>

class CircularWriterBlockQueue : public BaseQueue {
private:
    std::vector<Message> buffer_;
    size_t capacity_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t size_ = 0;
    bool is_closed_ = false;

    mutable std::mutex mutex_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;

    QueueStats stats_{0, 0, 0, 0, 0};
public:
    explicit CircularWriterBlockQueue(size_t capacity)
        : buffer_(capacity), capacity_(capacity) {}

    ~CircularWriterBlockQueue() override = default;

    size_t Size() const override;

    QueueStats GetStats() const override;

    PushStatus Push(Message message) override;

    std::optional<Message> TryPop() override;

    std::optional<Message> WaitPop() override;

    void Close() override;
};
