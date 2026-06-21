#pragma once

#include <message_queue/base_queue.hpp>

#include <memory>
#include <cstddef>

enum class QueuePolicy : size_t {
    kBlockingBounded     = 0,
    kCircularDropOldest  = 1,
    kCircularWriterBlock = 2,
    kPriority            = 3,
    kLockFree            = 4,
};

std::unique_ptr<BaseQueue> CreateQueue(QueuePolicy policy, size_t capacity);
