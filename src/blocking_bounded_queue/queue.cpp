#include <queue.hpp>

#include <optional>

size_t BlockingBoundedQueue::Size() const {
    // TODO
    return 0;
}

QueueStats BlockingBoundedQueue::GetStats() const {
    // TODO
    return {};
}

PushStatus BlockingBoundedQueue::Push(Message message) {
    // TODO
    return {};
}

std::optional<Message> BlockingBoundedQueue::TryPop() {
    // TODO
    return std::nullopt;
}

std::optional<Message> BlockingBoundedQueue::WaitPop() {
    // TODO
    return std::nullopt;
}

void BlockingBoundedQueue::Close() {
    // TODO
}
