#include "base_message.hpp"
#include "stats.hpp"

#include <cstdint>
#include <optional>

class BaseQueue {
public:
    virtual ~BaseQueue() = default;

    virtual size_t Size() const = 0;

    virtual QueueStats GetStats() const = 0;

    virtual PushStatus Push(Message message) = 0;

    virtual std::optional<Message> TryPop() = 0;

    virtual std::optional<Message> WaitPop() = 0;

    virtual void Close() = 0;
};
