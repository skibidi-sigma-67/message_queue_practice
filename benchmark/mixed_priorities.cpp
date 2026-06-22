#include "fixture.hpp"
#include "continuous_benchmark.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, MixedPriorities)(benchmark::State& state) {
    RunContinuousScenario<false, true, false>(
        state,
        queue.get(),
        4, 4,
        std::chrono::microseconds(10),
        std::chrono::microseconds(10)
    );
}
REGISTER_QUEUE_BENCHMARK(MixedPriorities);
