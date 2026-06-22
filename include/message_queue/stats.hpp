#pragma once

#include <cstddef>

struct QueueStats {
    size_t push_count = 0;
    size_t pop_count = 0;
    size_t dropout_count = 0;
    size_t failed_count = 0;
    size_t current_size = 0;
    double block_time_ns = 0.0;
};
