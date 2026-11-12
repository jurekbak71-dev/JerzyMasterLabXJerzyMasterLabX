#include "AudioRestoration.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
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

 // Independent De-Hiss quality fixture: learn deterministic broadband noise,
 // freeze the print, then require a real SNR improvement without erasing tone.
 {
  constexpr double sr=48000.;constexpr int blockSize=257;
  RestorationSettings learn;learn.enabled=true;learn.learn=true;learn.mix=1.f;
  AudioRestoration normal,delta;normal.settings(learn);delta.settings(learn);normal.prepare(sr);delta.prepare(sr);
  std::uint32_t rng=0x5a17d3c1u;
  auto noise=[&](){rng=rng*1664525u+1013904223u;return .018f*(static_cast<float>(rng>>8)/8388608.f-1.f);};
  for(int offset=0;offset<static_cast<int>(sr*2);offset+=blockSize){
   const int n=std::min(blockSize,static_cast<int>(sr*2)-offset);std::array<float,blockSize> l{},r{};
   for(int i=0;i<n;++i)l[i]=r[i]=noise();normal.process(l.data(),r.data(),n);
  }
  const float learnedLevel=normal.reading().noisePrintLevel;require(learnedLevel>1.e-5f,"broadband print was not learned");
  RestorationSettings active=learn;active.learn=false;active.freeze=true;active.hiss=true;active.reductionDb=15;active.protect=.35f;
  normal.settings(active);delta.settings(active);delta.prepare(sr);
  // Give the DELTA instance the identical print rather than comparing unlike state.
  rng=0x5a17d3c1u;RestorationSettings trainDelta=learn;delta.settings(trainDelta);
  for(int offset=0;offset<static_cast<int>(sr*2);offset+=blockSize){const int n=std::min(blockSize,static_cast<int>(sr*2)-offset);std::array<float,blockSize> l{},r{};for(int i=0;i<n;++i)l[i]=r[i]=noise();delta.process(l.data(),r.data(),n);}active.delta=true;delta.settings(active);
  rng=0x93bc771du;double dryNoise=0,wetEnergy=0,wetSin=0,wetCos=0,nullError=0;std::uint64_t measured=0;
  for(int offset=0;offset<static_cast<int>(sr*3);offset+=blockSize){
   const int n=std::min(blockSize,static_cast<int>(sr*3)-offset);std::array<float,blockSize> a{},b{},d{},e{},original{};
   for(int i=0;i<n;++i){const int sample=offset+i;const float tone=.12f*std::sin(2*pi*997*sample/sr),x=tone+noise();a[i]=b[i]=d[i]=e[i]=original[i]=x;}
   normal.process(a.data(),b.data(),n);delta.process(d.data(),e.data(),n);
   if(offset>static_cast<int>(sr))for(int i=0;i<n;++i){const int sample=offset+i;const double angle=2*pi*997*sample/sr,tone=.12*std::sin(angle);dryNoise+=std::pow(original[i]-tone,2);wetEnergy+=static_cast<double>(a[i])*a[i];wetSin+=a[i]*std::sin(angle);wetCos+=a[i]*std::cos(angle);++measured;nullError=std::max(nullError,std::abs(static_cast<double>(a[i]+d[i]-original[i])));}
  }
  const double wetAmplitude=2*std::hypot(wetSin,wetCos)/measured;
  const double wetNoise=std::max(1.e-20,wetEnergy-measured*.5*wetAmplitude*wetAmplitude);
  const double snrGain=10*std::log10(dryNoise/wetNoise),toneChange=20*std::log10(wetAmplitude/.12);
  std::printf("Restoration frozen-print SNR gain %.3f dB, tone change %.3f dB, delta null %.9f\n",snrGain,toneChange,nullError);
  require(snrGain>2.,"De-Hiss does not improve deterministic broadband SNR by 2 dB");
  require(toneChange>-.8&&toneChange<.8,"De-Hiss changes protected tone by more than 0.8 dB");
  require(nullError<2.e-5,"Restoration DELTA is not input minus processed output");
 }
 require(allocations==0,"realtime callback allocated memory");
 std::puts("Audio Restoration: hum, learned hiss, impulse repair, de-clip, bypass, no allocation PASS");
}
