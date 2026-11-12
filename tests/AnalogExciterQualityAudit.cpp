#include "AnalogExciter.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

constexpr double pi=3.14159265358979323846;
static double component(const std::vector<float>& x,double hz,double sr,int start){
 double re=0,im=0;for(int i=start;i<static_cast<int>(x.size());++i){const double a=2*pi*hz*i/sr;re+=x[i]*std::cos(a);im+=x[i]*std::sin(a);}
 return 2*std::hypot(re,im)/(x.size()-start);
}
static std::vector<float> render(masterlab::AnalogizerMode mode,float excite,double frequency,bool colourOn=true,bool harmonicOn=true){
 constexpr double rate=48000;constexpr int size=48000;
 masterlab::AnalogExciter dsp;masterlab::AnalogizerSettings colour;colour.enabled=colourOn;colour.mode=mode;colour.driveDb=12;colour.warmth=.65f;colour.dynamics=.35f;
 masterlab::HarmonicExciterSettings harmonic;harmonic.enabled=harmonicOn;harmonic.amount={.25f,.45f,.3f,.2f};harmonic.even=.6f;harmonic.odd=.7f;harmonic.driveDb=8;harmonic.adaa=true;
 dsp.colourSettings(colour);dsp.harmonicSettings(harmonic);dsp.excitement(excite);dsp.prepare(rate);
 std::vector<float> left(size),right(size);for(int i=0;i<size;++i)left[i]=right[i]=.2f*std::sin(2*pi*frequency*i/rate);
 for(int i=0;i<size;i+=257)dsp.process(left.data()+i,right.data()+i,std::min(257,size-i));return left;
}
int main(){
 std::array<std::array<double,5>,3> signature{};
 for(int mode=0;mode<3;++mode){
  const auto signal=render(static_cast<masterlab::AnalogizerMode>(mode),.55f,1000);
  for(int harmonic=1;harmonic<=5;++harmonic)signature[mode][harmonic-1]=component(signal,1000.*harmonic,48000,24000);
  std::printf("mode %d H1-H5 %.8f %.8f %.8f %.8f %.8f THD2-5 %.8f\n",mode,signature[mode][0],signature[mode][1],signature[mode][2],signature[mode][3],signature[mode][4],
   std::hypot(std::hypot(signature[mode][1],signature[mode][2]),std::hypot(signature[mode][3],signature[mode][4]))/signature[mode][0]);
  if(signature[mode][0]<.01||signature[mode][1]<1e-5||signature[mode][2]<1e-5)return 1;
 }
 for(int a=0;a<3;++a)for(int b=a+1;b<3;++b){double distance=0;for(int h=1;h<5;++h)distance+=std::abs(signature[a][h]-signature[b][h]);if(distance<1e-4)return 2;}
 const auto low=render(masterlab::AnalogizerMode::Tube,0,1000),high=render(masterlab::AnalogizerMode::Tube,1,1000);
 double authority=0;for(size_t i=24000;i<low.size();++i)authority+=std::abs(low[i]-high[i]);authority/=24000;
 if(authority<.005)return 3;std::printf("EXCITE mean absolute authority %.8f\n",authority);
 bool aliasGate=true;
 for(int mode=0;mode<3;++mode){const auto highTone=render(static_cast<masterlab::AnalogizerMode>(mode),.55f,10000);const double fundamental=component(highTone,10000,48000,24000),alias=component(highTone,18000,48000,24000);
  std::printf("mode %d 10k fundamental %.8f, 30k->18k alias %.8f, alias ratio %.8f\n",mode,fundamental,alias,alias/std::max(1e-12,fundamental));
  aliasGate=alias/fundamental<.05&&aliasGate;
  for(auto path:{std::pair{true,false},std::pair{false,true}}){const auto isolated=render(static_cast<masterlab::AnalogizerMode>(mode),0.f,10000,path.first,path.second);
   std::printf("  isolated colour=%d harmonics=%d alias ratio %.8f\n",path.first,path.second,component(isolated,18000,48000,24000)/std::max(1e-12,component(isolated,10000,48000,24000)));
  }
 }
 if(!aliasGate){std::puts("FAIL combined Analog Exciter alias ratio exceeds 0.05 audit gate");return 4;}
 // Maximum harmonic controls must remain a bounded colour contribution rather
 // than creating a second clipper before the Final Maximizer.
 {
  constexpr int n=48000;std::vector<float> left(n),right(n);
  masterlab::AnalogExciter dsp;
  masterlab::HarmonicExciterSettings harmonic;harmonic.enabled=true;
  harmonic.amount={1.f,1.f,1.f,1.f};harmonic.even=1.f;harmonic.odd=1.f;
  harmonic.driveDb=24.f;harmonic.impact=1.f;harmonic.mix=1.f;harmonic.outputDb=0.f;
  harmonic.adaa=true;
  dsp.harmonicSettings(harmonic);dsp.prepare(48000.);
  for(int i=0;i<n;++i){
   const float x=.48f*std::sin(2*pi*997*i/48000.)+.32f*std::sin(2*pi*7013*i/48000.);
   left[i]=right[i]=x;
  }
  for(int i=0;i<n;i+=127)dsp.process(left.data()+i,right.data()+i,std::min(127,n-i));
  float peak=0.f;int plateaus=0;
  for(int i=4096;i<n;++i){peak=std::max(peak,std::abs(left[i]));if(std::abs(left[i])>15.999f)++plateaus;}
  std::printf("maximum harmonic stress peak %.6f, clamp plateaus %d\n",peak,plateaus);
  if(peak>2.f||plateaus!=0){std::puts("FAIL Harmonic Exciter creates uncontrolled pre-maximizer peaks");return 5;}
 }
}
