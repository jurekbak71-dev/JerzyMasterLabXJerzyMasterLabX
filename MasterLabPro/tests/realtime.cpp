#include "AudioRack.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>

namespace {
std::atomic<unsigned long long> allocations{0};
std::atomic<bool> countAllocations{false};
}

void* operator new(std::size_t size) {
 if(countAllocations.load(std::memory_order_relaxed))
  allocations.fetch_add(1,std::memory_order_relaxed);
 if(void* memory=std::malloc(size))return memory;
 throw std::bad_alloc{};
}
void operator delete(void* memory) noexcept {std::free(memory);}
void operator delete(void* memory,std::size_t) noexcept {std::free(memory);}
void* operator new[](std::size_t size) {
 if(countAllocations.load(std::memory_order_relaxed))
  allocations.fetch_add(1,std::memory_order_relaxed);
 if(void* memory=std::malloc(size))return memory;
 throw std::bad_alloc{};
}
void operator delete[](void* memory) noexcept {std::free(memory);}
void operator delete[](void* memory,std::size_t) noexcept {std::free(memory);}

int main(){
 for(double rate:{44100.,48000.,88200.,96000.,192000.}){
  auto rack=std::make_unique<ml::AudioRack>();rack->prepare(rate);
  ml::RackConfig config;
  config.stages[0]={1,12.f};
  if(!rack->submit(config))return 1;
  std::array<float,257> left{},right{};
  const auto before=allocations.load(std::memory_order_relaxed);
  countAllocations.store(true,std::memory_order_relaxed);
  for(int block=0;block<100;++block){
   for(std::size_t i=0;i<left.size();++i){left[i]=std::sin(float(i+block)*.17f)*.1f;right[i]=-left[i];}
   rack->process(left.data(),right.data(),left.size());
  }
  countAllocations.store(false,std::memory_order_relaxed);
  if(allocations.load(std::memory_order_relaxed)!=before)return 2;
  for(float sample:left)if(!std::isfinite(sample))return 3;
 }

 // The bounded control queue rejects overflow instead of overwriting unread snapshots.
 auto rack=std::make_unique<ml::AudioRack>();rack->prepare(48000.);
 for(std::uint64_t id=1;id<=7;++id){ml::RackConfig c;c.stages[0]={id,0};if(!rack->submit(c))return 4;}
 ml::RackConfig overflow;overflow.stages[0]={8,0};if(rack->submit(overflow))return 5;
 std::array<float,17> mono{};rack->process(mono.data(),nullptr,mono.size());
 if(!rack->submit(overflow))return 6;
 std::cout<<"PASS callback allocation, multirate odd-block stress and bounded command queue\n";
}
