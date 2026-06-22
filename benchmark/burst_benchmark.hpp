#pragma once

#include <benchmark/benchmark.h>
#include <message_queue/base_queue.hpp>
#include "utils.hpp"
#include "workers.hpp"

#include <atomic>
#include <thread>
#include <vector>

inline void RunBurstScenario(
    benchmark::State& state,
    BaseQueue* target_queue,
    int producer_count,
    int consumer_count,
    int messages_per_producer
) {
    for (auto _ : state) {
        state.PauseTiming();

        std::atomic<bool> start_signal_write{false};
        std::vector<std::thread> producers;
        producers.reserve(producer_count);

        for (int i = 0; i < producer_count; ++i) {
            producers.emplace_back(
                BurstProducer,
                target_queue,
                i,
                messages_per_producer,
                std::ref(start_signal_write)
            );
        }

        state.ResumeTiming();
        start_signal_write.store(true, std::memory_order_release);

        for (auto& t : producers) {
            t.join();
        }

        state.PauseTiming();

        std::atomic<bool> start_signal_read{false};
        std::vector<std::thread> consumers;
        consumers.reserve(consumer_count);

        for (int i = 0; i < consumer_count; ++i) {
            consumers.emplace_back(
                BurstConsumer,
                target_queue,
                std::ref(start_signal_read)
            );
        }

        state.ResumeTiming();
        start_signal_read.store(true, std::memory_order_release);

        for (auto& t : consumers) {
            t.join();
        }
    }

    SetQueueStatsCounters(state, target_queue);
}
