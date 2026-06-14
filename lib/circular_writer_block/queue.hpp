#pragma once

#include <base_message.hpp>
#include <base_queue.hpp>
#include <stats.hpp>

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <vector>

namespace circular_writer_block {

class Queue : public BaseQueue {
public:
    explicit Queue(std::size_t capacity);
    ~Queue() override = default;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    size_t Size() const override;

    QueueStats GetStats() const override;

    PushStatus Push(Message message) override;

    std::optional<Message> TryPop() override;

    std::optional<Message> WaitPop() override;

    void Close() override;

private:
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;

    std::vector<Message> buffer_;
    std::size_t capacity_;
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
    std::size_t size_ = 0;
    bool closed_ = false;

    QueueStats stats_{};
    std::size_t push_blocked_count_ = 0;
};

}  // namespace circular_writer_block
