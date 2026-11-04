#pragma once
// Phase 3A experimental DSP core; not yet connected to VST3.
// Realtime processing allocates nothing; oversampling/anti-aliasing pending.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace masterlab {
struct HarmonicExciterSettings {
    bool enabled = false;
    std::array<float,4> amount{0.f,0.f,0.f,0.f};
    float even=0.5f,odd=0.5f,driveDb=0.f,mix=1.f,outputDb=0.f;
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
    HarmonicExciterSettings settings_{};
    HarmonicExciterReading reading_{};
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
        reading_={};
    }
    // Call settings() from control thread / before rendering; not per sample.
    void settings(HarmonicExciterSettings value) noexcept {
        settings_=value;updateCoefficients();
    }
    HarmonicExciterReading reading()const noexcept {return reading_;}
    void process(float* left,float* right,int count,bool delta=false) noexcept {
        if(!left||!right||count<=0)return;
        if(!settings_.enabled){
            reading_={};
            if(delta)for(int i=0;i<count;++i)left[i]=right[i]=0.f;
            return;
        }
        const float drive=std::pow(10.f,finiteClamp(settings_.driveDb,0.f,24.f)/20.f);
        const float even=finiteClamp(settings_.even,0.f,1.f);
        const float odd=finiteClamp(settings_.odd,0.f,1.f);
        const float wet=finiteClamp(settings_.mix,0.f,1.f);
        const float output=std::pow(10.f,finiteClamp(settings_.outputDb,-18.f,6.f)/20.f);
        const float dcAlpha=static_cast<float>(1.-std::exp(-6.283185307179586*12./sampleRate_));
        std::array<double,4> generatedSquare{};
        double inSquare=0.,outSquare=0.;
        for(int i=0;i<count;++i){
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
                    const float x=std::tanh(band[k]*drive);
                    const float evenRaw=x*x;
                    s.dc[k]+=dcAlpha*(evenRaw-s.dc[k]);
                    const float second=evenRaw-s.dc[k];
                    const float third=x*x*x;
                    const float intensity=finiteClamp(settings_.amount[k],0.f,1.f);
                    const float generated=intensity*(0.5f*even*second+0.5f*odd*third);
                    addition+=generated;
                    generatedSquare[k]+=static_cast<double>(generated)*generated;
                }
                const float normal=std::clamp((input+wet*addition)*output,-16.f,16.f);
                const float result=delta?normal-input:normal;
                io=std::isfinite(result)?result:0.f;
                inSquare+=static_cast<double>(input)*input;
                outSquare+=static_cast<double>(normal)*normal;
            }
        }
        const double denominator=static_cast<double>(count)*2.;
        reading_.inputRms=static_cast<float>(std::sqrt(inSquare/denominator));
        reading_.outputRms=static_cast<float>(std::sqrt(outSquare/denominator));
        for(size_t k=0;k<4;++k)
            reading_.generatedRms[k]=static_cast<float>(std::sqrt(generatedSquare[k]/denominator));
    }
};
}
