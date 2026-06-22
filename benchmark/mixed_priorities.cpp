#include "fixture.hpp"
#include "continuous_benchmark.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, MixedPriorities)(benchmark::State& state) {
    RunContinuousScenario<false, true, false>(
        state,
        queue.get(),
        state.range(2),
        state.range(3),
        std::chrono::microseconds(10),
        std::chrono::microseconds(10)
    );
}
REGISTER_QUEUE_BENCHMARK(MixedPriorities);
