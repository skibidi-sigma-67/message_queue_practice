#include <message_queue/factory.hpp>

#include <message_queue/policies/blocking_bounded_queue.hpp>
#include <message_queue/policies/circular_drop_oldest_queue.hpp>
#include <message_queue/policies/circular_writer_block.hpp>
#include <message_queue/policies/priority_queue.hpp>
#include <message_queue/policies/lock_free_queue.hpp>

#include <stdexcept>
#include <memory>

std::unique_ptr<BaseQueue> CreateQueue(QueuePolicy policy, size_t capacity) {
    switch (policy) {
        case QueuePolicy::kBlockingBounded:
            return std::make_unique<BlockingBoundedQueue>(capacity);
        case QueuePolicy::kCircularDropOldest:
            return std::make_unique<CircularDropOldestQueue>(capacity);
        case QueuePolicy::kCircularWriterBlock:
            return std::make_unique<CircularWriterBlockQueue>(capacity);
        case QueuePolicy::kPriority:
            return std::make_unique<PriorityQueue>();
        case QueuePolicy::kLockFree:
            return std::make_unique<LockFreeQueue>();
        default:
            throw std::invalid_argument("Unknown queue policy");
    }
}
