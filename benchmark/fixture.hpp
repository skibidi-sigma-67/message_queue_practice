#pragma once

#include <benchmark/benchmark.h>

#include <message_queue/factory.hpp>
#include <message_queue/base_queue.hpp>

#include <cstddef>
#include <memory>

class QueueBenchmarkFixture : public benchmark::Fixture {
public:
    std::unique_ptr<BaseQueue> queue;

    void SetUp(const benchmark::State& state) override {
        auto policy = static_cast<QueuePolicy>(state.range(0));
        size_t capacity = static_cast<size_t>(state.range(1));

        queue = CreateQueue(policy, capacity);
    }

    void TearDown(const benchmark::State& state) override {
        queue.reset();
    }
};

/*
    state.range(0) - Policy
    state.range(1) - Queue Capacity
    state.range(2) - Producer Count
    state.range(3) - Consumer Count
*/
#define REGISTER_QUEUE_BENCHMARK(BenchmarkName) \
        BENCHMARK_REGISTER_F(QueueBenchmarkFixture, BenchmarkName) \
            ->ArgsProduct({ \
                { \
                    static_cast<int64_t>(QueuePolicy::kBlockingBounded), \
                    static_cast<int64_t>(QueuePolicy::kCircularDropOldest), \
                    static_cast<int64_t>(QueuePolicy::kCircularWriterBlock), \
                    static_cast<int64_t>(QueuePolicy::kPriority), \
                    static_cast<int64_t>(QueuePolicy::kLockFree) \
                }, \
                {1024, 65536}, \
                {1, 4}, \
                {1, 4} \
            }) \
            ->Repetitions(3) \
            ->DisplayAggregatesOnly(true) \
            ->UseRealTime();
