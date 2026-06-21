#include <message_queue/policies/lock_free_queue.hpp>

#include <message_queue/base_message.hpp>
#include <message_queue/base_queue.hpp>
#include <message_queue/stats.hpp>

#include <atomic>
#include <utility>
#include <optional>
#include <cstddef>


struct LockFreeQueue::Node {
    Node() = default;

    explicit Node(Message message)
        : message(std::move(message)) {}

    Message message{};
    std::atomic<Node*> next{nullptr};
    Node* retired_next = nullptr;
};

LockFreeQueue::LockFreeQueue() {
    auto* dummy = new Node();
    head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
}

LockFreeQueue::~LockFreeQueue() {
    Close();
    DeleteAllNodes();
}

size_t LockFreeQueue::Size() const {
    return size_.load(std::memory_order_relaxed);
}

QueueStats LockFreeQueue::GetStats() const {
    return QueueStats{
        push_count_.load(std::memory_order_relaxed),
        pop_count_.load(std::memory_order_relaxed),
        dropout_count_.load(std::memory_order_relaxed),
        failed_count_.load(std::memory_order_relaxed),
        size_.load(std::memory_order_relaxed),
    };
}

PushStatus LockFreeQueue::Push(Message message) {
    if (is_closed_.load(std::memory_order_acquire)) {
        failed_count_.fetch_add(1, std::memory_order_relaxed);
        return PushStatus::kError;
    }

    auto* new_node = new Node(std::move(message));

    while (true) {
        if (is_closed_.load(std::memory_order_acquire)) {
            delete new_node;
            failed_count_.fetch_add(1, std::memory_order_relaxed);
            return PushStatus::kError;
        }

        Node* tail = tail_.load(std::memory_order_acquire);
        Node* next = tail->next.load(std::memory_order_acquire);

        if (tail != tail_.load(std::memory_order_acquire)) {
            continue;
        }

        if (next == nullptr) {
            Node* expected_next = nullptr;
            if (tail->next.compare_exchange_weak(
                    expected_next,
                    new_node,
                    std::memory_order_release,
                    std::memory_order_relaxed)) {
                tail_.compare_exchange_weak(
                    tail,
                    new_node,
                    std::memory_order_release,
                    std::memory_order_relaxed);

                push_count_.fetch_add(1, std::memory_order_relaxed);
                size_.fetch_add(1, std::memory_order_relaxed);
                wake_sequence_.fetch_add(1, std::memory_order_release);
                wake_sequence_.notify_one();
                return PushStatus::kPushed;
            }
            continue;
        }

        tail_.compare_exchange_weak(
            tail,
            next,
            std::memory_order_release,
            std::memory_order_relaxed);
    }
}

std::optional<Message> LockFreeQueue::TryPop() {
    while (true) {
        Node* head = head_.load(std::memory_order_acquire);
        Node* tail = tail_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);

        if (head != head_.load(std::memory_order_acquire)) {
            continue;
        }

        if (next == nullptr) {
            return std::nullopt;
        }

        if (head == tail) {
            tail_.compare_exchange_weak(
                tail,
                next,
                std::memory_order_release,
                std::memory_order_relaxed);
            continue;
        }

        if (head_.compare_exchange_weak(
                head,
                next,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            Message message = std::move(next->message);

            pop_count_.fetch_add(1, std::memory_order_relaxed);
            size_.fetch_sub(1, std::memory_order_relaxed);
            RetireNode(head);

            return message;
        }
    }
}

std::optional<Message> LockFreeQueue::WaitPop() {
    while (true) {
        if (auto message = TryPop()) {
            return message;
        }

        if (is_closed_.load(std::memory_order_acquire) && Size() == 0) {
            return std::nullopt;
        }

        size_t observed_wake_sequence = wake_sequence_.load(std::memory_order_acquire);

        if (auto message = TryPop()) {
            return message;
        }

        if (is_closed_.load(std::memory_order_acquire) && Size() == 0) {
            return std::nullopt;
        }

        wake_sequence_.wait(observed_wake_sequence, std::memory_order_acquire);
    }
}

void LockFreeQueue::Close() {
    is_closed_.store(true, std::memory_order_release);
    wake_sequence_.fetch_add(1, std::memory_order_release);
    wake_sequence_.notify_all();
}

void LockFreeQueue::RetireNode(Node* node) noexcept {
    Node* retired_head = retired_head_.load(std::memory_order_relaxed);

    do {
        node->retired_next = retired_head;
    } while (!retired_head_.compare_exchange_weak(
        retired_head,
        node,
        std::memory_order_release,
        std::memory_order_relaxed));
}

void LockFreeQueue::DeleteAllNodes() noexcept {
    Node* node = head_.exchange(nullptr, std::memory_order_acq_rel);
    tail_.store(nullptr, std::memory_order_release);

    while (node != nullptr) {
        Node* next = node->next.load(std::memory_order_relaxed);
        delete node;
        node = next;
    }

    Node* retired_node = retired_head_.exchange(nullptr, std::memory_order_acq_rel);

    while (retired_node != nullptr) {
        Node* next = retired_node->retired_next;
        delete retired_node;
        retired_node = next;
    }
}
