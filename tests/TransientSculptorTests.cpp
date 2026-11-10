#include "TransientSculptor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using namespace masterlab;
constexpr double pi=3.14159265358979323846;
static void check(bool ok,const char* label){if(!ok){std::fprintf(stderr,"FAIL: %s\n",label);std::abort();}}
static std::vector<float> render(double sr,TransientSculptorSettings s,int block){
 TransientSculptor dsp;dsp.settings(s);dsp.prepare(sr);
 const int n=static_cast<int>(sr*.5);
 std::vector<float> a(n),b(n);
 for(int i=0;i<n;++i){
  const float impulse=(i%1200<12)? .64f*std::exp(-.15f*static_cast<float>(i%1200)):0.f;
  a[i]=b[i]=impulse+.11f*std::sin(2.*pi*440.*i/sr);
 }
 for(int i=0;i<n;i+=block)dsp.process(a.data()+i,b.data()+i,std::min(block,n-i));
 for(int i=0;i<n;++i)check(std::isfinite(a[i])&&a[i]==b[i],"stereo finite");
 return a;
}
static double distance(const std::vector<float>& a,const std::vector<float>& b){
 double sum=0;for(size_t i=4096;i<a.size();++i){const double d=a[i]-b[i];sum+=d*d;}
 return std::sqrt(sum/(a.size()-4096));
}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  TransientSculptorSettings p;
  p.enabled=false;p.attack={1.f,1.f,1.f};p.sustain={1.f,1.f,1.f};
  auto dry=render(rate,p,257);
  p.enabled=true;p.attack={};p.sustain={};
  auto flat=render(rate,p,257);
  check(distance(flat,dry)<1e-7,"enabled neutral exactly dry");
  p.attack={1.f,1.f,1.f};auto stronger=render(rate,p,257);
  check(distance(stronger,dry)>.003,"strong attack produces meaningful delta");
  p.attack={-1.f,-1.f,-1.f};auto softer=render(rate,p,257);
  check(distance(stronger,softer)>.008,"attack polarity changes sound");
  auto single=render(rate,p,1),wide=render(rate,p,4097);
  for(size_t i=0;i<single.size();++i)
   check(std::abs(single[i]-softer[i])<1e-6f&&std::abs(wide[i]-softer[i])<1e-6f,"block independence");
  p.mix=0.f;auto bypass=render(rate,p,257);
  check(distance(bypass,dry)<1e-7,"wet zero equals dry");
  std::printf("%.0f Hz transient sculptor passed\n",rate);
 }
 // Delta and scope are calculated from the actual processed samples.
 {
  TransientSculptorSettings cfg;cfg.enabled=true;cfg.attack={.85f,.65f,.75f};cfg.sustain={-.3f,.3f,.4f};
  auto monitoring=cfg;monitoring.delta=true;
  TransientSculptor normal,delta;normal.settings(cfg);delta.settings(monitoring);
  normal.prepare(48000);delta.prepare(48000);
  std::array<float,257> a{},b{},c{},d{},raw{};
  double totalDifference=0;
  for(int block=0;block<120;++block){
   for(int i=0;i<257;++i){
    const int time=block*257+i;
    const float v=.16f*std::sin(time*.19f)+(time%997<14?.75f:0.f);
    a[i]=b[i]=c[i]=d[i]=raw[i]=v;
   }
   normal.process(a.data(),b.data(),257);delta.process(c.data(),d.data(),257);
   for(int i=0;i<257;++i){
    check(std::abs(c[i]-(raw[i]-a[i]))<1.e-5f,"Transient Delta differs from processed-minus-input");
    totalDifference+=std::abs(a[i]-raw[i]);
   }
  }
  check(totalDifference>1.,"Transient controls have no audible action");
  const auto reading=normal.reading();
  check(std::any_of(reading.outputWave.begin(),reading.outputWave.end(),[](float x){return std::abs(x)>.001f;}),
        "Transient output waveform is not actual DSP telemetry");
 }
 TransientSculptor d;TransientSculptorSettings p;p.enabled=true;p.attack={1,1,1};
 d.settings(p);d.prepare(48000);
 std::array<float,512> a{},b{};a[0]=std::numeric_limits<float>::quiet_NaN();
 d.process(a.data(),b.data(),512);
 for(float v:a)check(std::isfinite(v),"NaN safe");
 p.enabled=false;d.settings(p);d.process(a.data(),b.data(),512);
 for(float v:a)check(std::isfinite(v),"automation safe");
}
