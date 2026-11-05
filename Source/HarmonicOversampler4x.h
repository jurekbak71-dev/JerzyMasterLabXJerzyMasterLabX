#pragma once
// Experimental two-stage 4x oversampling. Do not expose to the VST host
// before fixed worst-case PDC and dry-path latency compensation are designed.
#include "HarmonicOversampler.h"
namespace masterlab {
class HarmonicOversampler4x {
    HarmonicOversampler2x outer_,inner_;
public:
    // 32 samples from the outer 2x stage plus 32 high-rate samples from
    // inner stage (16 original-rate samples). Total 48 base-rate samples.
    static constexpr int latencySamples=48;
    void prepare() noexcept {outer_.prepare();inner_.prepare();}
    void reset() noexcept {outer_.reset();inner_.reset();}
    template<class Nonlinear>
    float tick(float x,Nonlinear&& shape)noexcept {
        return outer_.tick(x,[&](float highRateSample)noexcept {
            return inner_.tick(highRateSample,shape);
        });
    }
};
}
