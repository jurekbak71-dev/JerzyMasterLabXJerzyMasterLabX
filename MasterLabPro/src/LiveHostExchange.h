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
 Host* active=nullptr;Host* target=nullptr;unsigned progress=0,warmRemaining=0;
 uint32_t heldLatency=0;
 StereoDelayAligner lines[2];unsigned activeLine=0;
 std::array<float,2048> oldL{},oldR{},newL{},newR{};
 static constexpr unsigned fadeSamples=256, minimumWarmSamples=256;
 static uint32_t latencyOf(const Host* h)noexcept{return std::min(65536u,h->diagnostics().latency);}
 void align(unsigned line,float* l,float* r,unsigned n,uint32_t delay)noexcept{
  lines[line].setDelay(delay);for(unsigned i=0;i<n;++i){float al=0,ar=0;lines[line].process(l[i],r[i],al,ar);l[i]=al;r[i]=ar;}
 }
public:
 void resetWhenStopped(Host* host)noexcept{active=host;target=nullptr;progress=warmRemaining=0;requested.store(nullptr);acknowledged.store(nullptr);activeLine=0;for(auto& line:lines)line.reset();heldLatency=host?latencyOf(host):0;effectiveLatency.store(heldLatency);}
 void publish(Host* host)noexcept{requested.store(host,std::memory_order_release);}
 Host* takeRetired()noexcept{return acknowledged.exchange(nullptr,std::memory_order_acquire);}
 uint32_t latency()const noexcept{return effectiveLatency.load(std::memory_order_acquire);}
 bool process(float* l,float* r,unsigned frames)noexcept{
  if(!active||!l||!r)return false;
  if(!target){target=requested.exchange(nullptr,std::memory_order_acq_rel);progress=0;if(target){lines[1-activeLine].reset();warmRemaining=std::max(minimumWarmSamples,std::max(heldLatency,latencyOf(target)));}}
  unsigned offset=0;
  while(offset<frames){
   const unsigned n=std::min(2048u,frames-offset);
   const auto oldLatency=latencyOf(active);
   heldLatency=std::max(heldLatency,oldLatency);
   if(!target){if(!active->process(l+offset,r+offset,n))return false;align(activeLine,l+offset,r+offset,n,heldLatency-oldLatency);}
   else{
    const auto newLatency=latencyOf(target);
    std::copy_n(l+offset,n,oldL.data());std::copy_n(r+offset,n,oldR.data());
    std::copy_n(oldL.data(),n,newL.data());std::copy_n(oldR.data(),n,newR.data());
    if(!active->process(oldL.data(),oldR.data(),n)||!target->process(newL.data(),newR.data(),n))return false;
    // During warm-up preserve the audible path; accumulate both delay histories.
    if(!warmRemaining)heldLatency=std::max(heldLatency,newLatency);
    align(activeLine,oldL.data(),oldR.data(),n,heldLatency-oldLatency);
    const uint32_t targetLatency=std::max(heldLatency,newLatency);
    align(1-activeLine,newL.data(),newR.data(),n,targetLatency-newLatency);
    if(warmRemaining){std::copy_n(oldL.data(),n,l+offset);std::copy_n(oldR.data(),n,r+offset);warmRemaining-=std::min(warmRemaining,n);}
    else{
     for(unsigned i=0;i<n;++i){const float x=float(std::min(progress+1,fadeSamples))/float(fadeSamples);l[offset+i]=oldL[i]+x*(newL[i]-oldL[i]);r[offset+i]=oldR[i]+x*(newR[i]-oldR[i]);if(progress<fadeSamples)++progress;}
     if(progress==fadeSamples){auto* old=active;active=target;target=nullptr;activeLine=1-activeLine;acknowledged.store(old,std::memory_order_release);}
    }
   }
   effectiveLatency.store(heldLatency,std::memory_order_release);
   offset+=n;
  }
  return true;
 }
};
}
