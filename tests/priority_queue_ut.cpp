#include <gtest/gtest.h>
#include <queue.hpp>
#include <thread>
#include <chrono>
#include <optional>

#include <base_message.hpp>
#include <base_queue.hpp>
#include <base_queue.hpp>


TEST(PriorityQueueTest, PushAndTryPop) {
    PriorityQueue queue;
    
    Message msg1{1, 1, 10, "low_priority", std::chrono::steady_clock::now()};
    EXPECT_EQ(queue.Push(msg1), PushStatus::kPushed);
    EXPECT_EQ(queue.Size(), 1);
    
    auto popped1 = queue.TryPop();
    ASSERT_TRUE(popped1.has_value());
    EXPECT_EQ(popped1->id, 1);
    
    EXPECT_FALSE(queue.TryPop().has_value());
}

TEST(PriorityQueueTest, PriorityOrdering) {
    PriorityQueue queue;

    queue.Push(Message{1, 1, 1, "low", std::chrono::steady_clock::now()});
    queue.Push(Message{2, 1, 10, "high", std::chrono::steady_clock::now()});
    queue.Push(Message{3, 1, 5, "medium", std::chrono::steady_clock::now()});
    
    EXPECT_EQ(queue.Size(), 3);
    
    auto popped1 = queue.TryPop();
    ASSERT_TRUE(popped1.has_value());
    EXPECT_EQ(popped1->id, 2); 

    auto popped2 = queue.TryPop();
    ASSERT_TRUE(popped2.has_value());
    EXPECT_EQ(popped2->id, 3);

    auto popped3 = queue.TryPop();
    ASSERT_TRUE(popped3.has_value());
    EXPECT_EQ(popped3->id, 1);
}


TEST(PriorityQueueTest, EqualPriorityFIFO) {
    PriorityQueue queue;

    queue.Push(Message{1, 1, 5, "first", std::chrono::steady_clock::now()});
    queue.Push(Message{2, 1, 5, "second", std::chrono::steady_clock::now()});
    
    auto popped1 = queue.TryPop();
    ASSERT_TRUE(popped1.has_value());
    EXPECT_EQ(popped1->id, 1);
    
    auto popped2 = queue.TryPop();
    ASSERT_TRUE(popped2.has_value());
    EXPECT_EQ(popped2->id, 2);
}

TEST(PriorityQueueTest, WaitPopBlocks) {
    PriorityQueue queue;
    
    std::thread t([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        queue.Push(Message{1, 1, 5, "msg", std::chrono::steady_clock::now()});
    });
    
    auto start = std::chrono::steady_clock::now();
    auto popped = queue.WaitPop();
    auto end = std::chrono::steady_clock::now();
    
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(popped->id, 1);
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 40);
    
    t.join();
}

TEST(PriorityQueueTest, CloseUnblocksWaitPop) {
    PriorityQueue queue;
    
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

TEST(PriorityQueueTest, PushToClosedQueue) {
    PriorityQueue queue;
    queue.Close();
    
    EXPECT_EQ(queue.Push(Message{1, 1, 5, "msg", std::chrono::steady_clock::now()}), PushStatus::kError);
}

TEST(PriorityQueueTest, StatsTracking) {
    PriorityQueue queue;
    
    queue.Push(Message{1, 1, 5, "msg1", std::chrono::steady_clock::now()});
    queue.Push(Message{2, 1, 10, "msg2", std::chrono::steady_clock::now()});
    
    queue.TryPop();
    
    queue.Close();
    queue.Push(Message{3, 1, 1, "msg3", std::chrono::steady_clock::now()}); 
    
    auto stats = queue.GetStats();
    EXPECT_EQ(stats.push_count, 2);
    EXPECT_EQ(stats.pop_count, 1);
    EXPECT_EQ(stats.current_size, 1);
}