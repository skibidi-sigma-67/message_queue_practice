#include "fixture.hpp"
#include "burst_benchmark.hpp"

#include <cstddef>

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, BurstWriteThenRead)(benchmark::State& state) {
    size_t burst_size = 500;

    RunBurstScenario(
        state,
        queue.get(),
        state.range(2),
        state.range(3),
        burst_size
    );
}

REGISTER_QUEUE_BENCHMARK(BurstWriteThenRead);
