#pragma once

#include "utils.hpp"
#include "workers.hpp"

#include <benchmark/benchmark.h>
#include <vector>
#include <thread>
#include <atomic>
#include <map>

template <bool ExpDelayProd = false, bool RandPrioProd = false, bool ExpDelayCons = false>
inline void RunBenchmarkScenario(
    benchmark::State& state,
    BaseQueue* queue,
    int num_producers,
    int num_consumers,
    std::chrono::microseconds prod_delay,
    std::chrono::microseconds cons_delay,
    std::chrono::milliseconds run_duration = std::chrono::milliseconds(50)
) {
    std::vector<double> global_latencies;
    std::vector<std::map<int, int>> global_consumer_counts(num_consumers);

    for (auto _ : state) {
        std::atomic<bool> start_signal{false};
        std::atomic<bool> producer_stop_signal{false};
        std::atomic<bool> consumer_stop_signal{false};

        std::vector<std::vector<double>> producer_latencies(num_producers);
        std::vector<std::vector<double>> consumer_latencies(num_consumers);
        std::vector<std::map<int, int>> consumer_counts(num_consumers);

        std::vector<std::thread> producer_threads;
        std::vector<std::thread> consumer_threads;

        for (int i = 0; i < num_producers; ++i) {
            producer_threads.emplace_back(Producer<ExpDelayProd, RandPrioProd>, queue, i, prod_delay, 
                std::ref(start_signal), std::ref(producer_stop_signal), std::ref(producer_latencies[i]));
        }

        for (int i = 0; i < num_consumers; ++i) {
            consumer_threads.emplace_back(Consumer<ExpDelayCons>, queue, i + num_producers, cons_delay, 
                std::ref(start_signal), std::ref(consumer_stop_signal), std::ref(consumer_latencies[i]), std::ref(consumer_counts[i]));
        }

        start_signal.store(true, std::memory_order_release);
        std::this_thread::sleep_for(run_duration);

        producer_stop_signal.store(true, std::memory_order_release);
        for (auto& t : producer_threads) t.join();

        consumer_stop_signal.store(true, std::memory_order_release);
        for (auto& t : consumer_threads) t.join();

        for (const auto& local_latencies : producer_latencies) {
            global_latencies.insert(global_latencies.end(), local_latencies.begin(), local_latencies.end());
        }
        for (const auto& local_latencies : consumer_latencies) {
            global_latencies.insert(global_latencies.end(), local_latencies.begin(), local_latencies.end());
        }
        for (int i = 0; i < num_consumers; ++i) {
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