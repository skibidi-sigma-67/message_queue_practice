#pragma once

#include <benchmark/benchmark.h>

#include "utils.hpp"
#include "workers.hpp"

#include <vector>
#include <thread>
#include <atomic>
#include <map>

template <bool UseExponentialProducerDelay = false,
          bool UseRandomProducerPriority   = false,
          bool UseExponentialConsumerDelay = false>
inline void RunContinuousScenario(
    benchmark::State& state,
    BaseQueue* queue,
    int producer_count,
    int consumer_count,
    std::chrono::microseconds producer_delay,
    std::chrono::microseconds consumer_delay,
    std::chrono::milliseconds run_duration = std::chrono::milliseconds(50)
) {
    std::vector<double> global_latencies;
    std::vector<std::map<int, int>> global_consumer_counts(consumer_count);

    for (auto _ : state) {
        std::atomic<bool> start_signal{false};
        std::atomic<bool> producer_stop_signal{false};
        std::atomic<bool> consumer_stop_signal{false};

        std::vector<std::vector<double>> producer_latencies(producer_count);
        std::vector<std::vector<double>> consumer_latencies(consumer_count);
        std::vector<std::map<int, int>> consumer_counts(consumer_count);

        std::vector<std::thread> producer_threads;
        std::vector<std::thread> consumer_threads;

        for (int i = 0; i < producer_count; ++i) {
            producer_threads.emplace_back(
                Producer<UseExponentialProducerDelay, UseRandomProducerPriority>,
                queue,
                i,
                producer_delay,
                std::ref(start_signal),
                std::ref(producer_stop_signal),
                std::ref(producer_latencies[i])
            );
        }

        for (int i = 0; i < consumer_count; ++i) {
            consumer_threads.emplace_back(
                Consumer<UseExponentialConsumerDelay>,
                queue,
                i + producer_count,
                consumer_delay,
                std::ref(start_signal),
                std::ref(consumer_stop_signal),
                std::ref(consumer_latencies[i]),
                std::ref(consumer_counts[i])
            );
        }

        start_signal.store(true, std::memory_order_release);
        std::this_thread::sleep_for(run_duration);

        producer_stop_signal.store(true, std::memory_order_release);
        for (auto& thread : producer_threads) {
            thread.join();
        }

        consumer_stop_signal.store(true, std::memory_order_release);
        for (auto& thread : consumer_threads) {
            thread.join();
        }

        for (const auto& local_latencies : producer_latencies) {
            global_latencies.insert(global_latencies.end(), local_latencies.begin(), local_latencies.end());
        }
        for (const auto& local_latencies : consumer_latencies) {
            global_latencies.insert(global_latencies.end(), local_latencies.begin(), local_latencies.end());
        }
        for (int i = 0; i < consumer_count; ++i) {
            for (const auto& [producer_id, count] : consumer_counts[i]) {
                global_consumer_counts[i][producer_id] += count;
            }
        }
    }

    SetLatencyCounters(state, global_latencies);
    SetFairnessCounter(state, global_consumer_counts);
    SetQueueStatsCounters(state, queue);
    state.SetItemsProcessed(global_latencies.size() / 2);
}
