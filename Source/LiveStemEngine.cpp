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
    toneCoefficient_ = static_cast<float>(1.0-std::exp(-2.0*3.141592653589793*1800.0/rate));
    // Both queue owners must be stopped here; never do this in reset/process.
    StereoLiveFrame in{}; while (input_.tryPop(in)) {}
    SeparatedLiveFrame out{}; while (output_.tryPop(out)) {}
    droppedInput_.store(0); missingFrames_.store(0); invalidFrames_.store(0); staleFrames_.store(0); deliveredWetFrames_.store(0);
    reset();
    return true;
}

void LiveStemEngine::newEpoch() noexcept {
    ++epoch_; position_ = 0; dryAt_ = 0;
    wetMix_ = 0; lastCorrection_.fill(0); recoveryHold_.fill(0); starving_ = false; fadeRemaining_ = 0;
    havePending_ = false;
    for(auto& pair:toneLowpass_)pair.fill(0.f);
    smoothedGain_.fill(1.f);
    publishedPlaybackPosition_.store(0,std::memory_order_release);
    publishedEpoch_.store(epoch_,std::memory_order_release);
}
void LiveStemEngine::reset() noexcept { newEpoch(); initialized_ = false; }

bool LiveStemEngine::findOutput(std::uint64_t wanted, std::array<float,2>& sum,
                                const LiveStemControls& controls) noexcept {
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
        const bool anySolo = std::any_of(controls.solo.begin(), controls.solo.end(),
                                        [](bool b){return b;});
        for (std::size_t stem=0; stem<4; ++stem) {
            if (!std::isfinite(controls.gainLinear[stem]) ||
                !std::isfinite(controls.tone[stem]) ||
                !std::isfinite(controls.width[stem]) ||
                controls.gainLinear[stem]<0.f || controls.gainLinear[stem]>8.f ||
                std::abs(controls.tone[stem])>1.f ||
                controls.width[stem]<0.f || controls.width[stem]>2.f) {
                invalidFrames_.fetch_add(1, std::memory_order_relaxed);return false;
            }
            float stereo[2];
            for (std::size_t channel=0;channel<2;++channel) {
                const float input=pending_.channels[stem*2+channel];
                if (!std::isfinite(input)) {
                    invalidFrames_.fetch_add(1,std::memory_order_relaxed);return false;
                }
                auto& smooth=toneLowpass_[stem][channel];
                smooth+=toneCoefficient_*(input-smooth);
                // Equal-and-opposite low/high gain gives a true, smoother
                // bipolar tilt (up to +/-4 dB per side) without the old
                // doubling of treble energy or full low-pass at -100%.
                stereo[channel]=smooth*toneLowGain_[stem]+
                                (input-smooth)*toneHighGain_[stem];
            }
            // Smooth gain changes over ~15 ms at 44.1 kHz rather than
            // generating a click when the host automates a fader or mute.
            const bool audible=!controls.mute[stem]&&(!anySolo||controls.solo[stem]);
            const float target=audible?controls.gainLinear[stem]:0.f;
            smoothedGain_[stem]+=(target-smoothedGain_[stem])*.0015f;
            const double mid=(static_cast<double>(stereo[0])+stereo[1])*.5;
            const double side=(static_cast<double>(stereo[0])-stereo[1])*.5*controls.width[stem];
            total[0]+=(mid+side)*smoothedGain_[stem];
            total[1]+=(mid-side)*smoothedGain_[stem];
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

void LiveStemEngine::process(float* left, float* right, std::size_t count,
                             LiveStemTransport t, const LiveStemControls& controls) noexcept {
    if (!dry_ || !left || count == 0) return;
    // Make the render mode visible before any queued hop can reach the worker.
    // Offline exports must never lose inference hops just because they are
    // processed faster than wall-clock playback.
    publishedNonRealtime_.store(t.nonRealtime,std::memory_order_release);
    const bool active = t.playing || t.nonRealtime;
    if(initialized_ && wasRemixEnabled_!=controls.enabled) newEpoch();
    wasRemixEnabled_=controls.enabled;
    if (initialized_ && (active != wasActive_ || t.nonRealtime != wasNonRealtime_ ||
        t.hasPosition != hadPosition_ || (active && t.hasPosition && t.samplePosition != expectedHost_)))
        newEpoch();
    initialized_ = true; wasActive_ = active; hadPosition_ = t.hasPosition; wasNonRealtime_ = t.nonRealtime;
    // Saturate host addition rather than overflowing signed time on malformed host data.
    const auto room = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) -
                      static_cast<std::uint64_t>(t.samplePosition);
    expectedHost_ = count > room ? std::numeric_limits<std::int64_t>::max() :
        static_cast<std::int64_t>(static_cast<std::uint64_t>(t.samplePosition) + count);
    // One cheap coefficient update per block rather than per sample.
    for (std::size_t stem=0;stem<4;++stem) {
        const float tone=std::clamp(std::isfinite(controls.tone[stem])?
                                    controls.tone[stem]:0.f,-1.f,1.f);
        toneLowGain_[stem]=std::pow(10.f,-4.f*tone/20.f);
        toneHighGain_[stem]=std::pow(10.f,4.f*tone/20.f);
    }
    bool enqueued = false;
    for (std::size_t i = 0; i < count; ++i, ++position_) {
        const float l = std::isfinite(left[i]) ? left[i] : 0.f;
        const float r = right ? (std::isfinite(right[i]) ? right[i] : 0.f) : l;
        if (active && controls.enabled) {
            if (input_.tryPush({l,r,position_,epoch_})) enqueued = true;
            else droppedInput_.fetch_add(1, std::memory_order_relaxed);
        }
        dry_[dryAt_] = {l,r,position_,epoch_};
        const auto read = (dryAt_ + 1) % drySize_;
        const auto delayed = dry_[read];
        dryAt_ = read;
        const bool ready = position_ >= latency_ && delayed.epoch == epoch_ &&
                           delayed.position == position_ - latency_;
        std::array<float,2> dry{ready ? delayed.left : 0.f, ready ? delayed.right : 0.f};
        std::array<float,2> wet{};
        bool available = active && controls.enabled && ready &&
                         findOutput(position_ - latency_, wet, controls);
        if (available) {
            deliveredWetFrames_.fetch_add(1, std::memory_order_relaxed);
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
    // Publish the end of this callback for non-audio worker deadline checks.
    publishedPlaybackPosition_.store(position_,std::memory_order_release);
    // One wakeup per block instead of a 100-us timed sleep in the worker.
    // This is lock-free; its real-time cost must still be measured in CI.
    if (enqueued) wakeInput();
}

LiveStemStatistics LiveStemEngine::statistics() const noexcept {
    return {droppedInput_.load(std::memory_order_relaxed), missingFrames_.load(std::memory_order_relaxed),
            invalidFrames_.load(std::memory_order_relaxed), staleFrames_.load(std::memory_order_relaxed),
            deliveredWetFrames_.load(std::memory_order_relaxed)};
}
} // namespace masterlab
