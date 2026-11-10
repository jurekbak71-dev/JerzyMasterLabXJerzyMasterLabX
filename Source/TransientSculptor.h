#pragma once
// Phase 5B: zero-lookahead, three-band transient shaping.
// Fixed memory, no allocation/locks in audio callback. Host latency 0.
#include <algorithm>
#include <array>
#include <cmath>
namespace masterlab {
struct TransientSculptorSettings {
 bool enabled=false;
 std::array<float,3> attack{},sustain{};
 float mix=1.f,outputDb=0.f;
 bool delta=false;
};
struct TransientSculptorReading {
 float inputRms=0.f,outputRms=0.f,deltaRms=0.f;
 std::array<float,3> modulationRms{};
 std::array<float,64> inputWave{},outputWave{};
};
class TransientSculptor {
 struct Slew {
  float value=0.f,c=.001f;
  void prepare(double sr,float initial)noexcept{
   value=initial;c=static_cast<float>(1.-std::exp(-1./(sr*.02)));
  }
  float tick(float target)noexcept {value+=c*(target-value);return value;}
 };
 struct Band {
  float fast=0.f,slow=0.f;
 };
 struct Channel {
  float low=0.f,midLow=0.f;
  std::array<Band,3> envelope{};
 };
 std::array<Channel,2> channels_{};
 std::array<Slew,3> attack_{},sustain_{};
 Slew active_{},mix_{},output_{},deltaMonitor_{};
 TransientSculptorSettings settings_{};
 TransientSculptorReading reading_{};
 double rate_=48000.;
 static float safe(float x,float lo,float hi)noexcept{
  return std::clamp(std::isfinite(x)?x:lo,lo,hi);
 }
public:
 void settings(TransientSculptorSettings s)noexcept{settings_=s;}
 void prepare(double sr)noexcept{
  rate_=std::isfinite(sr)&&sr>=8000.&&sr<=384000.?sr:48000.;
  reset();
 }
 void reset()noexcept{
  channels_={};reading_={};
  active_.prepare(rate_,settings_.enabled?1.f:0.f);
  mix_.prepare(rate_,safe(settings_.mix,0.f,1.f));
  output_.prepare(rate_,safe(settings_.outputDb,-12.f,12.f));
  deltaMonitor_.prepare(rate_,settings_.delta?1.f:0.f);
  for(int k=0;k<3;++k){
   attack_[k].prepare(rate_,safe(settings_.attack[k],-1.f,1.f));
   sustain_[k].prepare(rate_,safe(settings_.sustain[k],-1.f,1.f));
  }
 }
 TransientSculptorReading reading()const noexcept{return reading_;}
 void process(float* left,float* right,int n)noexcept{
  if(!left||!right||n<=0)return;
  if(!settings_.enabled&&active_.value<1e-6f){reading_={};if(settings_.delta)for(int i=0;i<n;++i)left[i]=right[i]=0.f;return;}
  const float splitLow=static_cast<float>(1.-std::exp(-6.283185307179586*170./rate_));
  const float splitMid=static_cast<float>(1.-std::exp(-6.283185307179586*3400./rate_));
  const float fastRise=static_cast<float>(1.-std::exp(-1./(rate_*.0015)));
  const float fastFall=static_cast<float>(1.-std::exp(-1./(rate_*.012)));
  const float slowStep=static_cast<float>(1.-std::exp(-1./(rate_*.055)));
  double input2=0.,out2=0.,delta2=0.;
  std::array<double,3> modulation2{};
  std::array<float,64> inWave{},outWave{};
  for(int i=0;i<n;++i){
   const float wet=active_.tick(settings_.enabled?1.f:0.f)*mix_.tick(safe(settings_.mix,0.f,1.f));
   const float output=std::pow(10.f,output_.tick(safe(settings_.outputDb,-12.f,12.f))*.05f);
   std::array<float,3> attack{},sustain{};
   for(int k=0;k<3;++k){
    attack[k]=attack_[k].tick(safe(settings_.attack[k],-1.f,1.f));
    sustain[k]=sustain_[k].tick(safe(settings_.sustain[k],-1.f,1.f));
   }
   float inputMono=0.f,outputMono=0.f;
   for(int ch=0;ch<2;++ch){
    float& io=(ch==0?left[i]:right[i]);
    const float dry=safe(io,-8.f,8.f);
    auto& state=channels_[ch];
    state.low+=splitLow*(dry-state.low);
    state.midLow+=splitMid*(dry-state.midLow);
    const std::array<float,3> b{state.low,state.midLow-state.low,dry-state.midLow};
    float processed=0.f;
    for(int k=0;k<3;++k){
     auto& e=state.envelope[k];
     const float level=std::abs(b[k]);
     const float rise=level>e.fast?fastRise:fastFall;
     e.fast+=rise*(level-e.fast);
     e.slow+=slowStep*(level-e.slow);
     const float difference=e.fast-e.slow;
     const float normal=std::min(1.f,std::abs(difference)/(e.slow+.04f));
     const float attackShape=std::max(0.f,difference)*normal/(std::abs(difference)+1e-12f);
     const float sustainShape=std::max(0.f,-difference)*normal/(std::abs(difference)+1e-12f);
     const float gain=std::clamp(1.f+3.2f*attack[k]*attackShape+2.3f*sustain[k]*sustainShape,.12f,4.5f);
     processed+=b[k]*gain;
     modulation2[k]+=static_cast<double>(gain-1.f)*(gain-1.f)*b[k]*b[k];
    }
    const float y=safe(dry+wet*(processed*output-dry),-16.f,16.f);
    const float deltaMonitor=deltaMonitor_.value;
    io=y+deltaMonitor*(dry-2.f*y);
    inputMono+=.5f*dry;outputMono+=.5f*y;
    const double diff=static_cast<double>(y)-dry;
    input2+=static_cast<double>(dry)*dry;
    out2+=static_cast<double>(y)*y;
    delta2+=diff*diff;
   }
   deltaMonitor_.tick(settings_.delta?1.f:0.f);
   const size_t scopeIndex=static_cast<size_t>(i)*64/static_cast<size_t>(n);
   inWave[scopeIndex]=inputMono;outWave[scopeIndex]=outputMono;
  }
  const double inv=1./(2.*n);
  reading_.inputRms=static_cast<float>(std::sqrt(input2*inv));
  reading_.outputRms=static_cast<float>(std::sqrt(out2*inv));
  reading_.deltaRms=static_cast<float>(std::sqrt(delta2*inv));
  for(int k=0;k<3;++k)reading_.modulationRms[k]=static_cast<float>(std::sqrt(modulation2[k]*inv));
  reading_.inputWave=inWave;reading_.outputWave=outWave;
 }
};
}
