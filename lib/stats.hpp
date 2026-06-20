#pragma once

#include <chrono>
#include <cstddef>

struct QueueStats {
    size_t push_count;
    size_t pop_count;
    size_t dropout_count;
    size_t failed_count;
    size_t current_size;
    std::chrono::steady_clock::duration block_time;
};


