#pragma once

#include <benchmark/benchmark.h>
#include <message_queue/base_queue.hpp>

#include <chrono>
#include <utility>
#include <vector>
#include <numeric>
#include <algorithm>
#include <map>

inline void SpinWait(std::chrono::microseconds required_delay) {
    if (required_delay.count() <= 0) {
        return;
    }
    const auto wait_start_time = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - wait_start_time < required_delay) {}
}

inline void SetLatencyCounters(benchmark::State& state, std::vector<double>& latencies) {
    if (latencies.empty()) {
        return;
    }

    std::sort(latencies.begin(), latencies.end());
    state.counters["latency_mean"] = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    state.counters["latency_p50"] = latencies[latencies.size() * 0.50];
    state.counters["latency_p95"] = latencies[latencies.size() * 0.95];
    state.counters["latency_p99"] = latencies[latencies.size() * 0.99];
}

inline void SetQueueStatsCounters(benchmark::State& state, BaseQueue* queue) {
    auto stats = queue->GetStats();
    double iterations = static_cast<double>(state.iterations());

    state.counters["queue_pushed"] = stats.push_count / iterations;
    state.counters["queue_popped"] = stats.pop_count / iterations;
    state.counters["queue_dropped"] = stats.dropout_count / iterations;
    state.counters["queue_blocked_ms"] = stats.block_time_ms / iterations;
}

inline void SetFairnessCounter(benchmark::State& state, const std::vector<std::map<int, int>>& all_consumer_counts) {
    std::map<int, int> total_producer_counts;

    for (const auto& consumer_counts : all_consumer_counts) {
        for (const auto& [producer_id, message_count] : consumer_counts) {
            total_producer_counts[producer_id] += message_count;
        }
    }

    if (total_producer_counts.empty()) {
        return;
    }

    int minimum_messages = total_producer_counts.begin()->second;
    int maximum_messages = total_producer_counts.begin()->second;

    for (const auto& [producer_id, message_count] : total_producer_counts) {
        minimum_messages = std::min(minimum_messages, message_count);
        maximum_messages = std::max(maximum_messages, message_count);
    }

    double fairness_ratio = 0.0;
    if (minimum_messages > 0) {
        fairness_ratio = static_cast<double>(maximum_messages) / minimum_messages;
    }

    state.counters["fairness_ratio"] = fairness_ratio;
}
