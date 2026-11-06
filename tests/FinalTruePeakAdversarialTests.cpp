#include "FinalTruePeak.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using namespace masterlab;
static constexpr double pi=3.14159265358979323846;
static constexpr int taps=49,phases=8;
struct OutputTruePeakMeter {
 std::array<std::array<double,taps>,phases> weights{};
 OutputTruePeakMeter() {
  for(int p=0;p<phases;++p){
   double sum=0.;
   for(int j=0;j<taps;++j){
    const double x=(j-(taps-1)/2.)+p/static_cast<double>(phases);
    const double sinc=std::abs(x)<1e-12?1.:std::sin(pi*x)/(pi*x);
    const double window=.42-.5*std::cos(2*pi*j/(taps-1))+.08*std::cos(4*pi*j/(taps-1));
    weights[p][j]=sinc*window;sum+=weights[p][j];
   }
   for(double& x:weights[p])x/=sum;
  }
 }
 double peak(const std::vector<float>& data)const {
  double maximum=0.;
  for(size_t i=256;i+256<data.size();++i)for(int p=0;p<phases;++p){
   double sample=0.;
   for(int j=0;j<taps;++j)sample+=weights[p][j]*data[i+j-taps/2];
   maximum=std::max(maximum,std::abs(sample));
  }
  return maximum;
 }
};
static void check(bool ok,const char* text){
 if(!ok){std::fprintf(stderr,"FAIL: %s\n",text);std::abort();}
}
static std::vector<float> render(int rate,int stimulus,int mode,FinalTruePeakReading& reading){
 constexpr int n=8192;
 std::vector<float> left(n),right(n);
 std::uint32_t rng=0x7a31c96du;
 auto randomSample=[&](){
  rng=rng*1664525u+1013904223u;
  return static_cast<float>((static_cast<double>(rng)/4294967295.)*2.-1.);
 };
 for(int i=0;i<n;++i){
  const double t=static_cast<double>(i)/rate;
  const double f=.45*rate;
  double v=0.;
  switch(stimulus){
   case 0:v=.8*std::sin(2*pi*f*t+pi/4.);break; // near Nyquist, intersample
   case 1:v=(i&1)?-1.:1.;break; // alternating
   case 2:v=(i%257)<128?1.:-1.;break; // hard square
   case 3:v=(i%251)==0?2.:0.;break; // sparse impulses
   case 4:v=(i%577)==0?2.:-.35;break; // DC/impulse discontinuity
   case 5:v=.7*std::sin(2*pi*rate*.171*t)+.7*std::sin(2*pi*rate*.314*t);break;
   case 6:v=randomSample();break;
   case 7:v=.62*std::sin(2*pi*1200*t)+.6*std::sin(2*pi*f*t);break;
  }
  left[i]=static_cast<float>(v);
  right[i]=static_cast<float>(-v*.77);
 }
 FinalTruePeak limiter;
 FinalTruePeakSettings settings;settings.enabled=true;settings.ceilingDb=-3.f;
 settings.driveDb=mode==0?0.f:mode==1?6.f:18.f;settings.releaseMs=mode==2?10.f:150.f;
 limiter.settings(settings);limiter.prepare(rate);
 for(int i=0;i<n;i+=257)
  limiter.process(left.data()+i,right.data()+i,std::min(257,n-i));
 reading=limiter.reading();
 for(int i=0;i<n;++i)
  check(std::isfinite(left[i])&&std::isfinite(right[i]),"nonfinite output");
 return left;
}
int main(){
 OutputTruePeakMeter independent;
 const double ceiling=std::pow(10.,-3./20.);
 double worstExcess=-100.;
 int worstRate=0,worstStimulus=0,worstMode=0;
 int measured=0;
 for(int rate:{44100,48000,96000,192000}){
  for(int stimulus=0;stimulus<8;++stimulus)for(int mode=0;mode<3;++mode){
   FinalTruePeakReading reading{};
   auto output=render(rate,stimulus,mode,reading);
   const double peak=independent.peak(output);
   const double excess=20*std::log10(std::max(peak,1e-12)/ceiling);
   if(excess>worstExcess){
    worstExcess=excess;worstRate=rate;worstStimulus=stimulus;worstMode=mode;
   }
   const float samplePeak=*std::max_element(output.begin(),output.end(),
     [](float a,float b){return std::abs(a)<std::abs(b);});
   check(std::abs(samplePeak)<=ceiling+1e-6,"sample peak exceeds declared ceiling");
   check(peak<10.,"independent 8x meter unstable");
   ++measured;
  }
 }
 std::printf("Independent 8x 49-tap true-peak stress cases: %d, worst excess %+0.4f dB, rate %d, signal %d, mode %d\n",
             measured,worstExcess,worstRate,worstStimulus,worstMode);
 // Diagnostic audit, not an unearned BS.1770/EBU compliance claim.
 // A positive excess is a documented failing of the intended true-peak ceiling,
 // even though the sample-peak limit is enforced.
 check(worstExcess<=0.05,"independent 8x output peak exceeds +0.05 dBTP tolerance");
 if(worstExcess>0.)
  std::printf("AUDIT NOTICE: tolerance remains below +0.05 dBTP; certification is separate\\n");
}
