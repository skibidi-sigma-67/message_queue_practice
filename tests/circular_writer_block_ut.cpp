#include <gtest/gtest.h>

#include <queue.hpp>
#include <base_message.hpp>
#include <base_queue.hpp>

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <utility>

TEST(CircularWriterBlockQueueTest, BasicPushTryPop) {
    CircularWriterBlockQueue queue(2);
    EXPECT_EQ(queue.Size(), 0);

    Message first_message{.id = 1, .payload = "test1"};
    EXPECT_EQ(queue.Push(std::move(first_message)), PushStatus::kPushed);
    EXPECT_EQ(queue.Size(), 1);

    Message second_message{.id = 2, .payload = "test2"};
    EXPECT_EQ(queue.Push(std::move(second_message)), PushStatus::kPushed);
    EXPECT_EQ(queue.Size(), 2);

    auto first_popped_message = queue.TryPop();
    ASSERT_TRUE(first_popped_message.has_value());
    EXPECT_EQ(first_popped_message->id, 1);
    EXPECT_EQ(queue.Size(), 1);

    auto second_popped_message = queue.TryPop();
    ASSERT_TRUE(second_popped_message.has_value());
    EXPECT_EQ(second_popped_message->id, 2);
    EXPECT_EQ(queue.Size(), 0);

    auto third_popped_message = queue.TryPop();
    EXPECT_FALSE(third_popped_message.has_value());
}

TEST(CircularWriterBlockQueueTest, BasicWaitPop) {
    CircularWriterBlockQueue queue(2);

    Message first_message{.id = 1, .payload = "test1"};
    queue.Push(std::move(first_message));

    auto popped_message = queue.WaitPop();
    ASSERT_TRUE(popped_message.has_value());
    EXPECT_EQ(popped_message->id, 1);
}

