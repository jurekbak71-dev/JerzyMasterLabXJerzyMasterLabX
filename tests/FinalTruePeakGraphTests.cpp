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
 // A dedicated True-Peak Maximizer must not serially trigger the masked
 // historical sample-peak limiter. The older engine still supplies PDC.
 {
  std::array<float,parameterCount> params{};
  params[5]=params[87]=1.f; // old limiter ON in an existing session
  params[221]=params[222]=1.f;
  params[223]=-6.f;params[225]=12.f;
  params[161]=0.f; // validated full serial routing, TP fixed last
  SerialGraph graph;
  configureGraph(graph,[&](size_t i){return params[i];});
  graph.prepare(48000);
  std::array<float,128> left{},right{},trace{};
  for(int block=0;block<120;++block){
   for(size_t i=0;i<left.size();++i){
    const float x=.8f*static_cast<float>(std::sin(2*pi*997.*(block*128+i)/48000.));
    left[i]=right[i]=x;trace[i]=0.f;
   }
   graph.process(left.data(),right.data(),128,false,trace.data());
   if(block>20)for(float gain:trace)
    check(std::abs(gain-1.f)<1.e-4f,
          "legacy limiter still compresses ahead of dedicated Maximizer");
  }
 }
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
