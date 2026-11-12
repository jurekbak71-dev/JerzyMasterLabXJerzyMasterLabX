#pragma once
// Experimental zero-latency music restoration. Four genuine independent
// processes: trained 16-band noise print, harmonic hum notches, causal
// impulse repair, and approximate saturated-plateau reconstruction.
// No claim of perfect source reconstruction or full FFT spectral repair.
#include <algorithm>
#include <array>
#include <cmath>
namespace masterlab {
struct RestorationSettings {
 bool enabled=false,hiss=false,hum=false,click=false,clip=false;
 bool learn=false,freeze=false,delta=false,abDry=false;
 int humHz=50;
 float thresholdDb=-48.f,reductionDb=6.f,quality=.5f,protect=.5f,mix=1.f;
};
struct RestorationReading {
 float inputRms=0.f,outputRms=0.f,removedRms=0.f;
 float noisePrintLevel=0.f;
 int impulsesRepaired=0,plateausReconstructed=0;
 bool learned=false;
};
class AudioRestoration {
 static constexpr size_t noiseBands=16;
 struct Notch {
  float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
  void prepare(double sr,double frequency,double q)noexcept{
   const double w=6.283185307179586*frequency/sr;
   const double c=std::cos(w),alpha=std::sin(w)/(2.*q),a0=1.+alpha;
   b0=static_cast<float>(1./a0);b1=static_cast<float>(-2.*c/a0);b2=b0;
   a1=b1;a2=static_cast<float>((1.-alpha)/a0);z1=z2=0;
  }
  float tick(float x)noexcept {
   const float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;
   return std::isfinite(y)?y:0.f;
  }
 };
 struct Channel {
  float old=0.f,old2=0.f;
  std::array<float,noiseBands-1> noiseLow{};
  std::array<float,noiseBands> noise{},envelope{},gain{};
  int plateau=0;
  std::array<Notch,4> hum50{},hum60{};
 };
 std::array<Channel,2> channels_{};
 RestorationSettings settings_{};
 RestorationReading reading_{};
 double rate_=48000.;
 float mix_=0.f;
 static float finite(float x)noexcept{return std::isfinite(x)?std::clamp(x,-8.f,8.f):0.f;}
 static float gainDb(float db)noexcept{return std::pow(10.f,db/20.f);}
public:
 void settings(RestorationSettings s)noexcept {settings_=s;}
 void prepare(double sr)noexcept {
  rate_=std::isfinite(sr)&&sr>=8000.&&sr<=384000.?sr:48000.;
  reset();
 }
 void reset()noexcept {
  channels_={};for(auto& channel:channels_)channel.gain.fill(1.f);reading_={};mix_=settings_.enabled&&!settings_.abDry?settings_.mix:0.f;
  for(auto& c:channels_)for(int i=0;i<4;++i){
   c.hum50[i].prepare(rate_,50.*(i+1),25.);
   c.hum60[i].prepare(rate_,60.*(i+1),25.);
  }
 }
 int latencySamples()const noexcept{return 0;}
 RestorationReading reading()const noexcept{return reading_;}
 void process(float* left,float* right,int n)noexcept {
  if(!left||!right||n<=0)return;
  if(!settings_.enabled && mix_<1e-6f){reading_={};return;}
  const float mixStep=static_cast<float>(1.-std::exp(-1./(rate_*.02)));
  const float envStep=static_cast<float>(1.-std::exp(-1./(rate_*.012)));
  const float noiseStep=static_cast<float>(1.-std::exp(-1./(rate_*.14)));
  const float gainStep=static_cast<float>(1.-std::exp(-1./(rate_*(.012+.08*std::clamp(settings_.quality,0.f,1.f)))));
  std::array<float,noiseBands-1> poles{};
  for(size_t band=0;band<poles.size();++band){
   const double fraction=static_cast<double>(band)/(poles.size()-1);
   const double frequency=80.*std::pow(std::min(16000.,rate_*.4)/80.,fraction);
   poles[band]=static_cast<float>(1.-std::exp(-6.283185307179586*frequency/rate_));
  }
  double energyIn=0.,energyOut=0.,energyRemoved=0.,print=0.;
  int impulses=0,plateaus=0;bool hasPrint=false;
  const float reduction=std::clamp(settings_.reductionDb,0.f,24.f);
  const float floorGain=gainDb(-reduction);
  // Threshold is a broadband amplitude control; distribute its equivalent
  // energy across the analysis bands before comparing per-band envelopes.
  const float trigger=gainDb(std::clamp(settings_.thresholdDb,-90.f,-6.f))/std::sqrt(static_cast<float>(noiseBands));
  const float protect=std::clamp(settings_.protect,0.f,1.f);
  for(int i=0;i<n;++i){
   const float desired=settings_.enabled&&!settings_.abDry?
       std::clamp(settings_.mix,0.f,1.f):0.f;
   mix_+=mixStep*(desired-mix_);
   for(int c=0;c<2;++c){
    float& io=c==0?left[i]:right[i];const float x=finite(io);
    auto& st=channels_[c];float current=x;
    if(settings_.hum){
     for(auto& f:settings_.humHz==60?st.hum60:st.hum50)current=f.tick(current);
    }
    if(settings_.click){
     const float predicted=std::clamp(2.f*st.old-st.old2,-2.f,2.f);
     const float quietStep=std::abs(st.old-st.old2);
     const float cutoff=.07f+.55f*protect;
     if(quietStep<cutoff*.22f && std::abs(current-predicted)>cutoff &&
        std::abs(current)>std::abs(st.old)+cutoff){
      current=predicted; ++impulses;
     }
    }
    if(settings_.clip){
     if(std::abs(current)>.965f &&
        std::abs(current-st.old)<.023f &&
        current*st.old>0.f){st.plateau=std::min(6,st.plateau+1);
      const float recovered=std::min(1.25f,std::abs(current)+(.018f+.022f*st.plateau)*(1.f-protect*.65f));
      current=std::copysign(recovered,current);++plateaus;
     }else st.plateau=0;
    }
    if(settings_.hiss || settings_.learn){
     std::array<float,noiseBands> band{};float previousLow=0.f;
     for(size_t k=0;k<noiseBands-1;++k){
      st.noiseLow[k]+=poles[k]*(current-st.noiseLow[k]);
      band[k]=st.noiseLow[k]-previousLow;previousLow=st.noiseLow[k];
     }
     band.back()=current-previousLow;
     for(size_t k=0;k<noiseBands;++k){
      st.envelope[k]+=envStep*(std::abs(band[k])-st.envelope[k]);
      if(settings_.learn&&!settings_.freeze)
       st.noise[k]+=noiseStep*(st.envelope[k]-st.noise[k]);
      const float estimate=st.noise[k];
      if(estimate>1.e-6f){hasPrint=true;print+=estimate;}
      // Preserves transients: noise floor is attenuated only when level is
      // close to the learned floor. Smoothing avoids rapid musical-noise gating.
      const float threshold=std::max(trigger,estimate*(1.05f+1.f*(1.f-protect)));
      const float ratio=std::clamp((st.envelope[k]-threshold)/(threshold+.00001f),0.f,1.f);
      const float target=settings_.hiss&&estimate>1.e-6f?
          floorGain+(1.f-floorGain)*ratio:1.f;
      st.gain[k]+=gainStep*(target-st.gain[k]);
      band[k]*=st.gain[k];
     }
     current=0.f;for(float value:band)current+=value;
    }
    st.old2=st.old;st.old=x;
    const float result=finite(x+mix_*(current-x));
    io=settings_.delta?finite(x-result):result;
    const float removed=x-result;
    energyIn+=x*x;energyOut+=result*result;energyRemoved+=removed*removed;
   }
  }
  const float inv=1.f/static_cast<float>(2*n);
  reading_={std::sqrt(static_cast<float>(energyIn)*inv),
            std::sqrt(static_cast<float>(energyOut)*inv),
            std::sqrt(static_cast<float>(energyRemoved)*inv),
            static_cast<float>(print)*inv,impulses,plateaus,hasPrint};
 }
};
}
