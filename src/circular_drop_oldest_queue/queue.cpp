#include <queue.hpp>

#include <mutex>

size_t CircularDropOldestQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);

    return size_;
}

QueueStats CircularDropOldestQueue::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    QueueStats current_stats = stats_;
    current_stats.current_size = capacity_;

    return current_stats;
}

PushStatus CircularDropOldestQueue::Push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_closed_) {
        stats_.failed_count++;
        return PushStatus::kError; 
    }

    PushStatus status = PushStatus::kPushed;

    if (size_ == capacity_) {
        head_ = (head_ + 1) % capacity_;
        stats_.dropout_count++;
        status = PushStatus::kDropped;
    } 
    else {
        size_++;
    }

    buffer_[tail_] = std::move(message);
    tail_ = (tail_ + 1) % capacity_;
    
    stats_.push_count++;
    cv_.notify_one(); 

    return status;
}

