#pragma once
// Phase 3A experimental DSP core; not yet connected to VST3.
// Realtime processing allocates nothing; optional experimental FIR 2x oversampling.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include "HarmonicOversampler.h"

namespace masterlab {
struct HarmonicExciterSettings {
    bool enabled = false;
    std::array<float,4> amount{0.f,0.f,0.f,0.f};
    float even=0.5f,odd=0.5f,driveDb=0.f,mix=1.f,outputDb=0.f,impact=0.f;
    bool oversample2x=false; // experimental non-automatable mode; adds 32 samples of latency
    std::array<float,3> crossovers{180.f,2400.f,8000.f};
};
struct HarmonicExciterReading {
    std::array<float,4> generatedRms{};
    float inputRms=0.f, outputRms=0.f;
};
class HarmonicExciter {
    struct FilterState {
        float a=0.f,b=0.f,c=0.f;
        std::array<float,4> dc{};
    } state_[2]{};
    struct ParameterSmoother {
        float value=0.f,coefficient=.001f;
        void prepare(double sampleRate,float initial) noexcept {
            coefficient=static_cast<float>(1.-std::exp(-1./(sampleRate*.02)));
            value=initial;
        }
        float tick(float target) noexcept {value+=coefficient*(target-value);return value;}
    };
    ParameterSmoother driveSmoother_,evenSmoother_,oddSmoother_,mixSmoother_,outputSmoother_,bypassSmoother_,impactSmoother_;
    std::array<ParameterSmoother,4> bandSmoothers_{};
    HarmonicExciterSettings settings_{};
    HarmonicExciterReading reading_{};
    std::array<std::array<HarmonicOversampler2x,4>,2> oversamplers_{};
    std::array<std::array<float,HarmonicOversampler2x::latencySamples>,2> dryDelay_{};
    size_t dryPosition_=0;
    double sampleRate_=48000.;
    std::array<float,3> coefficient_{};
    static float finiteClamp(float x,float lo,float hi) noexcept {
        return std::clamp(std::isfinite(x)?x:lo,lo,hi);
    }
    void updateCoefficients() noexcept {
        auto hz=settings_.crossovers;
        hz[0]=finiteClamp(hz[0],40.f,500.f);
        hz[1]=std::max(hz[0]*2.f,finiteClamp(hz[1],600.f,5000.f));
        hz[2]=std::max(hz[1]*1.5f,finiteClamp(hz[2],5500.f,14000.f));
        for(size_t i=0;i<3;++i)
            coefficient_[i]=static_cast<float>(1.-std::exp(-6.283185307179586*hz[i]/sampleRate_));
    }
public:
    void prepare(double sampleRate) noexcept {
        sampleRate_=std::isfinite(sampleRate)&&sampleRate>=8000.&&sampleRate<=384000.?sampleRate:48000.;
        updateCoefficients();reset();
    }
    void reset() noexcept {
        for(auto& s:state_)s={};
        for(auto& channel:oversamplers_)for(auto& filter:channel)filter.reset();
        dryDelay_={};dryPosition_=0;
        bypassSmoother_.prepare(sampleRate_,settings_.enabled?1.f:0.f);
        impactSmoother_.prepare(sampleRate_,finiteClamp(settings_.impact,0.f,1.f));
        driveSmoother_.prepare(sampleRate_,finiteClamp(settings_.driveDb,0.f,24.f));
        evenSmoother_.prepare(sampleRate_,finiteClamp(settings_.even,0.f,1.f));
        oddSmoother_.prepare(sampleRate_,finiteClamp(settings_.odd,0.f,1.f));
        mixSmoother_.prepare(sampleRate_,finiteClamp(settings_.mix,0.f,1.f));
        outputSmoother_.prepare(sampleRate_,finiteClamp(settings_.outputDb,-18.f,6.f));
        for(size_t k=0;k<4;++k)bandSmoothers_[k].prepare(sampleRate_,finiteClamp(settings_.amount[k],0.f,1.f));
        reading_={};
    }
    // Call settings() from control thread / before rendering; not per sample.
    // Changing oversample2x mid-stream is not click-free; not yet host exposed.
    void settings(HarmonicExciterSettings value) noexcept {
        settings_=value;updateCoefficients();
    }
    HarmonicExciterReading reading()const noexcept {return reading_;}
    int latencySamples()const noexcept {return settings_.enabled&&settings_.oversample2x?HarmonicOversampler2x::latencySamples:0;}
    void process(float* left,float* right,int count,bool delta=false) noexcept {
        if(!left||!right||count<=0)return;
        if(!settings_.enabled && bypassSmoother_.value<1e-6f){
            reading_={};
            if(delta)for(int i=0;i<count;++i)left[i]=right[i]=0.f;
            return;
        }
        const bool oversampled=settings_.oversample2x;
        const float dcAlpha=static_cast<float>(1.-std::exp(-6.283185307179586*12./(sampleRate_*(oversampled?2.:1.))));
        std::array<double,4> generatedSquare{};
        double inSquare=0.,outSquare=0.;
        for(int i=0;i<count;++i){
            const float bypass=bypassSmoother_.tick(settings_.enabled?1.f:0.f);
            const float drive=std::pow(10.f,driveSmoother_.tick(finiteClamp(settings_.driveDb,0.f,24.f))/20.f);
            const float even=evenSmoother_.tick(finiteClamp(settings_.even,0.f,1.f));
            const float odd=oddSmoother_.tick(finiteClamp(settings_.odd,0.f,1.f));
            const float wet=mixSmoother_.tick(finiteClamp(settings_.mix,0.f,1.f));
            const float authority=1.f+5.f*impactSmoother_.tick(finiteClamp(settings_.impact,0.f,1.f));
            const float output=std::pow(10.f,outputSmoother_.tick(finiteClamp(settings_.outputDb,-18.f,6.f))/20.f);
            std::array<float,4> intensities{};
            for(size_t k=0;k<4;++k)intensities[k]=bandSmoothers_[k].tick(finiteClamp(settings_.amount[k],0.f,1.f));
            for(int channel=0;channel<2;++channel){
                float& io=channel==0?left[i]:right[i];
                const float input=std::isfinite(io)?std::clamp(io,-8.f,8.f):0.f;
                auto& s=state_[channel];
                s.a+=coefficient_[0]*(input-s.a);
                s.b+=coefficient_[1]*(input-s.b);
                s.c+=coefficient_[2]*(input-s.c);
                const std::array<float,4> band{s.a,s.b-s.a,s.c-s.b,input-s.c};
                float addition=0.f;
                for(size_t k=0;k<4;++k){
                    const float intensity=intensities[k];
                    const auto nonlinear=[&](float v) noexcept -> float {
                        const float x=std::tanh(v*drive);
                        const float evenRaw=x*x;
                        s.dc[k]+=dcAlpha*(evenRaw-s.dc[k]);
                        const float second=evenRaw-s.dc[k];
                        const float third=x*x*x;
                        return intensity*(0.5f*even*second+0.5f*odd*third);
                    };
                    const float generated=oversampled?
                        oversamplers_[channel][k].tick(band[k],nonlinear):
                        nonlinear(band[k]);
                    addition+=generated;
                    generatedSquare[k]+=static_cast<double>(generated)*generated;
                }
                float dry=input;
                if(oversampled){
                    dry=dryDelay_[channel][dryPosition_];
                    dryDelay_[channel][dryPosition_]=input;
                }
                const float processed=std::clamp((dry+wet*authority*addition)*output,-16.f,16.f);
                const float normal=dry+bypass*(processed-dry);
                const float result=delta?normal-dry:normal;
                io=std::isfinite(result)?result:0.f;
                inSquare+=static_cast<double>(input)*input;
                outSquare+=static_cast<double>(normal)*normal;
            }
            if(oversampled)dryPosition_=(dryPosition_+1)%dryDelay_[0].size();
        }
        const double denominator=static_cast<double>(count)*2.;
        reading_.inputRms=static_cast<float>(std::sqrt(inSquare/denominator));
        reading_.outputRms=static_cast<float>(std::sqrt(outSquare/denominator));
        for(size_t k=0;k<4;++k)
            reading_.generatedRms[k]=static_cast<float>(std::sqrt(generatedSquare[k]/denominator));
    }
};
}
