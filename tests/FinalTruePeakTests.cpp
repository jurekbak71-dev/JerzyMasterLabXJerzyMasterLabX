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
  s.driveDb=0.f;
  const auto quiet=render(sr,s,257);
  check(amplitude(quiet,sr,991.)>.2,"quiet signal passes");
  std::printf("%.0f true-peak prototype: fixed latency, finite, peak bound, partition tests pass\n",sr);
 }
 FinalTruePeak dsp;FinalTruePeakSettings s;s.enabled=true;
 dsp.settings(s);dsp.prepare(48000);
 std::array<float,512> a{},b{};a[0]=std::numeric_limits<float>::quiet_NaN();
 dsp.process(a.data(),b.data(),512);
 for(float x:a)check(std::isfinite(x),"nonfinite input safe");
}
