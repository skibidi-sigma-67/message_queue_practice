#include <queue.hpp>

#include <queue.hpp>
#include <base_message.hpp>
#include <stats.hpp>

#include <utility>
#include <cstddef>
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
    std::unique_lock<std::mutex> lock(mutex_);
    cv_not_full_.wait(lock, [this]() {
        return queue_.size() < capacity_ || is_closed_;
    });

    if (is_closed_) {
        ++stats_.failed_count;
        return PushStatus::kError;
    }

    queue_.push_back(std::move(message));
    ++stats_.push_count;

    cv_not_empty_.notify_one();
    return PushStatus::kPushed;
}

std::optional<Message> BlockingBoundedQueue::TryPop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!queue_.empty()) {
        Message message = std::move(queue_.front());
        queue_.pop_front();
        ++stats_.pop_count;

        cv_not_full_.notify_one();
        return message;
    }
    return std::nullopt;
}

std::optional<Message> BlockingBoundedQueue::WaitPop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_not_empty_.wait(lock, [this]() {
        return !queue_.empty() || is_closed_;
    });

    if (queue_.empty() && is_closed_) {
        return std::nullopt;
    }

    Message message = std::move(queue_.front());
    queue_.pop_front();
    ++stats_.pop_count;

    cv_not_full_.notify_one();
    return message;
}

void BlockingBoundedQueue::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_closed_ = true;

    cv_not_empty_.notify_all();
    cv_not_full_.notify_all();
}
