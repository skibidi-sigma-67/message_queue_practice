#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <cstdint>

struct Message {
    uint64_t id = 0;
    int producer_id = 0;
    int priority = 0;
    std::string payload;
    std::chrono::steady_clock::time_point created_at;
};
