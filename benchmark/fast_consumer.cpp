#include "fixture.hpp"
#include "continuous_benchmark.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, FastConsumer)(benchmark::State& state) {
    RunContinuousScenario<false, false, false>(
        state,
        queue.get(),
        state.range(2),
        state.range(3),
        std::chrono::microseconds(50),
        std::chrono::microseconds(0)
    );
}
REGISTER_QUEUE_BENCHMARK(FastConsumer);
