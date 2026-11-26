#pragma once
#include "ModularGraph.h"
#include <atomic>
#include <algorithm>
#include <array>
#include <cstdint>
namespace ml {
template<class Host> class LiveHostExchange {
 static_assert(std::atomic<Host*>::is_always_lock_free);
 std::atomic<Host*> requested{nullptr},acknowledged{nullptr};
 std::atomic<uint32_t> effectiveLatency{0};
 Host* active=nullptr;Host* target=nullptr;
 unsigned progress=0,warmRemaining=0;
 uint32_t heldLatency=0,lastOld=0,lastNew=0;
 struct TapTransition{
  uint32_t from=0,to=0;unsigned position=256;
  void reset()noexcept{from=to=0;position=256;}
  void request(uint32_t value)noexcept{if(position>=256&&to!=value){from=to;to=value;position=0;}}
  bool settled(uint32_t value)const noexcept{return position>=256&&to==value;}
  void tick(StereoDelayAligner& line,float& l,float& r)noexcept{
   const float mix=float(std::min(position+1,256u))/256.f;float a=0,b=0;
   line.processBlend(l,r,from,to,mix,a,b);l=a;r=b;if(position<256)++position;
  }
 };
 StereoDelayAligner lines[2];TapTransition taps[2];unsigned activeLine=0;
 std::array<float,2048> oldL{},oldR{},newL{},newR{};
 static constexpr unsigned fadeSamples=256,minimumWarmSamples=256;
 static uint32_t latencyOf(const Host* h)noexcept{return std::min(65536u,h->diagnostics().latency);}
 void align(unsigned line,float* l,float* r,unsigned n,uint32_t delay)noexcept{
  taps[line].request(delay);for(unsigned i=0;i<n;++i)taps[line].tick(lines[line],l[i],r[i]);
 }
public:
 void resetWhenStopped(Host* host)noexcept{
  active=host;target=nullptr;progress=warmRemaining=0;requested.store(nullptr);acknowledged.store(nullptr);activeLine=0;
  for(auto& line:lines){line.reset();}
  for(auto& tap:taps){tap.reset();}
  heldLatency=host?latencyOf(host):0;lastOld=heldLatency;lastNew=0;effectiveLatency.store(heldLatency);
 }
 void publish(Host* host)noexcept{requested.store(host,std::memory_order_release);}
 Host* takeRetired()noexcept{return acknowledged.exchange(nullptr,std::memory_order_acquire);}
 uint32_t latency()const noexcept{return effectiveLatency.load(std::memory_order_acquire);}
 bool process(float* l,float* r,unsigned frames)noexcept{
  if(!active||!l||!r)return false;
  if(!target){target=requested.exchange(nullptr,std::memory_order_acq_rel);progress=0;if(target){
    lines[1-activeLine].reset();taps[1-activeLine].reset();lastOld=latencyOf(active);lastNew=latencyOf(target);
    warmRemaining=std::max(minimumWarmSamples,std::max(heldLatency,lastNew));
  }}
  unsigned offset=0;
  while(offset<frames){
   const unsigned n=std::min(2048u,frames-offset);
   const auto oldLatency=latencyOf(active);
   heldLatency=std::max(heldLatency,oldLatency);
   if(!target){
    if(!active->process(l+offset,r+offset,n))return false;
    align(activeLine,l+offset,r+offset,n,heldLatency-oldLatency);
   }else{
    const auto newLatency=latencyOf(target);
    // Changes invalidate the previous alignment. Hold the existing blend while both
    // output histories settle, rather than jumping back to the old graph.
    if(oldLatency!=lastOld||newLatency!=lastNew){
     lastOld=oldLatency;lastNew=newLatency;
     warmRemaining=std::max(minimumWarmSamples,std::max(heldLatency,newLatency));
    }
    std::copy_n(l+offset,n,oldL.data());std::copy_n(r+offset,n,oldR.data());
    std::copy_n(oldL.data(),n,newL.data());std::copy_n(oldR.data(),n,newR.data());
    if(!active->process(oldL.data(),oldR.data(),n)||!target->process(newL.data(),newR.data(),n))return false;
    // Do not move the old audible tap until the new graph has received live input.
    if(!warmRemaining||progress)heldLatency=std::max(heldLatency,newLatency);
    const auto oldDelay=heldLatency-oldLatency;
    const auto newDelay=std::max(heldLatency,newLatency)-newLatency;
    const bool aligned=taps[activeLine].settled(oldDelay)&&taps[1-activeLine].settled(newDelay);
    align(activeLine,oldL.data(),oldR.data(),n,oldDelay);
    align(1-activeLine,newL.data(),newR.data(),n,newDelay);
    const bool warming=warmRemaining!=0;
    warmRemaining-=std::min(warmRemaining,n);
    const bool stable=oldLatency==latencyOf(active)&&newLatency==latencyOf(target);
    const bool advance=!warming&&aligned&&stable;
    for(unsigned i=0;i<n;++i){
     if(advance&&progress<fadeSamples)++progress;
     const float x=float(progress)/float(fadeSamples);
     l[offset+i]=oldL[i]+x*(newL[i]-oldL[i]);r[offset+i]=oldR[i]+x*(newR[i]-oldR[i]);
    }
    if(advance&&progress==fadeSamples){auto* old=active;active=target;target=nullptr;activeLine=1-activeLine;acknowledged.store(old,std::memory_order_release);}
   }
   effectiveLatency.store(heldLatency,std::memory_order_release);offset+=n;
  }
  return true;
 }
};
}
