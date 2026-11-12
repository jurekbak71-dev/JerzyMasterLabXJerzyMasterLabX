#pragma once
// Independent fixed-latency tonal colour processor. Not a copy of JerzyTapeDriveVST3.
// Tube/transistor/tape transfer, bounded drive compensation and transient-aware
// de-fizz. No wow, flutter, hiss, noise, dropouts or callback allocations.
#include <algorithm>
#include <array>
#include <cmath>
#include "HarmonicOversampler4x.h"
namespace masterlab {
enum class AnalogizerMode : int { Tube=0, FET=1, Transistor=1, Tape=2 };
struct AnalogizerSettings {
 bool enabled=false;
 AnalogizerMode mode=AnalogizerMode::Tube;
 float driveDb=0.f,warmth=0.f,deFizz=0.f,mix=1.f,outputDb=0.f;
 float bias=0.f,tone=0.f,dynamics=0.f,impact=0.f;
 bool autoGain=true;
};
struct AnalogizerReading {float inputRms=0.f,outputRms=0.f,deltaRms=0.f;};
class Analogizer {
 struct State {float warmthLow=0.f,highLow=0.f,highEnv=0.f,tapeMemory=0.f,dc=0.f,toneLow=0.f,envelope=0.f;};
 std::array<State,2> channels_{};
 std::array<HarmonicOversampler4x,2> oversamplers_{};
 std::array<std::array<float,HarmonicOversampler4x::latencySamples>,2> dryDelay_{};
 size_t delayPosition_=0;
 AnalogizerSettings settings_{};
 AnalogizerReading reading_{};
 double rate_=48000.;
 float bypass_=0.f,driveDb_=0.f,warmth_=0.f,deFizz_=0.f,mix_=1.f,outputDb_=0.f;
 float bias_=0.f,tone_=0.f,dynamics_=0.f,impact_=0.f;
 float smooth_=0.001f;
 static float safe(float x)noexcept {return std::isfinite(x)?std::clamp(x,-8.f,8.f):0.f;}
 static float sat(float x,AnalogizerMode mode,float memory)noexcept {
  if(mode==AnalogizerMode::FET){
   // FET-inspired stiffer asymmetric pinch-off: a deliberately distinct
   // nonlinear transfer, not an alias of the earlier transistor model.
   const float bias=.14f;
   return .70f*(std::atan(2.35f*(x+bias))-std::atan(2.35f*bias))/
          (1.f+.035f*x*x);
  }
  if(mode==AnalogizerMode::Tape)return std::tanh(.85f*(x+.16f*memory));
  return std::tanh(x+.065f)-std::tanh(.065f);
 }
public:
 void settings(AnalogizerSettings s)noexcept {settings_=s;}
 void prepare(double rate)noexcept {
  rate_=std::isfinite(rate)&&rate>=8000.&&rate<=384000.?rate:48000.;
  smooth_=static_cast<float>(1.-std::exp(-1./(rate_*.02)));
  for(auto& oversampler:oversamplers_)oversampler.prepare();reset();
 }
 void reset()noexcept {
  channels_={};dryDelay_={};delayPosition_=0;for(auto& oversampler:oversamplers_)oversampler.reset();
  reading_={};bypass_=settings_.enabled?1.f:0.f;
  driveDb_=settings_.driveDb;warmth_=settings_.warmth;deFizz_=settings_.deFizz;
  mix_=settings_.mix;outputDb_=settings_.outputDb;
  bias_=settings_.bias;tone_=settings_.tone;dynamics_=settings_.dynamics;impact_=settings_.impact;
 }
 AnalogizerReading reading()const noexcept{return reading_;}
 static constexpr int latencySamples()noexcept{return HarmonicOversampler4x::latencySamples;}
 void process(float* left,float* right,int count)noexcept {
  if(!left||!right||count<=0)return;
  const float warmPole=static_cast<float>(1.-std::exp(-6.283185307179586*1850./rate_));
  const float highPole=static_cast<float>(1.-std::exp(-6.283185307179586*5400./rate_));
  const float magneticPole=static_cast<float>(1.-std::exp(-1./(rate_*.003)));
  const float envPole=static_cast<float>(1.-std::exp(-1./(rate_*.008)));
  const float dcPole=static_cast<float>(1.-std::exp(-6.283185307179586*12./rate_));
  const float tonePole=static_cast<float>(1.-std::exp(-6.283185307179586*2800./rate_));
  const float dynamicsPole=static_cast<float>(1.-std::exp(-1./(rate_*.03)));
  double in2=0.,out2=0.,delta2=0.;
  for(int i=0;i<count;++i){
   auto slew=[&](float& current,float target)noexcept {
    current+=smooth_*(target-current);return current;
   };
   const float blend=slew(bypass_,settings_.enabled?1.f:0.f);
   const float drive=std::pow(10.f,.05f*slew(driveDb_,std::clamp(settings_.driveDb,0.f,18.f)));
   const float warmth=slew(warmth_,std::clamp(settings_.warmth,0.f,1.f));
   const float fizz=slew(deFizz_,std::clamp(settings_.deFizz,0.f,1.f));
   const float wet=slew(mix_,std::clamp(settings_.mix,0.f,1.f))*blend;
   const float bias=slew(bias_,std::clamp(settings_.bias,-1.f,1.f));
   const float tone=slew(tone_,std::clamp(settings_.tone,-1.f,1.f));
   const float dynamics=slew(dynamics_,std::clamp(settings_.dynamics,0.f,1.f));
   const float impact=slew(impact_,std::clamp(settings_.impact,0.f,1.f));
   const float output=std::pow(10.f,.05f*slew(outputDb_,std::clamp(settings_.outputDb,-12.f,12.f)));
   for(int ch=0;ch<2;++ch){
    float& out=(ch==0?left[i]:right[i]);const float input=safe(out);
    const float delayedInput=dryDelay_[ch][delayPosition_];dryDelay_[ch][delayPosition_]=input;
    auto& st=channels_[ch];
    st.envelope+=dynamicsPole*(std::abs(input)-st.envelope);
    const float compression=1.f/(1.f+2.5f*dynamics*st.envelope);
    const float x=input*drive*compression;
    st.tapeMemory+=magneticPole*(x-st.tapeMemory);
    // Tube (asymmetric), FET (stiff asymmetric), Tape (magnetic memory).
    const float offset=bias*(settings_.mode==AnalogizerMode::Tube?.65f:.25f);
    const float coloured=oversamplers_[ch].tick(x,[&](float highRateX)noexcept {
     const float saturation=sat(highRateX+offset,settings_.mode,st.tapeMemory)-sat(offset,settings_.mode,0.f);
     const float warmHarmonic=warmth*.32f*std::tanh(3.f*highRateX)*
         std::tanh(3.f*highRateX)*(highRateX>=0.f?1.f:-.22f);
     return saturation+warmHarmonic;
    });
    // Compensation: moderate output growth, not hard clipping of peaks.
    const float compensated=settings_.autoGain?
        coloured/(1.f+.30f*(drive-1.f)):coloured;
    st.warmthLow+=warmPole*(compensated-st.warmthLow);
    // Warmth deliberately changes *harmonic generation*, not only EQ:
    // a level-dependent asymmetric second-order contribution adds H2,
    // while a gentle low-frequency tilt retains perceived body.
    // Zero retains the original transfer exactly for old projects.
    float shape=compensated+warmth*.36f*(st.warmthLow-compensated);
    st.toneLow+=tonePole*(shape-st.toneLow);
    shape+=tone*.65f*(shape-2.f*st.toneLow);
    st.highLow+=highPole*(shape-st.highLow);
    const float high=shape-st.highLow;
    st.highEnv+=envPole*(std::abs(high)-st.highEnv);
    // High-frequency suppression is level-dependent: quiet detail passes.
    const float above=std::clamp((st.highEnv-.018f)/.12f,0.f,1.f);
    shape-=high*(fizz*.75f*above);
    st.dc+=dcPole*(shape-st.dc);
    const float processed=(shape-st.dc)*output*(1.f+1.5f*impact);
    out=safe(delayedInput+wet*(processed-delayedInput));
    in2+=delayedInput*delayedInput;out2+=out*out;
    const double delta=out-delayedInput;delta2+=delta*delta;
   }
   delayPosition_=(delayPosition_+1)%latencySamples();
  }
  const float scale=1.f/static_cast<float>(2*count);
  reading_={std::sqrt(static_cast<float>(in2)*scale),
            std::sqrt(static_cast<float>(out2)*scale),
            std::sqrt(static_cast<float>(delta2)*scale)};
 }
};
}
