#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cstdint>
#include <cstddef>
#include <message_queue/base_queue.hpp>
#include <message_queue/base_message.hpp>
#include <message_queue/stats.hpp>

class PriorityQueue : public BaseQueue {
private:
    struct PriorityNode {
        Message message;
        uint64_t seq_id;

        bool operator>(const PriorityNode& other) const;
    };

    std::vector<PriorityNode> heap_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    
    uint64_t next_seq_id_ = 0;
    bool is_closed_ = false;
    QueueStats stats_{};

public:
    PriorityQueue() = default;
    ~PriorityQueue() override = default;

    size_t Size() const override;

    QueueStats GetStats() const override;

    PushStatus Push(Message message) override;

    std::optional<Message> TryPop() override;

    std::optional<Message> WaitPop() override;

    void Close() override;
};