#include <queue.hpp>

#include <base_message.hpp>
#include <base_queue.hpp>
#include <stats.hpp>

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <utility>

size_t CircularWriterBlockQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

QueueStats CircularWriterBlockQueue::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    QueueStats current_stats = stats_;
    current_stats.current_size = size_;
    return current_stats;
}

PushStatus CircularWriterBlockQueue::Push(Message message) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_not_full_.wait(lock, [this]() {
        return size_ < capacity_ || is_closed_;
    });

    if (is_closed_) {
        ++stats_.failed_count;
        return PushStatus::kError;
    }

    buffer_[tail_] = std::move(message);
    tail_ = (tail_ + 1) % capacity_;
    ++size_;
    ++stats_.push_count;

    cv_not_empty_.notify_one();
    return PushStatus::kPushed;
}

std::optional<Message> CircularWriterBlockQueue::TryPop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (size_ > 0) {
        Message message = std::move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --size_;
        ++stats_.pop_count;

        cv_not_full_.notify_one();
        return message;
    }
    return std::nullopt;
}

std::optional<Message> CircularWriterBlockQueue::WaitPop() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_not_empty_.wait(lock, [this]() {
        return size_ > 0 || is_closed_;
    });

    if (size_ == 0 && is_closed_) {
        return std::nullopt;
    }

    Message message = std::move(buffer_[head_]);
    head_ = (head_ + 1) % capacity_;
    --size_;
    ++stats_.pop_count;

    cv_not_full_.notify_one();
    return message;
}

void CircularWriterBlockQueue::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_closed_ = true;

    cv_not_empty_.notify_all();
    cv_not_full_.notify_all();
}
