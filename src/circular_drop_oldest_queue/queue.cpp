#include <queue.hpp>

#include <mutex>

size_t CircularDropOldestQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
}

QueueStats CircularDropOldestQueue::GetStats() const override {
    std::lock_guard<std::mutex> lock(mutex_);
    QueueStats current_stats = stats_;
    current_stats.current_size = capacity_;
    return current_stats;
}