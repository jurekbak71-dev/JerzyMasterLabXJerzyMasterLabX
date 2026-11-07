#pragma once
#include "LiveStemQueue.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace masterlab {
struct LiveStemTransport {
    std::int64_t samplePosition{}; // signed DAW time, including preroll
    bool playing{true};
    bool nonRealtime{};
    bool hasPosition{};
};
struct LiveStemStatistics {
    std::uint64_t droppedInput{}, missingFrames{}, invalidFrames{}, staleFrames{};
};

// Callback boundary only. This class is NOT a neural separator.
// One audio thread calls process/reset, one worker calls popInput/pushOutput.
// Worker outputs must retain input epoch/position, in increasing input order.
// Gaps are allowed; future outputs within the configured delay are retained.
// prepare/destruction require BOTH threads quiescent. reset is audio-thread-only
// and invalidates generations without concurrently resetting either SPSC queue.
class LiveStemEngine final {
public:
    static constexpr std::size_t queueCapacity = 65536;
    bool prepare(double sampleRate, std::size_t latencySamples, std::size_t fadeSamples);
    void reset() noexcept;
    void process(float* left, float* right, std::size_t samples, LiveStemTransport transport) noexcept;
    bool popInput(StereoLiveFrame& frame) noexcept { return input_.tryPop(frame); }
    bool pushOutput(const SeparatedLiveFrame& frame) noexcept { return output_.tryPush(frame); }
    LiveStemStatistics statistics() const noexcept;
    std::size_t latencySamples() const noexcept { return latency_; }
private:
    struct DryFrame { float left{}, right{}; std::uint64_t position{}; std::uint32_t epoch{}; };
    LiveStemQueue<StereoLiveFrame, queueCapacity> input_;
    LiveStemQueue<SeparatedLiveFrame, queueCapacity> output_;
    std::unique_ptr<DryFrame[]> dry_;
    std::size_t latency_{}, fade_{1}, drySize_{}, dryAt_{}, fadeRemaining_{};
    std::uint64_t position_{};
    std::uint32_t epoch_{1};
    std::int64_t expectedHost_{};
    bool initialized_{}, wasActive_{}, hadPosition_{}, wasNonRealtime_{};
    bool havePending_{}, starving_{};
    SeparatedLiveFrame pending_{};
    float wetMix_{};
    std::array<float,2> lastCorrection_{};
    std::array<float,2> recoveryHold_{};
    std::atomic<std::uint64_t> droppedInput_{0}, missingFrames_{0}, invalidFrames_{0}, staleFrames_{0};
    void newEpoch() noexcept;
    bool findOutput(std::uint64_t position, std::array<float,2>& sum) noexcept;
};
} // namespace masterlab
