#include "GraphConfiguration.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace masterlab;
static constexpr double pi=3.14159265358979323846;
static void check(bool ok,const char* name){if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::abort();}}
static std::vector<float> render(bool enable,bool parallel,int block,int rate){
 SerialGraph graph;graph.prepare(rate);
 graph.parameters(1000.f,0.f,1.f,-1.f,false,false);
 graph.topology(parallel,{false,false,false,false,false,false});
 FinalTruePeakSettings tp;tp.enabled=enable;tp.ceilingDb=-6.f;tp.driveDb=14.f;tp.releaseMs=70.f;
 graph.finalTruePeakSettings(tp);
 check(graph.latency()>=FinalTruePeak::latencySamples,"graph reports fixed final PDC");
 const int length=rate/2;
 std::vector<float> left(length),right(length),trace(std::max(block,1));
 for(int i=0;i<length;++i){
  const float x=.35f*std::sin(2*pi*991*i/rate)+.22f*std::sin(2*pi*11200*i/rate);
  left[i]=right[i]=x;
 }
 for(int i=0;i<length;i+=block)graph.process(left.data()+i,right.data()+i,std::min(block,length-i),false,trace.data());
 for(float x:left)check(std::isfinite(x),"finite graph");
 return left;
}
static double difference(const std::vector<float>& a,const std::vector<float>& b){
 double sum=0;for(size_t i=4096;i<a.size();++i){double d=a[i]-b[i];sum+=d*d;}return std::sqrt(sum/(a.size()-4096));
}
int main(){
 for(int rate:{44100,48000,96000,192000})for(bool parallel:{false,true}){
  const auto off=render(false,parallel,257,rate),on=render(true,parallel,257,rate);
  check(difference(off,on)>.03,"final limiter changes graph output audibly");
  const auto small=render(true,parallel,1,rate),wide=render(true,parallel,4097,rate);
  for(size_t i=0;i<on.size();++i)
   check(std::abs(on[i]-small[i])<1e-6f&&std::abs(on[i]-wide[i])<1e-6f,"graph block independence");
  const float ceiling=std::pow(10.f,-6.f/20.f);
  for(float x:on)check(std::abs(x)<ceiling+1e-6f,"last-stage ceiling");
  std::printf("rate %d mode %s final stage passed\n",rate,parallel?"parallel":"serial");
 }
}
