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
            ->Args({static_cast<size_t>(QueuePolicy::kBlockingBounded),     1024, 4, 4}) \
            ->Args({static_cast<size_t>(QueuePolicy::kCircularDropOldest),  1024, 4, 4}) \
            ->Args({static_cast<size_t>(QueuePolicy::kCircularWriterBlock), 1024, 4, 4}) \
            ->Args({static_cast<size_t>(QueuePolicy::kPriority),            1024, 4, 4}) \
            ->Args({static_cast<size_t>(QueuePolicy::kLockFree),            1024, 4, 4}) \
            ->UseRealTime();
