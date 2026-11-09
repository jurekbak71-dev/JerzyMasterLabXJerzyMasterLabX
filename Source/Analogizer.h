#pragma once
// Independent zero-latency tonal colour processor. Not a copy of JerzyTapeDriveVST3.
// Tube/transistor/tape transfer, bounded drive compensation and transient-aware
// de-fizz. No wow, flutter, hiss, noise, dropouts or callback allocations.
#include <algorithm>
#include <array>
#include <cmath>
namespace masterlab {
enum class AnalogizerMode : int { Tube=0, Transistor=1, Tape=2 };
struct AnalogizerSettings {
 bool enabled=false;
 AnalogizerMode mode=AnalogizerMode::Tube;
 float driveDb=0.f,warmth=0.f,deFizz=0.f,mix=1.f,outputDb=0.f;
};
struct AnalogizerReading {float inputRms=0.f,outputRms=0.f,deltaRms=0.f;};
class Analogizer {
 struct State {float warmthLow=0.f,highLow=0.f,highEnv=0.f,tapeMemory=0.f,dc=0.f;};
 std::array<State,2> channels_{};
 AnalogizerSettings settings_{};
 AnalogizerReading reading_{};
 double rate_=48000.;
 float bypass_=0.f,driveDb_=0.f,warmth_=0.f,deFizz_=0.f,mix_=1.f,outputDb_=0.f;
 float smooth_=0.001f;
 static float safe(float x)noexcept {return std::isfinite(x)?std::clamp(x,-8.f,8.f):0.f;}
 static float sat(float x,AnalogizerMode mode,float memory)noexcept {
  if(mode==AnalogizerMode::Transistor)return x/(1.f+.22f*std::abs(x)+.055f*x*x);
  if(mode==AnalogizerMode::Tape)return std::tanh(.85f*(x+.16f*memory));
  return std::tanh(x+.065f)-std::tanh(.065f);
 }
public:
 void settings(AnalogizerSettings s)noexcept {settings_=s;}
 void prepare(double rate)noexcept {
  rate_=std::isfinite(rate)&&rate>=8000.&&rate<=384000.?rate:48000.;
  smooth_=static_cast<float>(1.-std::exp(-1./(rate_*.02)));
  reset();
 }
 void reset()noexcept {
  channels_={};reading_={};bypass_=settings_.enabled?1.f:0.f;
  driveDb_=settings_.driveDb;warmth_=settings_.warmth;deFizz_=settings_.deFizz;
  mix_=settings_.mix;outputDb_=settings_.outputDb;
 }
 AnalogizerReading reading()const noexcept{return reading_;}
 int latencySamples()const noexcept{return 0;}
 void process(float* left,float* right,int count)noexcept {
  if(!left||!right||count<=0)return;
  if(!settings_.enabled && bypass_<1.e-6f){reading_={};return;}
  const float warmPole=static_cast<float>(1.-std::exp(-6.283185307179586*1850./rate_));
  const float highPole=static_cast<float>(1.-std::exp(-6.283185307179586*5400./rate_));
  const float magneticPole=static_cast<float>(1.-std::exp(-1./(rate_*.003)));
  const float envPole=static_cast<float>(1.-std::exp(-1./(rate_*.008)));
  const float dcPole=static_cast<float>(1.-std::exp(-6.283185307179586*12./rate_));
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
   const float output=std::pow(10.f,.05f*slew(outputDb_,std::clamp(settings_.outputDb,-12.f,12.f)));
   for(int ch=0;ch<2;++ch){
    float& out=(ch==0?left[i]:right[i]);const float input=safe(out);
    auto& st=channels_[ch];
    const float x=input*drive;
    st.tapeMemory+=magneticPole*(x-st.tapeMemory);
    const float coloured=sat(x,settings_.mode,st.tapeMemory);
    // Compensation: moderate output growth, not hard clipping of peaks.
    const float compensated=coloured/(1.f+.30f*(drive-1.f));
    st.warmthLow+=warmPole*(compensated-st.warmthLow);
    float shape=compensated+warmth*.18f*(st.warmthLow-compensated);
    st.highLow+=highPole*(shape-st.highLow);
    const float high=shape-st.highLow;
    st.highEnv+=envPole*(std::abs(high)-st.highEnv);
    // High-frequency suppression is level-dependent: quiet detail passes.
    const float above=std::clamp((st.highEnv-.018f)/.12f,0.f,1.f);
    shape-=high*(fizz*.75f*above);
    st.dc+=dcPole*(shape-st.dc);
    const float processed=(shape-st.dc)*output;
    out=safe(input+wet*(processed-input));
    in2+=input*input;out2+=out*out;
    const double delta=out-input;delta2+=delta*delta;
   }
  }
  const float scale=1.f/static_cast<float>(2*count);
  reading_={std::sqrt(static_cast<float>(in2)*scale),
            std::sqrt(static_cast<float>(out2)*scale),
            std::sqrt(static_cast<float>(delta2)*scale)};
 }
};
}
