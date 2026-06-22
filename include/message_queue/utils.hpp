#pragma once

#include <chrono>
#include <utility>
#include <type_traits>

template<typename Func, typename... Args>
auto MeasureExecutionTime(Func&& func, Args&&... args) {
    using ReturnType = std::invoke_result_t<Func, Args...>;

    const auto start_time = std::chrono::high_resolution_clock::now();

    if constexpr (std::is_same_v<ReturnType, void>) {
        std::forward<Func>(func)(std::forward<Args>(args)...);
        const auto end_time = std::chrono::high_resolution_clock::now();

        return std::chrono::duration<double, std::nano>(end_time - start_time).count();
    } else {
        ReturnType result = std::forward<Func>(func)(std::forward<Args>(args)...);
        const auto end_time = std::chrono::high_resolution_clock::now();
        double latency = std::chrono::duration<double, std::nano>(end_time - start_time).count();

        return std::make_pair(std::move(result), latency);
    }
}
