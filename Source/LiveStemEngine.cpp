#include "LiveStemEngine.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace masterlab {
static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
              "The live callback requires lock-free 64-bit atomics");

bool LiveStemEngine::prepare(double rate, std::size_t latency, std::size_t fade) {
    if (!std::isfinite(rate) || rate < 8000 || rate > 384000 || latency >= queueCapacity || fade == 0)
        return false;
    auto dry = std::make_unique<DryFrame[]>(latency + 1);
    dry_ = std::move(dry);
    latency_ = latency; fade_ = fade; drySize_ = latency + 1;
    // Both queue owners must be stopped here; never do this in reset/process.
    StereoLiveFrame in{}; while (input_.tryPop(in)) {}
    SeparatedLiveFrame out{}; while (output_.tryPop(out)) {}
    droppedInput_.store(0); missingFrames_.store(0); invalidFrames_.store(0); staleFrames_.store(0);
    reset();
    return true;
}

void LiveStemEngine::newEpoch() noexcept {
    ++epoch_; position_ = 0; dryAt_ = 0;
    wetMix_ = 0; lastCorrection_.fill(0); recoveryHold_.fill(0); starving_ = false; fadeRemaining_ = 0;
    havePending_ = false;
    publishedEpoch_.store(epoch_,std::memory_order_release);
}
void LiveStemEngine::reset() noexcept { newEpoch(); initialized_ = false; }

bool LiveStemEngine::findOutput(std::uint64_t wanted, std::array<float,2>& sum) noexcept {
    // Fixed work budget per sample, even when a worker returns a stale backlog.
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (!havePending_) havePending_ = output_.tryPop(pending_);
        if (!havePending_) return false;
        if (pending_.epoch != epoch_ || pending_.position < wanted) {
            havePending_ = false; staleFrames_.fetch_add(1, std::memory_order_relaxed); continue;
        }
        // The worker cannot have seen input beyond the current stream position.
        // Reject corrupt timestamps instead of hiding all later valid results.
        if (pending_.position > position_) {
            havePending_=false; invalidFrames_.fetch_add(1,std::memory_order_relaxed); continue;
        }
        if (pending_.position > wanted) return false;
        havePending_ = false;
        std::array<double,2> total{};
        for (std::size_t c = 0; c < 8; ++c) {
            if (!std::isfinite(pending_.channels[c])) {
                invalidFrames_.fetch_add(1, std::memory_order_relaxed); return false;
            }
            total[c % 2] += pending_.channels[c];
        }
        for (std::size_t c = 0; c < 2; ++c) {
            if (std::abs(total[c]) > std::numeric_limits<float>::max()) {
                invalidFrames_.fetch_add(1, std::memory_order_relaxed); return false;
            }
            sum[c] = static_cast<float>(total[c]);
        }
        return true;
    }
    return false;
}

void LiveStemEngine::process(float* left, float* right, std::size_t count, LiveStemTransport t) noexcept {
    if (!dry_ || !left || count == 0) return;
    const bool active = t.playing || t.nonRealtime;
    if (initialized_ && (active != wasActive_ || t.nonRealtime != wasNonRealtime_ ||
        t.hasPosition != hadPosition_ || (active && t.hasPosition && t.samplePosition != expectedHost_)))
        newEpoch();
    initialized_ = true; wasActive_ = active; hadPosition_ = t.hasPosition; wasNonRealtime_ = t.nonRealtime;
    // Saturate host addition rather than overflowing signed time on malformed host data.
    const auto room = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) -
                      static_cast<std::uint64_t>(t.samplePosition);
    expectedHost_ = count > room ? std::numeric_limits<std::int64_t>::max() :
        static_cast<std::int64_t>(static_cast<std::uint64_t>(t.samplePosition) + count);
    for (std::size_t i = 0; i < count; ++i, ++position_) {
        const float l = std::isfinite(left[i]) ? left[i] : 0.f;
        const float r = right ? (std::isfinite(right[i]) ? right[i] : 0.f) : l;
        if (active && !input_.tryPush({l,r,position_,epoch_}))
            droppedInput_.fetch_add(1, std::memory_order_relaxed);
        dry_[dryAt_] = {l,r,position_,epoch_};
        const auto read = (dryAt_ + 1) % drySize_;
        const auto delayed = dry_[read];
        dryAt_ = read;
        const bool ready = position_ >= latency_ && delayed.epoch == epoch_ &&
                           delayed.position == position_ - latency_;
        std::array<float,2> dry{ready ? delayed.left : 0.f, ready ? delayed.right : 0.f};
        std::array<float,2> wet{};
        bool available = active && ready && findOutput(position_ - latency_, wet);
        if (available) {
            for (std::size_t c=0;c<2;++c)
                if (!std::isfinite(wet[c]-dry[c])) available=false;
        }
        if (available && (dry[0] != 0 || dry[1] != 0)) {
            if (starving_) {
                const auto remaining = static_cast<float>(fadeRemaining_)/static_cast<float>(fade_);
                for (std::size_t c=0;c<2;++c) recoveryHold_[c] = lastCorrection_[c]*remaining;
                wetMix_ = 0;
            }
            starving_ = false;
            wetMix_ = std::min(1.f, wetMix_ + 1.f/static_cast<float>(fade_));
            for (std::size_t c=0;c<2;++c)
                lastCorrection_[c] = (1-wetMix_)*recoveryHold_[c] + wetMix_*(wet[c]-dry[c]);
            left[i] = dry[0] + lastCorrection_[0];
            if (right) right[i] = dry[1] + lastCorrection_[1];
        } else {
            if (active && ready && !available) missingFrames_.fetch_add(1, std::memory_order_relaxed);
            if (!starving_) { starving_=true; fadeRemaining_=fade_; }
            if (fadeRemaining_ > 0) --fadeRemaining_;
            // Exact digital silence must not expose hallucinated/held neural content.
            const float fade = (dry[0] == 0 && dry[1] == 0) ? 0.f :
                static_cast<float>(fadeRemaining_)/static_cast<float>(fade_);
            left[i] = dry[0] + fade*lastCorrection_[0];
            if (right) right[i] = dry[1] + fade*lastCorrection_[1];
        }
        // A held correction can overflow when dry suddenly changes polarity or
        // amplitude, even though both the model and host supplied finite floats.
        if (!std::isfinite(left[i]) || (right && !std::isfinite(right[i]))) {
            left[i]=dry[0]; if(right) right[i]=dry[1];
            lastCorrection_.fill(0); recoveryHold_.fill(0); wetMix_=0;
            starving_=true; fadeRemaining_=0;
            invalidFrames_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

LiveStemStatistics LiveStemEngine::statistics() const noexcept {
    return {droppedInput_.load(std::memory_order_relaxed), missingFrames_.load(std::memory_order_relaxed),
            invalidFrames_.load(std::memory_order_relaxed), staleFrames_.load(std::memory_order_relaxed)};
}
} // namespace masterlab
