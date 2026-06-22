#include "fixture.hpp"
#include "continuous_benchmark.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, RandomBursts)(benchmark::State& state) {
    RunContinuousScenario<true, false, true>(
        state,
        queue.get(),
        state.range(2),
        state.range(3),
        std::chrono::microseconds(20),
        std::chrono::microseconds(20)
    );
}
REGISTER_QUEUE_BENCHMARK(RandomBursts);
