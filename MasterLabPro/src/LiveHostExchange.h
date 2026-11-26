#pragma once
#include <atomic>
#include <algorithm>
#include <array>
#include <cstdint>
namespace ml {
template<class Host> class LiveHostExchange {
 static_assert(std::atomic<Host*>::is_always_lock_free);
 std::atomic<Host*> requested{nullptr}, acknowledged{nullptr};
 Host* active=nullptr;Host* target=nullptr;unsigned progress=0;
 std::array<float,2048> oldL{},oldR{},newL{},newR{};
 static constexpr unsigned fadeSamples=256;
public:
 void resetWhenStopped(Host* host)noexcept{active=host;target=nullptr;progress=0;requested.store(nullptr);acknowledged.store(nullptr);}
 void publish(Host* host)noexcept{requested.store(host,std::memory_order_release);}
 Host* takeRetired()noexcept{return acknowledged.exchange(nullptr,std::memory_order_acquire);}
 bool process(float* l,float* r,unsigned frames)noexcept{
  if(!active||!l||!r)return false;
  if(!target){target=requested.exchange(nullptr,std::memory_order_acq_rel);progress=0;}
  unsigned offset=0;
  while(offset<frames){
   const unsigned n=std::min(2048u,frames-offset);
   if(!target){if(!active->process(l+offset,r+offset,n))return false;}
   else{
    std::copy_n(l+offset,n,oldL.data());std::copy_n(r+offset,n,oldR.data());
    std::copy_n(oldL.data(),n,newL.data());std::copy_n(oldR.data(),n,newR.data());
    if(!active->process(oldL.data(),oldR.data(),n)||!target->process(newL.data(),newR.data(),n))return false;
    for(unsigned i=0;i<n;++i){const float x=float(std::min(progress+1,fadeSamples))/float(fadeSamples);l[offset+i]=oldL[i]+x*(newL[i]-oldL[i]);r[offset+i]=oldR[i]+x*(newR[i]-oldR[i]);if(progress<fadeSamples)++progress;}
    if(progress==fadeSamples){auto* old=active;active=target;target=nullptr;acknowledged.store(old,std::memory_order_release);}
   }
   offset+=n;
  }
  return true;
 }
};
}
