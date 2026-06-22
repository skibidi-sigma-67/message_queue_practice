#include "fixture.hpp"
#include "continuous_benchmark.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, FastProducer)(benchmark::State& state) {
    RunContinuousScenario<false, false, false>(
        state,
        queue.get(),
        4, 4,
        std::chrono::microseconds(0),
        std::chrono::microseconds(50)
    );
}
REGISTER_QUEUE_BENCHMARK(FastProducer);
