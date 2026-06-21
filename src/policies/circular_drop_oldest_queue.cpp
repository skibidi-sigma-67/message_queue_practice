#include <message_queue/policies/circular_drop_oldest_queue.hpp>
#include <message_queue/base_queue.hpp>
#include <message_queue/base_message.hpp>
#include <message_queue/stats.hpp>

#include <utility>
#include <cstddef>
#include <mutex>
#include <optional>

size_t CircularDropOldestQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return size_;
}

QueueStats CircularDropOldestQueue::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    QueueStats current_stats = stats_;
    current_stats.current_size = size_;

    return current_stats;
}

PushStatus CircularDropOldestQueue::Push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_closed_) {
        ++stats_.failed_count;
        return PushStatus::kError; 
    }

    PushStatus status = PushStatus::kPushed;

    if (size_ == capacity_) {
        head_ = (head_ + 1) % capacity_;
        ++stats_.dropout_count;
        status = PushStatus::kDropped;
    } 
    else {
        ++size_;
    }

    buffer_[tail_] = std::move(message);
    tail_ = (tail_ + 1) % capacity_;
    
    ++stats_.push_count;
    cv_.notify_one(); 

    return status;
}

std::optional<Message> CircularDropOldestQueue::TryPop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (size_ == 0) {
        return std::nullopt;
    }

    Message msg = std::move(buffer_[head_]);
    head_ = (head_ + 1) % capacity_;
    --size_;
    ++stats_.pop_count;

    return msg;
}

std::optional<Message> CircularDropOldestQueue::WaitPop() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    cv_.wait(lock, [this]() { 
        return size_ > 0 || is_closed_; 
    });

    if (size_ == 0 && is_closed_) {
        return std::nullopt;
    }

    Message msg = std::move(buffer_[head_]);
    head_ = (head_ + 1) % capacity_;
    --size_;
    ++stats_.pop_count;

    return msg;
}

void CircularDropOldestQueue::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    is_closed_ = true;
    cv_.notify_all(); 
}

