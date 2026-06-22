#pragma once

#include <message_queue/base_queue.hpp>
#include <message_queue/utils.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <random>
#include <thread>
#include <vector>

#include "utils.hpp"

template <bool UseExponentialDelay = false, bool UseRandomPriority = false>
inline void Producer(
    BaseQueue* target_queue,
    int thread_id,
    std::chrono::microseconds base_delay,
    std::atomic<bool>& start_signal,
    std::atomic<bool>& stop_signal,
    std::vector<double>& latencies
) {
    std::mt19937 random_number_generator(thread_id);
    double mean_delay = std::max<double>(1.0, static_cast<double>(base_delay.count()));
    std::exponential_distribution<> exponential_distribution(1.0 / mean_delay);

    while (!start_signal.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    while (!stop_signal.load(std::memory_order_acquire)) {
        Message generated_message;
        generated_message.producer_id = thread_id;

        if constexpr (UseRandomPriority) {
            generated_message.priority = random_number_generator() % 10;
        }

        double latency_microseconds = MeasureExecutionTime([&]() {
            target_queue->Push(generated_message);
        });

        latencies.push_back(latency_microseconds);

        if constexpr (UseExponentialDelay) {
            long long random_delay_count = static_cast<long long>(exponential_distribution(random_number_generator));
            SpinWait(std::chrono::microseconds(random_delay_count));
        } else {
            if (base_delay.count() > 0) {
                SpinWait(base_delay);
            }
        }
    }
}

template <bool UseExponentialDelay = false>
inline void Consumer(
    BaseQueue* target_queue,
    int thread_id,
    std::chrono::microseconds base_delay,
    std::atomic<bool>& start_signal,
    std::atomic<bool>& stop_signal,
    std::vector<double>& latencies,
    std::map<int, int>& consumed_counts)
{
    std::mt19937 random_number_generator(thread_id);
    double mean_delay = std::max<double>(1.0, static_cast<double>(base_delay.count()));
    std::exponential_distribution<> exponential_distribution(1.0 / mean_delay);

    while (!start_signal.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    while (!stop_signal.load(std::memory_order_acquire)) {
        auto [received_message, latency_microseconds] = MeasureExecutionTime([&]() {
            return target_queue->TryPop();
        });

        if (received_message.has_value()) {
            latencies.push_back(latency_microseconds);
            consumed_counts[received_message->producer_id]++;

            if constexpr (UseExponentialDelay) {
                long long random_delay_count = static_cast<long long>(exponential_distribution(random_number_generator));
                SpinWait(std::chrono::microseconds(random_delay_count));
            } else {
                if (base_delay.count() > 0) {
                    SpinWait(base_delay);
                }
            }
        } else {
            std::this_thread::yield();
        }
    }
}

inline void BurstProducer(
    BaseQueue* target_queue,
    int thread_id,
    int messages_to_push_count,
    std::atomic<bool>& start_signal
) {
    while (!start_signal.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    for (int i = 0; i < messages_to_push_count; ++i) {
        Message msg;
        msg.producer_id = thread_id;
        target_queue->Push(msg);
    }
}

inline void BurstConsumer(
    BaseQueue* target_queue,
    std::atomic<bool>& start_signal
) {
    while (!start_signal.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }

    while (true) {
        auto msg = target_queue->TryPop();
        if (!msg.has_value()) {
            break;
        }
    }
}
