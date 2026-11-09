#pragma once
// SPSC, fixed-capacity frame transfer for future live stem separation.
// Construct off the audio thread; tryPush/tryPop never allocate, wait, or lock.
// Exactly one producer and one consumer per queue. Transport discontinuities
// are represented by epoch/position metadata, not by concurrent queue reset.
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace masterlab {
struct StereoLiveFrame {
    float left{};
    float right{};
    std::uint64_t position{};
    std::uint32_t epoch{};
};
struct SeparatedLiveFrame {
    // [drums L/R, bass L/R, other L/R, vocals L/R]
    std::array<float, 8> channels{};
    std::uint64_t position{};
    std::uint32_t epoch{};
};

template <typename Frame, std::size_t Capacity>
class LiveStemQueue final {
    static_assert(Capacity >= 2, "Capacity must be at least two frames");
    static_assert(std::is_trivially_copyable_v<Frame>, "Frame must be trivially copyable");
    // Allocation happens once, during construction on the control thread.
    std::unique_ptr<Frame[]> frames_{std::make_unique<Frame[]>(Capacity)};
    alignas(64) std::atomic<std::uint64_t> head_{0}; // written by producer
    alignas(64) std::atomic<std::uint64_t> tail_{0}; // written by consumer
public:
    LiveStemQueue() = default;
    LiveStemQueue(const LiveStemQueue&) = delete;
    LiveStemQueue& operator=(const LiveStemQueue&) = delete;

    bool tryPush(const Frame& frame) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);
        if (head - tail >= Capacity) return false;
        frames_[head % Capacity] = frame;
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    bool tryPop(Frame& frame) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire);
        if (head == tail) return false;
        frame = frames_[tail % Capacity];
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    std::size_t size() const noexcept {
        const auto head = head_.load(std::memory_order_acquire);
        const auto tail = tail_.load(std::memory_order_acquire);
        return static_cast<std::size_t>(head - tail);
    }

    bool empty() const noexcept { return size() == 0; }
    static constexpr std::size_t capacity() noexcept { return Capacity; }
};
} // namespace masterlab
