#include <gtest/gtest.h>

#include <lock_free_queue/queue.hpp>
#include <base_message.hpp>
#include <base_queue.hpp>


#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace {

Message MakeMessage(uint64_t id) {
    return Message{
        id,
        1,
        1,
        "msg",
        std::chrono::steady_clock::now(),
    };
}

}  // namespace

TEST(LockFreeQueueTest, PushAndTryPop) {
    LockFreeQueue queue;

    EXPECT_EQ(queue.Push(MakeMessage(1)), PushStatus::kPushed);
    EXPECT_EQ(queue.Size(), 1);

    EXPECT_EQ(queue.Push(MakeMessage(2)), PushStatus::kPushed);
    EXPECT_EQ(queue.Size(), 2);

    auto popped1 = queue.TryPop();
    ASSERT_TRUE(popped1.has_value());
    EXPECT_EQ(popped1->id, 1);

    auto popped2 = queue.TryPop();
    ASSERT_TRUE(popped2.has_value());
    EXPECT_EQ(popped2->id, 2);

    EXPECT_FALSE(queue.TryPop().has_value());
    EXPECT_EQ(queue.Size(), 0);
}

TEST(LockFreeQueueTest, DoesNotDropMessagesWhenGrowing) {
    LockFreeQueue queue;

    constexpr size_t kMessageCount = 16;
    for (size_t i = 0; i < kMessageCount; ++i) {
        EXPECT_EQ(queue.Push(MakeMessage(static_cast<uint64_t>(i))), PushStatus::kPushed);
    }

    EXPECT_EQ(queue.Size(), kMessageCount);

    for (size_t i = 0; i < kMessageCount; ++i) {
        auto popped = queue.TryPop();
        ASSERT_TRUE(popped.has_value());
        EXPECT_EQ(popped->id, i);
    }

    EXPECT_FALSE(queue.TryPop().has_value());
    EXPECT_EQ(queue.Size(), 0);
}

TEST(LockFreeQueueTest, WaitPopBlocks) {
    LockFreeQueue queue;

    std::thread producer([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        queue.Push(MakeMessage(1));
    });

    auto start = std::chrono::steady_clock::now();
    auto popped = queue.WaitPop();
    auto end = std::chrono::steady_clock::now();

    producer.join();

    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped->id, 1);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 40);
}

TEST(LockFreeQueueTest, CloseUnblocksWaitPop) {
    LockFreeQueue queue;

    std::thread closer([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        queue.Close();
    });

    auto start = std::chrono::steady_clock::now();
    auto popped = queue.WaitPop();
    auto end = std::chrono::steady_clock::now();

    closer.join();

    EXPECT_FALSE(popped.has_value());
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 40);
}

TEST(LockFreeQueueTest, PushToClosedQueue) {
    LockFreeQueue queue;
    queue.Close();

    EXPECT_EQ(queue.Push(MakeMessage(1)), PushStatus::kError);
    EXPECT_EQ(queue.Size(), 0);
}

TEST(LockFreeQueueTest, StatsTracking) {
    LockFreeQueue queue;

    queue.Push(MakeMessage(1));
    queue.Push(MakeMessage(2));
    queue.Push(MakeMessage(3));

    queue.TryPop();

    queue.Close();
    queue.Push(MakeMessage(4));

    auto stats = queue.GetStats();
    EXPECT_EQ(stats.push_count, 3);
    EXPECT_EQ(stats.pop_count, 1);
    EXPECT_EQ(stats.dropout_count, 0);
    EXPECT_EQ(stats.failed_count, 1);
    EXPECT_EQ(stats.current_size, 2);
}

TEST(LockFreeQueueTest, SizeInvariants) {
    LockFreeQueue queue;

    EXPECT_EQ(queue.Size(), 0);
    EXPECT_FALSE(queue.TryPop().has_value());
    EXPECT_EQ(queue.Size(), 0);

    constexpr size_t kMessageCount = 10;
    for (size_t i = 0; i < kMessageCount; ++i) {
        queue.Push(MakeMessage(static_cast<uint64_t>(i)));
        EXPECT_EQ(queue.Size(), i + 1);
    }

    for (size_t i = 0; i < kMessageCount; ++i) {
        EXPECT_TRUE(queue.TryPop().has_value());
        EXPECT_EQ(queue.Size(), kMessageCount - i - 1);
    }

    EXPECT_EQ(queue.Size(), 0);
    EXPECT_FALSE(queue.TryPop().has_value());
}

TEST(LockFreeQueueTest, MultipleProducersMultipleConsumers) {
    LockFreeQueue queue;

    constexpr size_t kProducerCount = 4;
    constexpr size_t kConsumerCount = 4;
    constexpr size_t kMessagesPerProducer = 250;
    constexpr size_t kTotalMessages = kProducerCount * kMessagesPerProducer;

    std::atomic<size_t> consumed_count{0};
    std::atomic<size_t> failed_pushes{0};
    std::atomic<size_t> duplicate_count{0};
    std::atomic<size_t> invalid_count{0};
    std::vector<bool> seen(kTotalMessages, false);
    std::mutex seen_mutex;

    std::vector<std::thread> consumers;
    consumers.reserve(kConsumerCount);
    for (size_t consumer_id = 0; consumer_id < kConsumerCount; ++consumer_id) {
        consumers.emplace_back([&queue,
                                &consumed_count,
                                &duplicate_count,
                                &invalid_count,
                                &seen,
                                &seen_mutex]() {
            while (true) {
                auto popped = queue.WaitPop();
                if (!popped.has_value()) {
                    return;
                }

                const auto id = static_cast<size_t>(popped->id);
                if (id >= kTotalMessages) {
                    invalid_count.fetch_add(1, std::memory_order_relaxed);
                } else {
                    std::lock_guard<std::mutex> lock(seen_mutex);
                    if (seen[id]) {
                        duplicate_count.fetch_add(1, std::memory_order_relaxed);
                    }
                    seen[id] = true;
                }
                consumed_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    std::vector<std::thread> producers;
    producers.reserve(kProducerCount);
    for (size_t producer_id = 0; producer_id < kProducerCount; ++producer_id) {
        producers.emplace_back([&queue, &failed_pushes, producer_id]() {
            for (size_t index = 0; index < kMessagesPerProducer; ++index) {
                const size_t id = producer_id * kMessagesPerProducer + index;
                if (queue.Push(MakeMessage(static_cast<uint64_t>(id))) != PushStatus::kPushed) {
                    failed_pushes.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }
    queue.Close();

    for (auto& consumer : consumers) {
        consumer.join();
    }

    EXPECT_EQ(failed_pushes.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(consumed_count.load(std::memory_order_relaxed), kTotalMessages);
    EXPECT_EQ(duplicate_count.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(invalid_count.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(queue.Size(), 0);

    for (bool was_seen : seen) {
        EXPECT_TRUE(was_seen);
    }

    auto stats = queue.GetStats();
    EXPECT_EQ(stats.push_count, kTotalMessages);
    EXPECT_EQ(stats.pop_count, kTotalMessages);
    EXPECT_EQ(stats.dropout_count, 0);
    EXPECT_EQ(stats.failed_count, 0);
    EXPECT_EQ(stats.current_size, 0);
}
