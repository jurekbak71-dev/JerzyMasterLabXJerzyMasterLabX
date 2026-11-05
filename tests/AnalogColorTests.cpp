#include "AnalogColor.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <limits>
using namespace masterlab;
constexpr double pi=3.14159265358979323846;
static void require(bool condition,const char* name){if(!condition){std::fprintf(stderr,"FAIL %s\n",name);std::abort();}}
static double metric(const std::vector<float>& v,int harmonic,double sr,double hz,int start){
 double re=0,im=0;for(int i=start;i<(int)v.size();++i){
  const double theta=2*pi*harmonic*hz*i/sr;re+=v[i]*std::cos(theta);im+=v[i]*std::sin(theta);}
 return 2*std::hypot(re,im)/(v.size()-start);
}
static std::vector<float> render(double rate,int mode,int block,bool on=true){
 AnalogColor effect;AnalogColorSettings s;s.enabled=on;s.mode=static_cast<AnalogColorMode>(mode);
 s.driveDb=18.f;s.bias=.35f;s.tone=.4f;s.dynamics=.6f;s.mix=.8f;
 effect.settings(s);effect.prepare(rate);
 const int frames=static_cast<int>(rate/2);
 std::vector<float> l(frames),r(frames);
 for(int i=0;i<frames;++i)l[i]=r[i]=.3f*std::sin(2*pi*997*i/rate);
 for(int i=0;i<frames;i+=block)effect.process(l.data()+i,r.data()+i,std::min(block,frames-i));
 for(int i=0;i<frames;++i)require(std::isfinite(l[i])&&l[i]==r[i],"finite dual mono");
 require(!on || effect.reading().deltaRms>.00001f,"actual nonlinear delta telemetry");
 return l;
}
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  std::array<std::vector<float>,4> results;
  for(int mode=0;mode<4;++mode){
   results[mode]=render(sr,mode,257);
   require(std::isfinite(metric(results[mode],2,sr,997,4096)),"finite even harmonic measurement");
   const auto single=render(sr,mode,1);
   const auto wide=render(sr,mode,4097);
   for(size_t i=0;i<single.size();++i)
    require(std::abs(single[i]-results[mode][i])<1e-6f
          && std::abs(wide[i]-results[mode][i])<1e-6f,"block independence");
  }
  for(int a=0;a<4;++a)for(int b=a+1;b<4;++b){
   double distance=0;
   for(size_t i=4096;i<results[a].size();++i){const double d=results[a][i]-results[b][i];distance+=d*d;}
   require(distance>1e-5,"models distinguishable");
  }
  const auto bypass=render(sr,0,257,false);
  for(size_t i=0;i<bypass.size();++i)
   require(std::abs(bypass[i]-.3f*std::sin(2*pi*997*i/sr))<1e-6f,"disabled exact pass");
  std::printf("%.0f Hz: four models distinct, bypass and block tests PASS\n",sr);
 }
 // Switching models must start from the time-aligned previous render and
 // finish at a measurably different sound on a non-DC stimulus.
 {
  AnalogColor e;AnalogColorSettings settings;settings.enabled=true;
  settings.mode=AnalogColorMode::Tube;settings.driveDb=18.f;settings.bias=.3f;
  settings.mix=1.f;e.settings(settings);e.prepare(48000);
  std::array<float,4096> l{},r{};
  for(size_t i=0;i<l.size();++i)l[i]=r[i]=.2f*std::sin(2*pi*997*i/48000);
  e.process(l.data(),r.data(),4096);
  AnalogColor reference=e;
  settings.mode=AnalogColorMode::SolidState;e.settings(settings);
  std::array<float,4096> wet{},wetR{},referenceL{},referenceR{};
  for(size_t i=0;i<wet.size();++i){
   const float x=.2f*std::sin(2*pi*997*(i+4096)/48000);
   wet[i]=wetR[i]=referenceL[i]=referenceR[i]=x;
  }
  e.process(wet.data(),wetR.data(),4096);
  reference.process(referenceL.data(),referenceR.data(),4096);
  require(std::abs(wet.front()-referenceL.front())<.002f,"model switch begins near old model");
  double changed=0.;
  for(size_t i=2048;i<wet.size();++i){
   const double d=wet[i]-referenceL[i];changed+=d*d;
   require(std::isfinite(wet[i])&&wet[i]==wetR[i],"model switch finite dual mono");
  }
  require(std::sqrt(changed/2048.)>.001,"switched model converges to distinct sound");
 }
 // Silence, invalid input and midstream automation.
 AnalogColor e;AnalogColorSettings s;s.enabled=true;s.driveDb=24;s.bias=.8f;s.mix=1;
 e.settings(s);e.prepare(48000);
 std::array<float,512> l{},r{};e.process(l.data(),r.data(),512);
 for(float x:l)require(x==0.f,"silence neutral");
 for(int mode=0;mode<4;++mode){
  s.mode=static_cast<AnalogColorMode>(mode);s.driveDb=mode*6.f;s.bias=mode*.2f;
  e.settings(s);l.fill(.1f);r.fill(.1f);e.process(l.data(),r.data(),512);
  for(float x:l)require(std::isfinite(x),"automation finite");
 }
 l.fill(0.f);r.fill(0.f);l[0]=std::numeric_limits<float>::quiet_NaN();
 e.process(l.data(),r.data(),512);for(float x:l)require(std::isfinite(x),"NaN protected");
 std::puts("AnalogColor DSP tests PASS");
}
