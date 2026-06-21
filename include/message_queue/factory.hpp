#pragma once

#include <message_queue/base_queue.hpp>

#include <memory>
#include <cstddef>

enum class QueuePolicy {
    kBlockingBounded,
    kCircularDropOldest,
    kCircularWriterBlock,
    kPriority,
    kLockFree
};

std::unique_ptr<BaseQueue> CreateQueue(QueuePolicy policy, size_t capacity);
