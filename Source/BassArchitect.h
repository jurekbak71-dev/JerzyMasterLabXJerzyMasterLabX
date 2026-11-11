#pragma once
// Bass Architect prototype DSP. No host-facing APVTS or routing changes yet.
// Zero-latency, stereo, no heap allocation/locks in process().
#include <algorithm>
#include <array>
#include <cmath>
#include "ScopeCapture.h"
namespace masterlab {
struct BassArchitectSettings {
 bool enabled=false;
 float sub=0.f,body=0.f,harmonics=0.f,monoBass=0.f,mix=1.f,outputDb=0.f;
 bool delta=false;
};
struct BassArchitectReading {
 float inputRms=0.f,outputRms=0.f,deltaRms=0.f,lowRms=0.f;
 std::array<float,3> sectionsRms{};
 ScopeCapture::Wave inputWave{},outputWave{};
};
class BassArchitect {
 struct Smoother {
  float current=0.f,step=.001f;
  void prepare(double sr,float initial)noexcept {
   current=initial;step=static_cast<float>(1.-std::exp(-1./(std::max(8000.,sr)*.02)));
  }
  float tick(float target)noexcept{current+=step*(target-current);return current;}
 };
 struct Channel {
  float sub=0.f,body=0.f,dc=0.f;
 };
 std::array<Channel,2> state_{};
 Smoother enabled_{},sub_{},body_{},harmonics_{},mono_{},mix_{},out_{},deltaMonitor_{};
 BassArchitectSettings settings_{};
 BassArchitectReading reading_{};
 ScopeCapture scope_;
 double sr_=48000.;
 static float finite(float value,float lo,float hi)noexcept{
  return std::clamp(std::isfinite(value)?value:lo,lo,hi);
 }
public:
 void settings(BassArchitectSettings next)noexcept {
  if(next.enabled!=settings_.enabled)scope_.reset();
  settings_=next;
 }
 void prepare(double sr)noexcept{
  sr_=std::isfinite(sr)&&sr>=8000.&&sr<=384000.?sr:48000.;
  scope_.prepare(sr_);
  reset();
 }
 void reset()noexcept{
  state_={};reading_={};scope_.reset();
  enabled_.prepare(sr_,settings_.enabled?1.f:0.f);
  sub_.prepare(sr_,finite(settings_.sub,-1.f,1.f));
  body_.prepare(sr_,finite(settings_.body,-1.f,1.f));
  harmonics_.prepare(sr_,finite(settings_.harmonics,0.f,1.f));
  mono_.prepare(sr_,finite(settings_.monoBass,0.f,1.f));
  mix_.prepare(sr_,finite(settings_.mix,0.f,1.f));
  out_.prepare(sr_,finite(settings_.outputDb,-12.f,12.f));
  deltaMonitor_.prepare(sr_,settings_.delta?1.f:0.f);
 }
 BassArchitectReading reading()const noexcept{return reading_;}
 void process(float* left,float* right,int n)noexcept{
  if(!left||!right||n<=0)return;
  if(!settings_.enabled&&enabled_.current<1e-6f){reading_={};if(settings_.delta)for(int i=0;i<n;++i)left[i]=right[i]=0.f;return;}
  const float lp90=static_cast<float>(1.-std::exp(-6.283185307179586*90./sr_));
  const float lp250=static_cast<float>(1.-std::exp(-6.283185307179586*250./sr_));
  const float dcPole=static_cast<float>(1.-std::exp(-6.283185307179586*10./sr_));
  double input2=0.,output2=0.,difference2=0.,low2=0.;
  std::array<double,3> componentEnergy{};
  for(int i=0;i<n;++i){
   const float on=enabled_.tick(settings_.enabled?1.f:0.f);
   const float sub= sub_.tick(finite(settings_.sub,-1.f,1.f));
   const float body=body_.tick(finite(settings_.body,-1.f,1.f));
   const float harm=harmonics_.tick(finite(settings_.harmonics,0.f,1.f));
   const float mono=mono_.tick(finite(settings_.monoBass,0.f,1.f));
   const float wet=mix_.tick(finite(settings_.mix,0.f,1.f));
   const float gain=std::pow(10.f,out_.tick(finite(settings_.outputDb,-12.f,12.f))*.05f);
   float dry[2]{finite(left[i],-8.f,8.f),finite(right[i],-8.f,8.f)};
   float low[2]{},mid[2]{},out[2]{};
   for(int c=0;c<2;++c){
    auto& st=state_[c];
    st.sub+=lp90*(dry[c]-st.sub);
    st.body+=lp250*(dry[c]-st.body);
    low[c]=st.sub;
    mid[c]=st.body-st.sub;
   }
   const float sharedLow=(low[0]+low[1])*.5f;
   for(int c=0;c<2;++c){
    auto& st=state_[c];
    const float bass=low[c]+mono*(sharedLow-low[c]);
    const float shaped=std::tanh((bass+mid[c])*(1.f+5.f*harm));
    // Remove the linear approximation: retain genuine harmonics, not only gain.
    const float nonlinear=shaped-(bass+mid[c])/(1.f+5.f*harm);
    st.dc+=dcPole*(nonlinear-st.dc);
    const float enhancement=harm*.85f*(nonlinear-st.dc);
    const float lowGain=std::pow(10.f,sub*.6f);
    const float bodyGain=std::pow(10.f,body*.6f);
    const float high=dry[c]-low[c]-mid[c];
    const float processed=(high+bass*lowGain+mid[c]*bodyGain+enhancement)*gain;
    out[c]=finite(dry[c]+on*wet*(processed-dry[c]),-16.f,16.f);
    input2+=static_cast<double>(dry[c])*dry[c];
    output2+=static_cast<double>(out[c])*out[c];
    const double d=out[c]-dry[c];difference2+=d*d;
    low2+=static_cast<double>(bass)*bass;
    componentEnergy[0]+=static_cast<double>(bass)*bass;
    componentEnergy[1]+=static_cast<double>(mid[c])*mid[c];
    componentEnergy[2]+=static_cast<double>(enhancement)*enhancement;
   }
   scope_.push(.5f*(dry[0]+dry[1]),.5f*(out[0]+out[1]));
   const float delta=deltaMonitor_.tick(settings_.delta?1.f:0.f);
   left[i]=out[0]+delta*(dry[0]-2.f*out[0]);right[i]=out[1]+delta*(dry[1]-2.f*out[1]);
  }
  const double div=1./(2.*n);
  reading_={static_cast<float>(std::sqrt(input2*div)),
    static_cast<float>(std::sqrt(output2*div)),
    static_cast<float>(std::sqrt(difference2*div)),
    static_cast<float>(std::sqrt(low2*div))};
  for(size_t i=0;i<3;++i)reading_.sectionsRms[i]=static_cast<float>(std::sqrt(componentEnergy[i]*div));
  reading_.inputWave=scope_.input();reading_.outputWave=scope_.output();
 }
};
}
