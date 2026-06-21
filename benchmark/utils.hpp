#pragma once

#include <chrono>
#include <utility>
#include <type_traits>

inline void SpinWait(std::chrono::microseconds required_delay) {
    if (required_delay.count() <= 0) {
        return;
    }
    const auto wait_start_time = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - wait_start_time < required_delay) {}
}

template<typename Func, typename... Args>
auto MeasureExecutionTime(Func&& func, Args&&... args) {
    using ReturnType = std::invoke_result_t<Func, Args...>;

    const auto start_time = std::chrono::high_resolution_clock::now();

    if constexpr (std::is_same_v<ReturnType, void>) {
        std::forward<Func>(func)(std::forward<Args>(args)...);
        const auto end_time = std::chrono::high_resolution_clock::now();

        return std::chrono::duration<double, std::micro>(end_time - start_time).count();
    } else {
        ReturnType result = std::forward<Func>(func)(std::forward<Args>(args)...);
        const auto end_time = std::chrono::high_resolution_clock::now();
        double latency = std::chrono::duration<double, std::micro>(end_time - start_time).count();

        return std::make_pair(std::move(result), latency);
    }
}