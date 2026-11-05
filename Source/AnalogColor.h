#pragma once
// Phase 3B: four distinct, original colour transfer models.
// Real-time safe: fixed storage, no allocation or locks in process().
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace masterlab {

enum class AnalogColorMode : int { Tube=0, Tape=1, Transformer=2, SolidState=3 };

struct AnalogColorSettings {
    bool enabled=false;
    AnalogColorMode mode=AnalogColorMode::Tube;
    float driveDb=0.f, bias=0.f, tone=0.f, dynamics=0.f, mix=1.f, outputDb=0.f;
    bool autoGain=true;
};
struct AnalogColorReading {
    float inputRms=0.f, outputRms=0.f, deltaRms=0.f, dcEstimate=0.f;
};

class AnalogColor {
    struct Slew {
        float value=0.f, coefficient=.001f;
        void prepare(double sr,float initial)noexcept {
            value=initial;coefficient=static_cast<float>(1.-std::exp(-1./(std::max(8000.,sr)*.02)));
        }
        float tick(float v)noexcept {value+=coefficient*(v-value);return value;}
    };
    struct Channel {
        float dc=0.f, tapeMemory=0.f, tapeLow=0.f, transformerMemory=0.f;
        float toneLow=0.f, envelope=0.f;
    };
    std::array<Channel,2> channels_{};
    Slew bypass_{},drive_{},bias_{},tone_{},dynamics_{},mix_{},output_{};
    AnalogColorSettings settings_{};
    AnalogColorReading reading_{};
    double rate_=48000.;
    static float clamp(float x,float lo,float hi)noexcept{
        return std::clamp(std::isfinite(x)?x:lo,lo,hi);
    }
    static float softOdd(float v)noexcept {
        return std::tanh(v);
    }
public:
    void settings(AnalogColorSettings value)noexcept {settings_=value;}
    void prepare(double sr)noexcept {
        rate_=std::isfinite(sr)&&sr>=8000&&sr<=384000?sr:48000.;
        reset();
    }
    void reset()noexcept {
        channels_={};reading_={};
        bypass_.prepare(rate_,settings_.enabled?1.f:0.f);
        drive_.prepare(rate_,clamp(settings_.driveDb,0.f,30.f));
        bias_.prepare(rate_,clamp(settings_.bias,-1.f,1.f));
        tone_.prepare(rate_,clamp(settings_.tone,-1.f,1.f));
        dynamics_.prepare(rate_,clamp(settings_.dynamics,0.f,1.f));
        mix_.prepare(rate_,clamp(settings_.mix,0.f,1.f));
        output_.prepare(rate_,clamp(settings_.outputDb,-18.f,12.f));
    }
    AnalogColorReading reading()const noexcept {return reading_;}
    void process(float* left,float* right,int count)noexcept {
        if(!left||!right||count<=0)return;
        if(!settings_.enabled&&bypass_.value<1e-6f){reading_={};return;}
        const float dcStep=static_cast<float>(1.-std::exp(-6.283185307179586*12./rate_));
        const float toneStep=static_cast<float>(1.-std::exp(-6.283185307179586*2800./rate_));
        const float tapeStep=static_cast<float>(1.-std::exp(-6.283185307179586*6000./rate_));
        const float memoryStep=static_cast<float>(1.-std::exp(-1./(rate_*.003)));
        const float envelopeStep=static_cast<float>(1.-std::exp(-1./(rate_*.03)));
        const int mode=std::clamp(static_cast<int>(settings_.mode),0,3);
        double energyIn=0.,energyOut=0.,energyDelta=0.,dc=0.;
        for(int i=0;i<count;++i){
            const float bypass=bypass_.tick(settings_.enabled?1.f:0.f);
            const float db=drive_.tick(clamp(settings_.driveDb,0.f,30.f));
            const float drive=std::pow(10.f,db*.05f);
            const float bias=bias_.tick(clamp(settings_.bias,-1.f,1.f));
            const float tone=tone_.tick(clamp(settings_.tone,-1.f,1.f));
            const float dynamics=dynamics_.tick(clamp(settings_.dynamics,0.f,1.f));
            const float mix=mix_.tick(clamp(settings_.mix,0.f,1.f));
            const float output=std::pow(10.f,output_.tick(clamp(settings_.outputDb,-18.f,12.f))*.05f);
            for(int c=0;c<2;++c){
                float& io=(c==0?left[i]:right[i]);
                const float input=clamp(io,-8.f,8.f);
                auto& s=channels_[c];
                s.envelope+=envelopeStep*(std::abs(input)-s.envelope);
                const float compression=1.f/(1.f+dynamics*2.5f*s.envelope);
                const float v=clamp(input*drive*compression,-32.f,32.f);
                float shaped=0.f;
                switch(mode){
                    case 0: { // Tube: asymmetric smooth transfer with bias; subtract zero-input offset.
                        const float b=bias*.65f;
                        shaped=(softOdd(v+b)-softOdd(b));
                        break;
                    }
                    case 1: { // Tape-inspired: adaptive soft limiting with low-pass and magnetic memory.
                        s.tapeMemory+=memoryStep*(v-s.tapeMemory);
                        const float memory=.15f*s.tapeMemory+dynamics*.3f*s.tapeMemory;
                        const float magnet=softOdd(.8f*(v+memory+bias*.2f))-softOdd(.8f*bias*.2f);
                        s.tapeLow+=tapeStep*(magnet-s.tapeLow);
                        shaped=(1.f-.55f*dynamics)*s.tapeLow+.55f*dynamics*magnet;
                        break;
                    }
                    case 2: { // Transformer-inspired: low-frequency memory and odd soft knee.
                        s.transformerMemory+=memoryStep*.3f*(v-s.transformerMemory);
                        const float core=clamp(v-.18f*dynamics*s.transformerMemory,-32.f,32.f);
                        const float b=bias*.15f;
                        shaped=softOdd(1.5f*(core+b))-softOdd(1.5f*b);
                        break;
                    }
                    default: { // Solid state: stiff piecewise transfer and controlled cubic transition.
                        const float b=bias*.25f;
                        const auto transfer=[](float x)noexcept{
                            const float a=std::abs(x);
                            return x/(1.f+.18f*a+.24f*a*a);
                        };
                        shaped=transfer(v+b)-transfer(b);
                        break;
                    }
                }
                // Tone is a continuous wet-only tilt based on a real first-order split.
                s.toneLow+=toneStep*(shaped-s.toneLow);
                shaped+=tone*.65f*(shaped-2.f*s.toneLow);
                // High pass/DC blocker removes any asymmetry-produced DC.
                s.dc+=dcStep*(shaped-s.dc);
                const float colored=clamp(shaped-s.dc,-12.f,12.f);
                const float gainComp=settings_.autoGain?1.f/std::sqrt(std::max(1.f,drive)):1.f;
                const float processed=clamp(colored*gainComp*output,-16.f,16.f);
                const float blend=clamp(bypass*mix,0.f,1.f);
                const float y=clamp(input+blend*(processed-input),-16.f,16.f);
                io=y;
                const double d=static_cast<double>(y)-input;
                energyIn+=static_cast<double>(input)*input;
                energyOut+=static_cast<double>(y)*y;
                energyDelta+=d*d;
                dc+=s.dc;
            }
        }
        const double inv=1./(2.*count);
        reading_={static_cast<float>(std::sqrt(energyIn*inv)),
                  static_cast<float>(std::sqrt(energyOut*inv)),
                  static_cast<float>(std::sqrt(energyDelta*inv)),
                  static_cast<float>(dc*inv)};
    }
};
}
