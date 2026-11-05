#pragma once
// Experimental fixed-rate 2x oversampler for nonlinear processors.
// 65-tap windowed-sinc reconstruction and anti-alias decimation, no heap allocations.
#include <array>
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <utility>
namespace masterlab {
class HarmonicOversampler2x {
 public:
  static constexpr int taps=65;
  static constexpr int latencySamples=taps/2; // up+down group delay = 64 high-rate samples
 private:
  std::array<double,taps> kernel_{};
  std::array<float,taps> up_{},down_{};
  int upPos_=0,downPos_=0;
  static constexpr double pi_=3.1415926535897932384626433832795;
  static void add(std::array<float,taps>& ring,int& pos,float sample)noexcept {ring[pos]=sample;pos=(pos+1)%taps;}
  float filter(const std::array<float,taps>& ring,int pos)const noexcept {
   double value=0.;int idx=pos;
   for(int i=0;i<taps;++i){idx=(idx==0?taps:idx)-1;value+=kernel_[i]*ring[idx];}
   return static_cast<float>(value);
  }
 public:
  HarmonicOversampler2x()noexcept {prepare();}
  void prepare()noexcept {
   // cutoff is 0.23 cycles per 2x sample (output Nyquist corresponds to 0.25).
   constexpr double cutoff=.23;
   double sum=0.;
   for(int n=0;n<taps;++n){const int i=n-(taps-1)/2;
    const double ideal=i==0?2*cutoff:std::sin(2*pi_*cutoff*i)/(pi_*i);
    const double window=.42-.5*std::cos(2*pi_*n/(taps-1))+.08*std::cos(4*pi_*n/(taps-1));
    kernel_[n]=ideal*window;sum+=kernel_[n];}
   for(auto& c:kernel_)c/=sum;
   reset();
  }
  void reset()noexcept {up_={};down_={};upPos_=downPos_=0;}
  template<class Nonlinear> float tick(float input,Nonlinear&& shape)noexcept {
   float output=0;
   for(int sub=0;sub<2;++sub){
    add(up_,upPos_,sub==0?2.f*input:0.f);
    const float interpolated=filter(up_,upPos_);
    const float shaped=shape(interpolated);
    add(down_,downPos_,std::isfinite(shaped)?std::clamp(shaped,-32.f,32.f):0.f);
    if(sub==1)output=filter(down_,downPos_);
   }
   return output;
  }
};
}
