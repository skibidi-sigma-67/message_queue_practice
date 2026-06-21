#include "fixture.hpp"
#include "benchmark_runner.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, RandomBursts)(benchmark::State& state) {
    RunBenchmarkScenario<true, false, true>(
        state, queue.get(), 
        4, 4, 
        std::chrono::microseconds(20), 
        std::chrono::microseconds(20)
    );
}
REGISTER_QUEUE_BENCHMARK(RandomBursts);