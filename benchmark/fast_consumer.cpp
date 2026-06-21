#include "fixture.hpp"
#include "benchmark_runner.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, FastConsumer)(benchmark::State& state) {
    RunBenchmarkScenario<false, false, false>(
        state, queue.get(), 
        4, 4, 
        std::chrono::microseconds(50), 
        std::chrono::microseconds(0)
    );
}
REGISTER_QUEUE_BENCHMARK(FastConsumer);