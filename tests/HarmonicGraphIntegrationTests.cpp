#include "SerialGraph.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace masterlab;
namespace {
void check(bool condition,const char* name) {
 if(!condition){std::fprintf(stderr,"FAIL: %s\n",name);std::abort();}
}
std::vector<float> render(double rate,int block,bool parallel,bool enabled) {
 constexpr int n=32768;
 SerialGraph graph;
 graph.prepare(rate);
 graph.parameters(1000.f,0.f,1.f,-1.f,false,false);
 graph.topology(parallel,{false,false,false,true,false,false},
                       {false,false,false,false,false,false});
 HarmonicExciterSettings h;
 h.enabled=enabled;h.amount={1.f,1.f,1.f,1.f};h.even=1.f;h.odd=1.f;h.driveDb=12.f;
 graph.harmonicExciterSettings(h);
 std::vector<float> left(n),right(n);
 for(int i=0;i<n;++i){
  const float x=.2f*std::sin(6.283185307179586*1000.*i/rate);
  left[i]=x;right[i]=x;
 }
 std::vector<float> trace(static_cast<size_t>(block));
 for(int i=0;i<n;i+=block)
  graph.process(left.data()+i,right.data()+i,std::min(block,n-i),false,trace.data());
 for(int i=0;i<n;++i)check(std::isfinite(left[i])&&left[i]==right[i],"finite symmetric output");
 if(enabled)check(graph.harmonicExciterRead().outputRms>0.01f,"real harmonic activity telemetry");
 return left;
}
}
int main() {
 for(double rate:{44100.,48000.,96000.,192000.}) {
  for(bool parallel:{false,true}) {
   auto legacy=render(rate,257,parallel,false);
   auto processed=render(rate,257,parallel,true);
   double energy=0,delta=0;
   for(int i=4096;i<(int)legacy.size();++i){
    energy+=static_cast<double>(legacy[i])*legacy[i];
    const double difference=processed[i]-legacy[i];
    delta+=difference*difference;
   }
   check(energy>1e-5,"reference graph has audio");
   check(delta/energy>0.0001,"connected exciter changes actual graph output");
   auto single=render(rate,1,parallel,true);
   auto large=render(rate,4097,parallel,true);
   for(size_t i=0;i<single.size();++i)check(std::abs(single[i]-large[i])<1e-6f,"graph block independence");
   std::printf("%g Hz %s: delta/RMS %f; block test pass\n",
               rate,parallel?"parallel":"serial",std::sqrt(delta/energy));
  }
 }
}
