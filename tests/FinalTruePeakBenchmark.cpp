#include "FinalTruePeak.h"
#include "FinalTruePeakReference.h"
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace masterlab;
template<class Limiter,class Settings>
double bench(int rate,int blocks){
 Limiter limiter;Settings settings{};settings.enabled=true;settings.ceilingDb=-1.f;settings.driveDb=9.f;
 limiter.settings(settings);limiter.prepare(rate);
 std::array<float,256> left{},right{};
 volatile double sink=0.;
 auto start=std::chrono::steady_clock::now();
 for(int b=0;b<blocks;++b){
  for(int i=0;i<256;++i){
   const double t=(static_cast<double>(b)*256+i)/rate;
   left[i]=static_cast<float>(.9*std::sin(2*3.141592653589793*3200*t));
   right[i]=static_cast<float>(.4*std::cos(2*3.141592653589793*8700*t));
  }
  limiter.process(left.data(),right.data(),256);
  sink=sink+left[(b*7)%256];
 }
 auto finish=std::chrono::steady_clock::now();
 if(!std::isfinite(sink))std::abort();
 return std::chrono::duration<double,std::nano>(finish-start).count()/(blocks*256.);
}
int main(){
 for(int rate:{44100,48000,96000}){
  constexpr int blocks=2000;
  double oldNs=bench<masterlab_reference::FinalTruePeak,masterlab_reference::FinalTruePeakSettings>(rate,blocks);
  double newNs=bench<masterlab::FinalTruePeak,masterlab::FinalTruePeakSettings>(rate,blocks);
  std::printf("%d Hz, old %.2f ns/sample, new %.2f ns/sample, speedup %.3fx\n",
    rate,oldNs,newNs,oldNs/newNs);
 }
}
