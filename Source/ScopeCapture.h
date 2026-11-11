#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace masterlab {
// Continuous sample-clock capture, independent of host callback boundaries.
// No resampling: every plotted point is one actual sample. A rising zero
// crossing stabilizes periodic signals; auto trigger refreshes DC/silence.
class ScopeCapture {
public:
 static constexpr size_t size=512;
 using Wave=std::array<float,size>;
private:
 std::array<Wave,3> pending_{},published_{};
 size_t position_=0;
 int waiting_=0,timeout_=4800;
 float previous_=0.f;
 bool capturing_=false;
public:
 void prepare(double rate)noexcept {
  timeout_=static_cast<int>(std::clamp(std::isfinite(rate)?rate:48000.,8000.,384000.)*.1);
  reset();
 }
 void reset()noexcept {pending_={};published_={};pending_[2].fill(1.f);published_[2].fill(1.f);position_=0;waiting_=0;previous_=0;capturing_=false;}
 void push(float input,float output,float gain=1.f)noexcept {
  if(!capturing_ && ((previous_<=0.f&&input>0.f)||++waiting_>=timeout_)) {
   capturing_=true;position_=0;waiting_=0;
  }
  previous_=input;
  if(!capturing_)return;
  pending_[0][position_]=input;pending_[1][position_]=output;pending_[2][position_]=gain;
  if(++position_==size){published_=pending_;capturing_=false;}
 }
 const Wave& input()const noexcept{return published_[0];}
 const Wave& output()const noexcept{return published_[1];}
 const Wave& gain()const noexcept{return published_[2];}
};
}