TEST(CircularWriterBlockQueueTest, WaitPopBlocks) {
    CircularWriterBlockQueue queue(2);
    std::atomic<bool> is_popped{false};

    std::thread consumer_thread([&]() {
        auto popped_message = queue.WaitPop();

        if (popped_message.has_value() && popped_message->id == 1) {
            is_popped = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(is_popped.load());

    queue.Push(Message{.id = 1});
    consumer_thread.join();
    EXPECT_TRUE(is_popped.load());
}

TEST(CircularWriterBlockQueueTest, PushBlocksOnFull) {
    CircularWriterBlockQueue queue(1);
    queue.Push(Message{.id = 1});

    std::atomic<bool> is_pushed{false};
    std::thread producer_thread([&]() {
        queue.Push(Message{.id = 2});
        is_pushed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(is_pushed.load());

    auto popped_message = queue.WaitPop();
    ASSERT_TRUE(popped_message.has_value());
    EXPECT_EQ(popped_message->id, 1);

    producer_thread.join();
    EXPECT_TRUE(is_pushed.load());
    EXPECT_EQ(queue.Size(), 1);
}

TEST(CircularWriterBlockQueueTest, CloseUnblocksPush) {
    CircularWriterBlockQueue queue(1);
    queue.Push(Message{.id = 1});

    std::atomic<PushStatus> push_status{PushStatus::kPushed};
    std::thread producer_thread([&]() {
        push_status = queue.Push(Message{.id = 2});
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.Close();

    producer_thread.join();
    EXPECT_EQ(push_status.load(), PushStatus::kError);
}

TEST(CircularWriterBlockQueueTest, CloseUnblocksWaitPop) {
    CircularWriterBlockQueue queue(1);

    std::atomic<bool> is_popped_nullopt{false};
    std::thread consumer_thread([&]() {
        auto popped_message = queue.WaitPop();
        if (!popped_message.has_value()) {
            is_popped_nullopt = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.Close();

    consumer_thread.join();
    EXPECT_TRUE(is_popped_nullopt.load());
}

TEST(CircularWriterBlockQueueTest, PushAfterCloseReturnsError) {
    CircularWriterBlockQueue queue(2);
    queue.Close();
    EXPECT_EQ(queue.Push(Message{.id = 1}), PushStatus::kError);
}

TEST(CircularWriterBlockQueueTest, WaitPopAfterCloseDrainsQueue) {
    CircularWriterBlockQueue queue(2);
    queue.Push(Message{.id = 1});
    queue.Push(Message{.id = 2});
    queue.Close();

    auto first_popped_message = queue.WaitPop();
    ASSERT_TRUE(first_popped_message.has_value());
    EXPECT_EQ(first_popped_message->id, 1);

    auto second_popped_message = queue.WaitPop();
    ASSERT_TRUE(second_popped_message.has_value());
    EXPECT_EQ(second_popped_message->id, 2);

    auto third_popped_message = queue.WaitPop();
    EXPECT_FALSE(third_popped_message.has_value());
}

TEST(CircularWriterBlockQueueTest, StatsTracking) {
    CircularWriterBlockQueue queue(5);

    queue.Push(Message{.id = 1});
    queue.Push(Message{.id = 2});

    queue.TryPop();

    auto queue_stats = queue.GetStats();
    EXPECT_EQ(queue_stats.push_count, 2);
    EXPECT_EQ(queue_stats.pop_count, 1);
    EXPECT_EQ(queue_stats.current_size, 1);
    EXPECT_EQ(queue_stats.failed_count, 0);

    queue.Close();
    queue.Push(Message{.id = 3});

    queue_stats = queue.GetStats();
    EXPECT_EQ(queue_stats.failed_count, 1);
}

TEST(CircularWriterBlockQueueTest, PreservesPayload) {
    CircularWriterBlockQueue queue(2);
    queue.Push(Message{.id = 1, .payload = "hello"});
    queue.Push(Message{.id = 2, .payload = "world"});

    auto first_popped_message = queue.WaitPop();
    ASSERT_TRUE(first_popped_message.has_value());
    EXPECT_EQ(first_popped_message->id, 1);
    EXPECT_EQ(first_popped_message->payload, "hello");

    auto second_popped_message = queue.WaitPop();
    ASSERT_TRUE(second_popped_message.has_value());
    EXPECT_EQ(second_popped_message->id, 2);
    EXPECT_EQ(second_popped_message->payload, "world");
}

TEST(CircularWriterBlockQueueTest, NeverDropsMessages) {
    CircularWriterBlockQueue queue(2);
    queue.Push(Message{.id = 1});
    queue.Push(Message{.id = 2});
    queue.TryPop();

    queue.Close();
    queue.Push(Message{.id = 3});

    auto queue_stats = queue.GetStats();
    EXPECT_EQ(queue_stats.dropout_count, 0);
}

TEST(CircularWriterBlockQueueTest, RingBufferWrapAroundPreservesOrder) {
    CircularWriterBlockQueue queue(3);

    uint64_t next_to_push = 1;
    uint64_t next_expected = 1;

    for (int cycle = 0; cycle < 5; ++cycle) {
        while (queue.Size() < 3) {
            EXPECT_EQ(queue.Push(Message{.id = next_to_push}), PushStatus::kPushed);
            ++next_to_push;
        }
        for (int i = 0; i < 2; ++i) {
            auto popped_message = queue.TryPop();
            ASSERT_TRUE(popped_message.has_value());
            EXPECT_EQ(popped_message->id, next_expected);
            ++next_expected;
        }
    }

    while (auto popped_message = queue.TryPop()) {
        EXPECT_EQ(popped_message->id, next_expected);
        ++next_expected;
    }

    EXPECT_EQ(queue.Size(), 0);
    EXPECT_EQ(next_expected, next_to_push);
}

TEST(CircularWriterBlockQueueTest, TryPopUnblocksPush) {
    CircularWriterBlockQueue queue(1);
    queue.Push(Message{.id = 1});

    std::atomic<bool> is_pushed{false};
    std::thread producer_thread([&]() {
        queue.Push(Message{.id = 2});
        is_pushed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(is_pushed.load());

    auto popped_message = queue.TryPop();
    ASSERT_TRUE(popped_message.has_value());
    EXPECT_EQ(popped_message->id, 1);

    producer_thread.join();
    EXPECT_TRUE(is_pushed.load());

    auto second_popped_message = queue.TryPop();
    ASSERT_TRUE(second_popped_message.has_value());
    EXPECT_EQ(second_popped_message->id, 2);
}

TEST(CircularWriterBlockQueueTest, MultipleProducersConsumers) {
    const int num_producers = 4;
    const int num_consumers = 4;
    const int messages_per_producer = 1000;

    CircularWriterBlockQueue queue(100);
    std::atomic<int> received_count{0};

    auto producer_task = [&]() {
        for (int i = 0; i < messages_per_producer; ++i) {
            EXPECT_EQ(queue.Push(Message{}), PushStatus::kPushed);
        }
    };

    auto consumer_task = [&]() {
        while (true) {
            auto popped_message = queue.WaitPop();
            if (!popped_message.has_value()) {
                break;
            }
            ++received_count;
        }
    };

    std::vector<std::thread> producer_threads;
    for (int i = 0; i < num_producers; ++i) {
        producer_threads.emplace_back(producer_task);
    }

    std::vector<std::thread> consumer_threads;
    for (int i = 0; i < num_consumers; ++i) {
        consumer_threads.emplace_back(consumer_task);
    }

    for (auto& thread : producer_threads) {
        thread.join();
    }

    queue.Close();

    for (auto& thread : consumer_threads) {
        thread.join();
    }

    EXPECT_EQ(received_count.load(), num_producers * messages_per_producer);
}
