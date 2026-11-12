#include "FinalTruePeak.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
using namespace masterlab;
static constexpr double pi=3.14159265358979323846;
static void check(bool ok,const char* message){
 if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::abort();}
}
static double amplitude(const std::vector<float>& a,double rate,double hz){
 double re=0.,im=0.;const size_t offset=a.size()/2;
 for(size_t i=offset;i<a.size();++i){
  const double angle=2*pi*hz*i/rate;
  re+=a[i]*std::cos(angle);im+=a[i]*std::sin(angle);
 }
 return 2.*std::hypot(re,im)/(a.size()-offset);
}
// Independent 8x output reconstruction: 41-tap Blackman-windowed sinc FIR,
// explicitly distinct from the production 25-tap 4x input detector.
static double outputTruePeak8x(const std::vector<float>& a){
 constexpr int taps=41;
 double peak=0.;
 for(int phase=0;phase<8;++phase){
  double weights[taps]{},sum=0.;
  for(int j=0;j<taps;++j){
   const double t=j-(taps-1)/2.+phase/8.;
   const double sinc=std::abs(t)<1e-12?1.:std::sin(pi*t)/(pi*t);
   const double w=.42-.5*std::cos(2*pi*j/(taps-1))+.08*std::cos(4*pi*j/(taps-1));
   weights[j]=sinc*w;sum+=weights[j];
  }
  for(int j=0;j<taps;++j)weights[j]/=sum;
  // Exclude initial delay transient and final incomplete reconstruction.
  for(size_t i=256;i+256<a.size();++i){
   double v=0.;
   for(int j=0;j<taps;++j)v+=weights[j]*a[i-j+taps/2];
   peak=std::max(peak,std::abs(v));
  }
 }
 return peak;
}
static std::vector<float> render(double sr,FinalTruePeakSettings settings,int block,bool stress=false){
 const int count=static_cast<int>(sr*.4);
 FinalTruePeak limiter;limiter.settings(settings);limiter.prepare(sr);
 std::vector<float> left(count),right(count);
 for(int i=0;i<count;++i){
  const float sample=stress?
    .99f*static_cast<float>(std::sin(2*pi*13000.*i/sr)):
    .35f*static_cast<float>(std::sin(2*pi*991.*i/sr));
  left[i]=right[i]=sample;
 }
 for(int i=0;i<count;i+=block)
  limiter.process(left.data()+i,right.data()+i,std::min(block,count-i));
 for(size_t i=0;i<left.size();++i)
  check(std::isfinite(left[i])&&left[i]==right[i],"finite stereo linked");
 return left;
}
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  FinalTruePeakSettings s;s.enabled=false;s.driveDb=0;
  const auto delayed=render(sr,s,113);
  for(size_t i=FinalTruePeak::latencySamples;i<delayed.size();++i)
   check(std::abs(delayed[i]-.35f*std::sin(2*pi*991.*(i-FinalTruePeak::latencySamples)/sr))<1e-5f,"fixed 128-sample delay");
  s.enabled=true;s.ceilingDb=-3.f;s.driveDb=12.f;
  const auto strong=render(sr,s,113,true);
  const auto single=render(sr,s,1,true);
  const auto wide=render(sr,s,4097,true);
  const float ceiling=std::pow(10.f,-3.f/20.f);
  for(size_t i=0;i<strong.size();++i){
   check(std::abs(strong[i])<=ceiling+1e-6f,"sample ceiling never exceeded");
   check(std::abs(single[i]-strong[i])<1e-6f&&std::abs(wide[i]-strong[i])<1e-6f,"block invariant");
  }
  const auto max=*std::max_element(strong.begin(),strong.end(),
    [](float a,float b){return std::abs(a)<std::abs(b);});
  check(std::abs(max)>.1f,"nonzero signal not muted");
  // Verify inter-sample peaks independently, not just sample ceiling.
  const double outputTP=outputTruePeak8x(strong);
  std::printf("%.0f measured 8x output TP %.6f (sample ceiling %.6f)\\n",sr,outputTP,ceiling);
  check(outputTP<=ceiling*1.012,"reconstructed output true peak near ceiling");
  s.driveDb=0.f;
  const auto quiet=render(sr,s,257);
  check(amplitude(quiet,sr,991.)>.2,"quiet signal passes");
  std::printf("%.0f true-peak prototype: fixed latency, finite, peak bound, partition tests pass\n",sr);
 }
 // Expanded Maximizer controls must change the real signal but preserve
 // deterministic block processing and respect the requested output ceiling.
 {
  FinalTruePeakSettings style;style.enabled=true;style.ceilingDb=-6;style.driveDb=12;
  style.character=2;style.autoRelease=true;style.softClip=.75f;style.outputDb=2.f;
  const auto processed=render(48000,style,257,true);
  const auto single=render(48000,style,1,true);
  const auto wide=render(48000,style,4097,true);
  const float ceiling=std::pow(10.f,style.ceilingDb/20.f);
  for(size_t i=0;i<processed.size();++i){
   check(std::isfinite(processed[i])&&std::abs(processed[i])<=ceiling+1.e-6f,
         "expanded maximizer exceeded selected output ceiling");
   check(processed[i]==single[i]&&processed[i]==wide[i],
         "expanded maximizer depends on callback buffer size");
  }
  auto base=style;base.character=0;base.autoRelease=false;base.softClip=0;base.outputDb=0;
  const auto transparent=render(48000,base,257,true);
  double difference=0;for(size_t i=1024;i<processed.size();++i)difference+=std::abs(processed[i]-transparent[i]);
  check(difference>2.,"expanded maximizer controls do not affect actual audio");
  check(outputTruePeak8x(processed)<=ceiling*1.012,
        "expanded maximizer output violates independently reconstructed 8x ceiling");
 }
 // Advanced controls are DSP controls, not GUI-only state. Every detector
 // quality keeps the fixed latency and the independently reconstructed bound.
 for(int oversamplingMode=0;oversamplingMode<3;++oversamplingMode){
  FinalTruePeakSettings advanced;advanced.enabled=true;advanced.ceilingDb=-4;advanced.driveDb=12;
  advanced.attackMs=4;advanced.lookaheadSamples=48;advanced.transientEmphasis=.8f;advanced.oversamplingMode=oversamplingMode;
  const auto output=render(48000,advanced,113,true);
  const float ceiling=std::pow(10.f,advanced.ceilingDb/20.f);
  check(outputTruePeak8x(output)<=ceiling*1.012,"selectable oversampling violates reconstructed ceiling");
  auto neutral=advanced;neutral.attackMs=0;neutral.lookaheadSamples=128;neutral.transientEmphasis=0;
  const auto reference=render(48000,neutral,113,true);
  double difference=0;for(size_t i=1024;i<output.size();++i)difference+=std::abs(output[i]-reference[i]);
  check(difference>1.,"attack/lookahead/transient emphasis do not affect actual limiter audio");
 }
 FinalTruePeak dsp;FinalTruePeakSettings s;s.enabled=true;
 dsp.settings(s);dsp.prepare(48000);
 std::array<float,512> a{},b{};a[0]=std::numeric_limits<float>::quiet_NaN();
 dsp.process(a.data(),b.data(),512);
 for(float x:a)check(std::isfinite(x),"nonfinite input safe");
}
