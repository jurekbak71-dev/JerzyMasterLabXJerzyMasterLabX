#include "HarmonicExciter.h"
#include "AnalogColor.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace masterlab;
constexpr double pi=3.14159265358979323846;
static void require(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL %s\n",what);std::abort();}}
static double delta(double rate,bool exciter,float impact,int block){
 const int frames=static_cast<int>(rate);
 std::vector<float> original(frames),l(frames),r(frames);
 for(int i=0;i<frames;++i){
  original[i]=.18f*static_cast<float>(std::sin(2*pi*997.*i/rate)+.3*std::sin(2*pi*3500.*i/rate));
  l[i]=r[i]=original[i];
 }
 if(exciter){
  HarmonicExciter e;HarmonicExciterSettings s;s.enabled=true;s.amount={1.f,1.f,1.f,1.f};
  s.even=.8f;s.odd=.8f;s.driveDb=15.f;s.impact=impact;
  e.settings(s);e.prepare(rate);
  for(int i=0;i<frames;i+=block)e.process(l.data()+i,r.data()+i,std::min(block,frames-i));
 }else{
  AnalogColor e;AnalogColorSettings s;s.enabled=true;s.driveDb=16.f;s.bias=.3f;
  s.tone=.2f;s.dynamics=.5f;s.mix=1.f;s.autoGain=true;s.impact=impact;
  e.settings(s);e.prepare(rate);
  for(int i=0;i<frames;i+=block)e.process(l.data()+i,r.data()+i,std::min(block,frames-i));
 }
 double sum=0;for(int i=frames/4;i<frames;++i){
  require(std::isfinite(l[i])&&l[i]==r[i],"finite dual mono");
  const double d=l[i]-original[i];sum+=d*d;
 }
 return std::sqrt(sum/(frames-frames/4));
}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  for(bool exciter:{false,true}){
   const double low=delta(rate,exciter,0.f,257);
   const double high=delta(rate,exciter,1.f,257);
   const double high1=delta(rate,exciter,1.f,1);
   const double high4097=delta(rate,exciter,1.f,4097);
   std::printf("%.0f %s impact 0=%.7f impact 1=%.7f ratio %.3f\n",rate,exciter?"exciter":"analog",low,high,high/std::max(low,1e-12));
   require(low>1e-5&&high>low*1.2,"high impact audibly stronger measured change");
   require(std::abs(high-high1)<1e-6&&std::abs(high-high4097)<1e-6,"impact block-independence");
  }
 }
}
