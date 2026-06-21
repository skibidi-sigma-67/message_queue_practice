#pragma once

#include <benchmark/benchmark.h>

#include <message_queue/factory.hpp>
#include <message_queue/base_queue.hpp>

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
