#include "AnalogExciter.h"
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>
#include <cstdlib>
#include <new>
static bool auditing=false;static size_t allocations=0;
void* operator new(size_t n){if(auditing)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete[](void* p,size_t)noexcept{std::free(p);}

using namespace masterlab;
struct Render {std::vector<float> l,r;AnalogExciterReading reading;};
static Render render(float mix,bool delta,int block,double rate,bool active=true){
 AnalogExciter dsp;HarmonicExciterSettings h;h.enabled=active;h.amount={.2f,.5f,.3f,.1f};h.driveDb=8;
 AnalogizerSettings c;c.enabled=active;c.driveDb=6;c.warmth=.6f;
 dsp.harmonicSettings(h);dsp.colourSettings(c);dsp.monitor(mix,delta);dsp.prepare(rate);
 Render out;out.l.resize(24001);out.r.resize(out.l.size());
 for(size_t i=0;i<out.l.size();++i){out.l[i]=.18f*std::sin(6.283185307179586*997*i/rate);out.r[i]=-.7f*out.l[i];}
 auditing=true;
 for(int i=0;i<static_cast<int>(out.l.size());i+=block)dsp.process(out.l.data()+i,out.r.data()+i,std::min(block,static_cast<int>(out.l.size())-i));
 auditing=false;out.reading=dsp.reading();return out;
}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  const auto dry=render(0,false,257,rate),wet=render(1,false,257,rate),delta=render(1,true,257,rate),half=render(.5f,false,257,rate),off=render(1,true,257,rate,false);
  double nullError=0,mixError=0,authority=0;
  for(size_t i=0;i<dry.l.size();++i){
   const float input=.18f*std::sin(6.283185307179586*997*i/rate);
   if(dry.l[i]!=input||dry.r[i]!=-.7f*input||off.l[i]!=0||off.r[i]!=0)return 1;
   nullError=std::max(nullError,static_cast<double>(std::abs(delta.l[i]+wet.l[i]-input)));
   nullError=std::max(nullError,static_cast<double>(std::abs(delta.r[i]+wet.r[i]+.7f*input)));
   mixError=std::max(mixError,static_cast<double>(std::abs(half.l[i]-.5f*(wet.l[i]+input))));
   authority+=std::abs(delta.l[i]);
  }
  if(nullError>1.e-6||mixError>1.e-6||authority<1)return 2;
  for(int block:{1,17,128,4097}){
   const auto other=render(1,false,block,rate);
   for(size_t i=0;i<wet.l.size();++i)if(std::abs(other.l[i]-wet.l[i])>1.e-6||std::abs(other.r[i]-wet.r[i])>1.e-6)return 3;
   for(size_t i=0;i<wet.reading.inputWave.size();++i)if(std::abs(other.reading.inputWave[i]-wet.reading.inputWave[i])>1.e-6||std::abs(other.reading.outputWave[i]-wet.reading.outputWave[i])>1.e-6)return 4;
  }
  std::printf("rate %.0f dry/wet/delta null %.9f mix %.9f partition PASS\n",rate,nullError,mixError);
 }
 // Neutral defaults retain the exact existing chain audio.
 {
  AnalogExciter combined;HarmonicExciter harmonic;Analogizer colour;
  HarmonicExciterSettings h;h.enabled=true;h.amount={.1f,.3f,.2f,.2f};h.driveDb=8;
  AnalogizerSettings c;c.enabled=true;c.driveDb=5;c.warmth=.7f;
  combined.harmonicSettings(h);combined.colourSettings(c);combined.prepare(48000);
  harmonic.settings(h);harmonic.prepare(48000);colour.settings(c);colour.prepare(48000);
  for(int block=0;block<200;++block){
   std::array<float,128> a{},b{},x{},y{};
   for(int i=0;i<128;++i){a[i]=x[i]=.2f*std::sin((block*128+i)*.13f);b[i]=y[i]=-a[i];}
   combined.process(a.data(),b.data(),128);harmonic.process(x.data(),y.data(),128);colour.process(x.data(),y.data(),128);
   if(a!=x||b!=y){std::puts("FAIL neutral monitor changed chain audio");return 7;}
  }
  if(combined.reading().inputRms<.1f){std::puts("FAIL anti-phase RMS cancelled");return 8;}
 }
 // A monitor switch is smoothed on the sample clock, even for block size 1.
 {
  AnalogExciter smooth;smooth.prepare(48000);smooth.monitor(1,true);
  float a=.2f,b=-.2f;smooth.process(&a,&b,1);
  if(a<.199f||a>=.2f||b>-.199f)return 9;
  std::vector<float> l(24000,.2f),r(24000,-.2f);smooth.process(l.data(),r.data(),24000);
  if(std::abs(l.back())>1.e-5f||std::abs(r.back())>1.e-5f)return 10;
 }
 if(allocations!=0)return 11;
 AnalogExciter dsp;dsp.monitor(std::numeric_limits<float>::quiet_NaN(),true);dsp.prepare(48000);
 float l=.2f,r=-.2f;dsp.process(&l,&r,1);if(!std::isfinite(l)||!std::isfinite(r))return 5;
 dsp.reset();if(dsp.reading().inputRms!=0)return 6;
 std::puts("Analog combined monitor PASS");
}
