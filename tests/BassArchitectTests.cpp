#include "BassArchitect.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
using namespace masterlab;
static constexpr double pi=3.14159265358979323846;
static void check(bool ok,const char* name){if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);std::abort();}}
static double rms(const std::vector<float>& x,int start){
 double total=0;for(size_t i=start;i<x.size();++i)total+=static_cast<double>(x[i])*x[i];
 return std::sqrt(total/(x.size()-start));
}
static std::vector<float> render(double rate,BassArchitectSettings settings,int block,bool opposite=false){
 BassArchitect b;b.settings(settings);b.prepare(rate);
 const int length=static_cast<int>(rate*.4);
 std::vector<float> l(length),r(length);
 for(int i=0;i<length;++i){
  const float v=.22f*std::sin(2*pi*60.*i/rate)+.06f*std::sin(2*pi*3000.*i/rate);
  l[i]=v;r[i]=opposite?-v:v;
 }
 for(int i=0;i<length;i+=block)b.process(l.data()+i,r.data()+i,std::min(block,length-i));
 for(int i=0;i<length;++i)check(std::isfinite(l[i])&&std::isfinite(r[i]),"finite stereo");
 return l;
}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  BassArchitectSettings s;s.enabled=false;s.sub=1.f;s.body=1.f;s.harmonics=1.f;s.monoBass=1.f;
  const auto off=render(rate,s,127);
  const auto enabled0=off;
  s.enabled=true;s.sub=0.f;s.body=0.f;s.harmonics=0.f;s.monoBass=0.f;s.mix=1.f;
  const auto neutral=render(rate,s,127);
  for(size_t i=0;i<neutral.size();++i)check(std::abs(neutral[i]-enabled0[i])<1e-6f,"neutral should preserve dry");
  s.sub=1.f;s.body=.5f;s.harmonics=.7f;s.monoBass=.3f;
  const auto a=render(rate,s,1);const auto b=render(rate,s,127);const auto c=render(rate,s,4097);
  check(rms(b,4096)>rms(off,4096)*1.03,"measurable bass authority");
  for(size_t i=0;i<b.size();++i)check(std::abs(a[i]-b[i])<1e-6f&&std::abs(c[i]-b[i])<1e-6f,"block invariance");
  s.mix=0.f;
  const auto dry=render(rate,s,127);
  for(size_t i=0;i<dry.size();++i)check(std::abs(dry[i]-off[i])<1e-6f,"zero mix transparent");
  s.mix=1.f;s.sub=0;s.body=0;s.harmonics=0;s.monoBass=1.f;
  const auto collapsed=render(rate,s,127,true);
  check(rms(collapsed,4096)<rms(off,4096)*.6,"anti-phase low bass suppressed while high survives");
  std::printf("%.0f Bass Architect tests pass\n",rate);
 }
 BassArchitect e;BassArchitectSettings s;s.enabled=true;s.harmonics=1;e.settings(s);e.prepare(48000);
 std::array<float,512> l{},r{};
 l[0]=std::numeric_limits<float>::quiet_NaN();
 e.process(l.data(),r.data(),512);
 for(float v:l)check(std::isfinite(v),"invalid input handled");
 e.settings({});l.fill(.2f);r.fill(.2f);e.process(l.data(),r.data(),512);
 for(float v:l)check(std::isfinite(v),"bypass automation finite");
}
