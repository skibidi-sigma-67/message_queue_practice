#include <gtest/gtest.h>
#include <message_queue/policies/circular_drop_oldest_queue.hpp>
#include <thread>
#include <chrono>
#include <optional>
#include <cstddef>

#include <message_queue/base_message.hpp>
#include <message_queue/base_queue.hpp>

TEST(CircularDropOldestQueueTest, PushAndTryPop) {
    CircularDropOldestQueue queue(2);
    
    Message msg1{1, 1, 1, "msg1", std::chrono::steady_clock::now()};
    EXPECT_EQ(queue.Push(msg1), PushStatus::kPushed);
    EXPECT_EQ(queue.Size(), 1);
    
    Message msg2{2, 1, 1, "msg2", std::chrono::steady_clock::now()};
    EXPECT_EQ(queue.Push(msg2), PushStatus::kPushed);
    EXPECT_EQ(queue.Size(), 2);
    
    auto popped1 = queue.TryPop();
    ASSERT_TRUE(popped1.has_value());
    EXPECT_EQ(popped1->id, 1);
    
    auto popped2 = queue.TryPop();
    ASSERT_TRUE(popped2.has_value());
    EXPECT_EQ(popped2->id, 2);
    
    EXPECT_FALSE(queue.TryPop().has_value());
}

TEST(CircularDropOldestQueueTest, DropOldestWhenFull) {
    CircularDropOldestQueue queue(2);
    
    queue.Push(Message{1, 1, 1, "msg1", std::chrono::steady_clock::now()});
    queue.Push(Message{2, 1, 1, "msg2", std::chrono::steady_clock::now()});
    
    EXPECT_EQ(queue.Push(Message{3, 1, 1, "msg3", std::chrono::steady_clock::now()}), PushStatus::kDropped);
    EXPECT_EQ(queue.Size(), 2);
    
    auto popped1 = queue.TryPop();
    ASSERT_TRUE(popped1.has_value());
    EXPECT_EQ(popped1->id, 2); 
    
    auto popped2 = queue.TryPop();
    ASSERT_TRUE(popped2.has_value());
    EXPECT_EQ(popped2->id, 3);
}

TEST(CircularDropOldestQueueTest, WaitPopBlocks) {
    CircularDropOldestQueue queue(2);
    
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        queue.Push(Message{1, 1, 1, "msg1", std::chrono::steady_clock::now()});
    });
    
    auto start = std::chrono::steady_clock::now();
    auto popped = queue.WaitPop();
    auto end = std::chrono::steady_clock::now();
    
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped->id, 1);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 40);
    
    t.join();
}

TEST(CircularDropOldestQueueTest, CloseUnblocksWaitPop) {
    CircularDropOldestQueue queue(2);
    
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        queue.Close();
    });
    
    auto start = std::chrono::steady_clock::now();
    auto popped = queue.WaitPop();
    auto end = std::chrono::steady_clock::now();
    
    EXPECT_FALSE(popped.has_value());
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 40);
    
    t.join();
}

TEST(CircularDropOldestQueueTest, PushToClosedQueue) {
    CircularDropOldestQueue queue(2);
    queue.Close();
    
    EXPECT_EQ(queue.Push(Message{1, 1, 1, "msg1", std::chrono::steady_clock::now()}), PushStatus::kError);
}

TEST(CircularDropOldestQueueTest, StatsTracking) {
    CircularDropOldestQueue queue(2);
    
    queue.Push(Message{1, 1, 1, "msg1", std::chrono::steady_clock::now()});
    queue.Push(Message{2, 1, 1, "msg2", std::chrono::steady_clock::now()});
    queue.Push(Message{3, 1, 1, "msg3", std::chrono::steady_clock::now()}); 
    
    queue.TryPop();
    
    queue.Close();
    queue.Push(Message{4, 1, 1, "msg4", std::chrono::steady_clock::now()}); 
    
    auto stats = queue.GetStats();
    EXPECT_EQ(stats.push_count, 3);
    EXPECT_EQ(stats.pop_count, 1);
    EXPECT_EQ(stats.dropout_count, 1);
    EXPECT_EQ(stats.failed_count, 1);
    EXPECT_EQ(stats.current_size, 1);
}

TEST(CircularDropOldestQueueTest, SizeInvariants) {
    const size_t capacity = 3;
    CircularDropOldestQueue queue(capacity);
    
    EXPECT_EQ(queue.Size(), 0);
    EXPECT_LE(queue.Size(), capacity);

    queue.TryPop();
    EXPECT_EQ(queue.Size(), 0);

    for (size_t i = 0; i < capacity; ++i) {
        queue.Push(Message{i, 1, 1, "msg", std::chrono::steady_clock::now()});
        EXPECT_LE(queue.Size(), capacity);
    }
    EXPECT_EQ(queue.Size(), capacity);

    for (size_t i = 0; i < 10; ++i) {
        queue.Push(Message{capacity + i, 1, 1, "msg", std::chrono::steady_clock::now()});
        EXPECT_EQ(queue.Size(), capacity);
        EXPECT_LE(queue.Size(), capacity);
    }

    for (size_t i = 0; i < capacity; ++i) {
        EXPECT_TRUE(queue.TryPop().has_value());
        EXPECT_LE(queue.Size(), capacity);
    }

    EXPECT_EQ(queue.Size(), 0);
    EXPECT_FALSE(queue.TryPop().has_value());
    EXPECT_EQ(queue.Size(), 0);
}
