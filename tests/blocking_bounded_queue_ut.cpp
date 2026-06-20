#include <gtest/gtest.h>

#include <queue.hpp>
#include <base_message.hpp>

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

TEST(BlockingBoundedQueueTest, BasicPushTryPop) {
    BlockingBoundedQueue queue(2);
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

TEST(BlockingBoundedQueueTest, BasicWaitPop) {
    BlockingBoundedQueue queue(2);

    Message first_message{.id = 1, .payload = "test1"};
    queue.Push(std::move(first_message));

    auto popped_message = queue.WaitPop();
    ASSERT_TRUE(popped_message.has_value());
    EXPECT_EQ(popped_message->id, 1);
}

TEST(BlockingBoundedQueueTest, WaitPopBlocks) {
    BlockingBoundedQueue queue(2);
    std::atomic<bool> is_popped{false};

    std::thread consumer_thread([&]() {
        auto popped_message = queue.WaitPop();

        if (popped_message.has_value() && popped_message->id == 1) {
            is_popped = true;
        }
    });

    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(is_popped.load());

    queue.Push(Message{.id = 1});
    consumer_thread.join();
    EXPECT_TRUE(is_popped.load());
}

TEST(BlockingBoundedQueueTest, PushBlocksOnFull) {
    BlockingBoundedQueue queue(1);
    queue.Push(Message{.id = 1});

    std::atomic<bool> is_pushed{false};
    std::thread producer_thread([&]() {
        queue.Push(Message{.id = 2});
        is_pushed = true;
    });

    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(is_pushed.load());

    auto popped_message = queue.WaitPop();
    ASSERT_TRUE(popped_message.has_value());
    EXPECT_EQ(popped_message->id, 1);

    producer_thread.join();
    EXPECT_TRUE(is_pushed.load());
    EXPECT_EQ(queue.Size(), 1);
}

TEST(BlockingBoundedQueueTest, CloseUnblocksPush) {
    BlockingBoundedQueue queue(1);
    queue.Push(Message{.id = 1});

    std::atomic<PushStatus> push_status{PushStatus::kPushed};
    std::thread producer_thread([&]() {
        push_status = queue.Push(Message{.id = 2});
    });

    std::this_thread::sleep_for(50ms);
    queue.Close();

    producer_thread.join();
    EXPECT_EQ(push_status.load(), PushStatus::kError);
}

TEST(BlockingBoundedQueueTest, CloseUnblocksWaitPop) {
    BlockingBoundedQueue queue(1);

    std::atomic<bool> is_popped_nullopt{false};
    std::thread consumer_thread([&]() {
        auto popped_message = queue.WaitPop();
        if (!popped_message.has_value()) {
            is_popped_nullopt = true;
        }
    });

    std::this_thread::sleep_for(50ms);
    queue.Close();

    consumer_thread.join();
    EXPECT_TRUE(is_popped_nullopt.load());
}

TEST(BlockingBoundedQueueTest, PushAfterCloseReturnsError) {
    BlockingBoundedQueue queue(2);
    queue.Close();
    EXPECT_EQ(queue.Push(Message{.id = 1}), PushStatus::kError);
}

TEST(BlockingBoundedQueueTest, WaitPopAfterCloseDrainsQueue) {
    BlockingBoundedQueue queue(2);
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

TEST(BlockingBoundedQueueTest, StatsTracking) {
    BlockingBoundedQueue queue(5);

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

TEST(BlockingBoundedQueueTest, MultipleProducersConsumers) {
    const int num_producers = 4;
    const int num_consumers = 4;
    const int messages_per_producer = 1000;

    BlockingBoundedQueue queue(100);
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
