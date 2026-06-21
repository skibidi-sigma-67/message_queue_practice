#pragma once

#include <message_queue/base_message.hpp>
#include <message_queue/stats.hpp>

#include <cstddef>
#include <optional>

enum struct PushStatus {
    kPushed,
    kDropped,
    kBlocked,
    kError
};

class BaseQueue {
private:
    QueueStats stats_;
public:
    virtual ~BaseQueue() = default;

    virtual size_t Size() const = 0;

    virtual QueueStats GetStats() const = 0;

    virtual PushStatus Push(Message message) = 0;

    virtual std::optional<Message> TryPop() = 0;

    virtual std::optional<Message> WaitPop() = 0;

    virtual void Close() = 0;
};
