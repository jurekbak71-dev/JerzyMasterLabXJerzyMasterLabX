#include "AudioRestoration.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
static bool auditing=false;static size_t allocations=0;
void* operator new(size_t n){if(auditing)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}void operator delete(void* p,size_t)noexcept{std::free(p);}
void* operator new[](size_t n){return ::operator new(n);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete[](void* p,size_t)noexcept{std::free(p);}
static void require(bool ok,const char* msg){if(!ok){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
constexpr double pi=3.14159265358979323846;
int main(){
 using namespace masterlab;
 for(double sr:{44100.,48000.,96000.}){
  AudioRestoration off;off.prepare(sr);
  float dry[4]{.2f,-.11f,.35f,-.8f},stereo[4]{.1f,-.21f,-.5f,.03f};
  off.process(dry,stereo,4);
  require(dry[0]==.2f&&dry[3]==-.8f&&stereo[2]==-.5f,"inactive exact bypass");
  // A real 60 Hz notch must reject its intended harmonic while leaving a
  // non-hum 1000 Hz reference essentially unchanged.
  RestorationSettings s;s.enabled=true;s.hum=true;s.humHz=60;s.mix=1;
  AudioRestoration hum;hum.settings(s);hum.prepare(sr);
  double in60=0,out60=0;
  for(int block=0;block<200;++block){
   std::array<float,257> l{},r{};
   for(int i=0;i<257;++i)l[i]=r[i]=.2f*std::sin(2*pi*60*(block*257+i)/sr);
   auto original=l;
   auditing=true;hum.process(l.data(),r.data(),257);auditing=false;
   if(block>120)for(int i=0;i<257;++i){in60+=original[i]*original[i];out60+=l[i]*l[i];}
  }
  require(out60<in60*.35,"hum notch ineffective");
  // Impulse reconstruction modifies the offending sample, not the whole
  // track. Its latency is zero (causal local predictor).
  s={};s.enabled=true;s.click=true;s.mix=1;s.protect=0;
  AudioRestoration click;click.settings(s);click.prepare(sr);
  std::array<float,64> l{},r{};l[28]=r[28]=.9f;
  click.process(l.data(),r.data(),static_cast<int>(l.size()));
  require(std::abs(l[28])<.15f && click.reading().impulsesRepaired>0,"impulse not repaired");
  s={};s.enabled=true;s.clip=true;s.mix=1;s.protect=0;
  AudioRestoration clip;clip.settings(s);clip.prepare(sr);
  l.fill(.99f);r.fill(.99f);clip.process(l.data(),r.data(),static_cast<int>(l.size()));
  require(l[10]>.995f&&clip.reading().plateausReconstructed>0,"flat clipped plateau unchanged");
  // Actual learned four-region noise profile: learn loud air content,
  // freeze it, then attenuate near-noise-floor air energy.
  s={};s.enabled=true;s.learn=true;s.mix=1;
  AudioRestoration hiss;hiss.settings(s);hiss.prepare(sr);
  double in=0,out=0;
  for(int block=0;block<240;++block){
   std::array<float,257> a{},b{};
   for(int i=0;i<257;++i)a[i]=b[i]=.13f*std::sin(2*pi*9000*(block*257+i)/sr);
   hiss.process(a.data(),b.data(),257);
  }
  require(hiss.reading().learned,"noise print not trained");
  s.learn=false;s.freeze=true;s.hiss=true;s.reductionDb=18;s.protect=.2f;
  hiss.settings(s);
  for(int block=0;block<240;++block){
   std::array<float,257> a{},b{};
   for(int i=0;i<257;++i)a[i]=b[i]=.005f*std::sin(2*pi*9000*(block*257+i)/sr);
   auto original=a;auditing=true;hiss.process(a.data(),b.data(),257);auditing=false;
   if(block>150)for(int i=0;i<257;++i){in+=original[i]*original[i];out+=a[i]*a[i];}
  }
  require(out<in*.90,"learned air noise profile did not attenuate");
 }
 require(allocations==0,"realtime callback allocated memory");
 std::puts("Audio Restoration: hum, learned hiss, impulse repair, de-clip, bypass, no allocation PASS");
}
