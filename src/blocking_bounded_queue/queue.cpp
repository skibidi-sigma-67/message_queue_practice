#include <queue.hpp>

#include <mutex>
#include <condition_variable>
#include <optional>

size_t BlockingBoundedQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

QueueStats BlockingBoundedQueue::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    QueueStats current_stats = stats_;
    current_stats.current_size = queue_.size();
    return current_stats;
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
    std::lock_guard<std::mutex> lock(mutex_);
    is_closed_ = true;

    cv_not_empty_.notify_all();
    cv_not_full_.notify_all();
}
