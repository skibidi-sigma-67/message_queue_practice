#include "fixture.hpp"
#include "burst_benchmark.hpp"

BENCHMARK_DEFINE_F(QueueBenchmarkFixture, BurstWriteThenRead)(benchmark::State& state) {
    int num_producers = 4;
    int num_consumers = 4;
    int burst_size = 500;

    RunBurstScenario(
        state,
        queue.get(),
        num_producers,
        num_consumers,
        burst_size
    );
}

REGISTER_QUEUE_BENCHMARK(BurstWriteThenRead);
