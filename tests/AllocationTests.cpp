#include "WindowMeter.h"
#include "LoudnessMeter.h"
#include <cstdlib>
#include <new>
#include <iostream>
static size_t allocations=0;
void* operator new(std::size_t n){++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
int main(){
 masterlab::RoutedEngine engine;masterlab::WindowMeter meter;masterlab::LoudnessMeter loudness;
 engine.prepare(48000);meter.prepare(48000);loudness.prepare(48000);
 std::array<float,257> left{},right{},trace{};const auto before=allocations;
 for(int block=0;block<1000;++block){
  for(size_t i=0;i<left.size();++i){left[i]=.1f*std::sin((block*257+i)*.17f);right[i]=-left[i];}
  engine.setBandMode(0,static_cast<masterlab::FilterType>(block%6),block%2==0);engine.process(left.data(),right.data(),257,block%2==0,trace.data());
  for(size_t i=0;i<left.size();++i){meter.push(left[i],right[i]);loudness.push(left[i],right[i]);}
  meter.read();loudness.read();if(block==500){meter.reset();loudness.reset();}
 }
 if(allocations!=before){std::cerr<<"allocation during core DSP/meter processing or reset\n";return 1;}
 std::cout<<"Passed: no C++ heap allocation in prepared DSP, routing, EQ switching, window/loudness metering or meter reset\n";
}
