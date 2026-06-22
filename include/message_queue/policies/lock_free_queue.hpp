#pragma once

#include <message_queue/base_queue.hpp>
#include <message_queue/base_message.hpp>
#include <message_queue/stats.hpp>

#include <atomic>
#include <cstddef>
#include <optional>


class LockFreeQueue : public BaseQueue {
public:
    LockFreeQueue();
    ~LockFreeQueue() override;

    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;

    size_t Size() const override;

    QueueStats GetStats() const override;

    PushStatus Push(Message message) override;

    std::optional<Message> TryPop() override;

    std::optional<Message> WaitPop() override;

    void Close() override;

private:
    struct Node;

    void RetireNode(Node* node) noexcept;
    void DeleteAllNodes() noexcept;
    void ScanRetiredNodes() noexcept;
    void PushRetiredNode(Node* node) noexcept;
    static bool IsNodeHazard(Node* node) noexcept;

    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};

    std::atomic<bool> is_closed_{false};
    std::atomic<size_t> size_{0};

    std::atomic<size_t> push_count_{0};
    std::atomic<size_t> pop_count_{0};
    std::atomic<size_t> dropout_count_{0};
    std::atomic<size_t> failed_count_{0};
    std::atomic<double> block_time_ns_{0.0};

    std::atomic<size_t> wake_sequence_{0};

    std::atomic<Node*> retired_head_{nullptr};
};

