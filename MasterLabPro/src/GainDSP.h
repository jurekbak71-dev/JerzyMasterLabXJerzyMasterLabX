#pragma once
#include <cmath>
#include <algorithm>
#include <cstddef>
namespace ml {
// Full-band test processor; audio-thread owned. No allocation in process.
class GainDSP {
public:
 void prepare(double sampleRate) noexcept {
  rampSamples=std::max(1, int(std::clamp(std::isfinite(sampleRate)?sampleRate:44100.,1.,768000.)*.02));
  current=target=1.f;remaining=0;step=0;
 }
 void setGainDb(float db) noexcept {
  if(!std::isfinite(db))return;
  const float next=std::pow(10.f,std::clamp(db,-24.f,24.f)/20.f);
  if(next==target)return;
  target=next;remaining=rampSamples;step=(target-current)/float(remaining);
 }
 void process(float* left,float* right,std::size_t n) noexcept {
  for(std::size_t i=0;i<n;++i){
   if(remaining>0){current+=step;if(--remaining==0)current=target;}
   if(left)left[i]*=current;
   if(right&&right!=left)right[i]*=current;
  }
 }
private:
 int rampSamples=882,remaining=0;float current=1,target=1,step=0;
};
}
