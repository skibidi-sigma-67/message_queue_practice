#include <message_queue/policies/lock_free_queue.hpp>

#include <message_queue/base_message.hpp>
#include <message_queue/base_queue.hpp>
#include <message_queue/stats.hpp>
#include <message_queue/utils.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>
#include <utility>

namespace {

constexpr size_t kHazardPointersPerThread = 2;

struct HazardRecord {
    std::atomic<bool> is_active{false};
    std::array<std::atomic<void*>, kHazardPointersPerThread> pointers{};
    std::atomic<HazardRecord*> next{nullptr};
};

std::atomic<HazardRecord*> hazard_records_head{nullptr};

HazardRecord* TryAcquireExistingHazardRecord() noexcept {
    HazardRecord* record = hazard_records_head.load(std::memory_order_acquire);

    while (record != nullptr) {
        bool expected = false;
        if (record->is_active.compare_exchange_strong(
                expected,
                true,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return record;
        }

        record = record->next.load(std::memory_order_acquire);
    }

    return nullptr;
}

HazardRecord* CreateHazardRecord() {
    auto* new_record = new HazardRecord();
    new_record->is_active.store(true, std::memory_order_relaxed);

    HazardRecord* old_head = hazard_records_head.load(std::memory_order_acquire);
    do {
        new_record->next.store(old_head, std::memory_order_relaxed);
    } while (!hazard_records_head.compare_exchange_weak(
        old_head,
        new_record,
        std::memory_order_release,
        std::memory_order_acquire));

    return new_record;
}

HazardRecord* AcquireHazardRecord() {
    if (HazardRecord* record = TryAcquireExistingHazardRecord()) {
        return record;
    }

    return CreateHazardRecord();
}

class HazardRecordOwner {
public:
    HazardRecordOwner()
        : record_(AcquireHazardRecord()) {}

    ~HazardRecordOwner() {
        Clear();
        record_->is_active.store(false, std::memory_order_release);
    }

    HazardRecordOwner(const HazardRecordOwner&) = delete;
    HazardRecordOwner& operator=(const HazardRecordOwner&) = delete;

    std::atomic<void*>& GetPointer(size_t index) noexcept {
        return record_->pointers[index];
    }

    void Clear() noexcept {
        for (auto& pointer : record_->pointers) {
            pointer.store(nullptr, std::memory_order_release);
        }
    }

private:
    HazardRecord* record_;
};

HazardRecordOwner& GetHazardRecordForCurrentThread() {
    thread_local HazardRecordOwner hazard_record_owner;
    return hazard_record_owner;
}

bool IsHazardPointer(void* node) noexcept {
    HazardRecord* record = hazard_records_head.load(std::memory_order_acquire);

    while (record != nullptr) {
        for (auto& pointer : record->pointers) {
            if (pointer.load(std::memory_order_acquire) == node) {
                return true;
            }
        }

        record = record->next.load(std::memory_order_acquire);
    }

    return false;
}

}  // namespace

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
        block_time_ns_.load(std::memory_order_relaxed),
    };
}

PushStatus LockFreeQueue::Push(Message message) {
    if (is_closed_.load(std::memory_order_acquire)) {
        failed_count_.fetch_add(1, std::memory_order_relaxed);
        return PushStatus::kError;
    }

    auto* new_node = new Node(std::move(message));
    auto& tail_hazard = GetHazardRecordForCurrentThread().GetPointer(0);

    while (true) {
        if (is_closed_.load(std::memory_order_acquire)) {
            tail_hazard.store(nullptr, std::memory_order_release);
            delete new_node;
            failed_count_.fetch_add(1, std::memory_order_relaxed);
            return PushStatus::kError;
        }

        Node* tail = nullptr;
        do {
            tail = tail_.load(std::memory_order_acquire);
            tail_hazard.store(tail, std::memory_order_release);
        } while (tail != tail_.load(std::memory_order_acquire));

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
                tail_hazard.store(nullptr, std::memory_order_release);
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
    auto& hazard_record = GetHazardRecordForCurrentThread();
    auto& head_hazard = hazard_record.GetPointer(0);
    auto& next_hazard = hazard_record.GetPointer(1);

    while (true) {
        Node* head = nullptr;
        do {
            head = head_.load(std::memory_order_acquire);
            head_hazard.store(head, std::memory_order_release);
        } while (head != head_.load(std::memory_order_acquire));

        Node* tail = tail_.load(std::memory_order_acquire);
        Node* next = nullptr;
        do {
            next = head->next.load(std::memory_order_acquire);
            next_hazard.store(next, std::memory_order_release);
        } while (next != head->next.load(std::memory_order_acquire));

        if (head != head_.load(std::memory_order_acquire)) {
            continue;
        }

        if (next == nullptr) {
            head_hazard.store(nullptr, std::memory_order_release);
            next_hazard.store(nullptr, std::memory_order_release);
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

        Message message = next->message;

        if (head_.compare_exchange_weak(
                head,
                next,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            pop_count_.fetch_add(1, std::memory_order_relaxed);
            size_.fetch_sub(1, std::memory_order_relaxed);
            head_hazard.store(nullptr, std::memory_order_release);
            next_hazard.store(nullptr, std::memory_order_release);
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

        auto latency = MeasureExecutionTime([&]() {
            wake_sequence_.wait(observed_wake_sequence, std::memory_order_acquire);
        });
        block_time_ns_.fetch_add(latency, std::memory_order_relaxed);
    }
}

void LockFreeQueue::Close() {
    is_closed_.store(true, std::memory_order_release);
    wake_sequence_.fetch_add(1, std::memory_order_release);
    wake_sequence_.notify_all();
}

void LockFreeQueue::RetireNode(Node* node) noexcept {
    PushRetiredNode(node);
    ScanRetiredNodes();
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

void LockFreeQueue::ScanRetiredNodes() noexcept {
    Node* retired_node = retired_head_.exchange(nullptr, std::memory_order_acq_rel);

    while (retired_node != nullptr) {
        Node* next = retired_node->retired_next;
        retired_node->retired_next = nullptr;

        if (IsNodeHazard(retired_node)) {
            PushRetiredNode(retired_node);
        } else {
            delete retired_node;
        }

        retired_node = next;
    }
}

void LockFreeQueue::PushRetiredNode(Node* node) noexcept {
    Node* retired_head = retired_head_.load(std::memory_order_relaxed);

    do {
        node->retired_next = retired_head;
    } while (!retired_head_.compare_exchange_weak(
        retired_head,
        node,
        std::memory_order_release,
        std::memory_order_relaxed));
}

bool LockFreeQueue::IsNodeHazard(Node* node) noexcept {
    return IsHazardPointer(node);
}
