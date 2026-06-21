#include <queue.hpp>
#include <algorithm>
#include <utility>
#include <functional>

bool PriorityQueue::PriorityNode::operator>(const PriorityNode& other) const {
    if (this->message.priority != other.message.priority) {
        return this->message.priority > other.message.priority; 
    }
    return this->seq_id > other.seq_id;
}

size_t PriorityQueue::Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return heap_.size();
}

QueueStats PriorityQueue::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

PushStatus PriorityQueue::Push(Message message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_closed_) {
        return PushStatus::kError; 
    }

    heap_.push_back({std::move(message), next_seq_id_++});

    std::push_heap(heap_.begin(), heap_.end(), std::greater<PriorityNode>());

    stats_.push_count++;
    stats_.current_size = heap_.size();

    not_empty_.notify_one();
    
    return PushStatus::kPushed;
}

std::optional<Message> PriorityQueue::TryPop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (heap_.empty()) {
        return std::nullopt;
    }

    std::pop_heap(heap_.begin(), heap_.end(), std::greater<PriorityNode>());

    PriorityNode node = std::move(heap_.back());
    heap_.pop_back();

    stats_.pop_count++;
    stats_.current_size = heap_.size();

    return node.message;
}

std::optional<Message> PriorityQueue::WaitPop() {
    std::unique_lock<std::mutex> lock(mutex_);

    not_empty_.wait(lock, [this]() { 
        return !heap_.empty() || is_closed_; 
    });

    if (heap_.empty() && is_closed_) {
        return std::nullopt;
    }

    std::pop_heap(heap_.begin(), heap_.end(), std::greater<PriorityNode>());
    PriorityNode node = std::move(heap_.back());
    heap_.pop_back();

    stats_.pop_count++;
    stats_.current_size = heap_.size();

    return node.message;
}

void PriorityQueue::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_closed_ = true;
    not_empty_.notify_all();
}