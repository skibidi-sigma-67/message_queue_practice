#pragma once
#include "../../lib/base_queue.hpp"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <optional>


class PriorityQueue : public BaseQueue {
private:
    struct PriorityNode {
        Message message;
        uint64_t seq_id;

        bool operator>(const PriorityNode& other) const {
            return false;
        }
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

    size_t Size() const override {
        return 0;
    }

    QueueStats GetStats() const override {
        return {};
    }

    PushStatus Push(Message message) override {
        return PushStatus::kPushed;
    }

    std::optional<Message> TryPop() override {
        return std::nullopt;
    }

    std::optional<Message> WaitPop() override {
        return std::nullopt;
    }

    void Close() override {
    }
};