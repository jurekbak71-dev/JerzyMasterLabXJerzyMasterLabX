#include "HarmonicOversampler.h"
#include "HarmonicOversampler4x.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace masterlab;
constexpr double pi=3.14159265358979323846;
static double amplitude(const std::vector<float>& x,double hz,double sr,int start){
 double re=0.,im=0.;
 for(int i=start;i<(int)x.size();++i){
  const double angle=2.*pi*hz*i/sr;
  re+=x[i]*std::cos(angle);im+=x[i]*std::sin(angle);
 }
 return 2.*std::hypot(re,im)/(x.size()-start);
}
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  const int n=(int)sr;
  HarmonicOversampler2x x2;HarmonicOversampler4x x4;
  std::vector<float> a(n),b(n);
  for(int i=0;i<n;++i){
   const float v=.25f*std::sin(2.*pi*997.*i/sr);
   a[i]=x2.tick(v,[](float x){return x;});
   b[i]=x4.tick(v,[](float x){return x;});
  }
  const double g2=amplitude(a,997,sr,n/2);
  const double g4=amplitude(b,997,sr,n/2);
  std::printf("rate %.0f: 2x identity %.8f 4x identity %.8f\n",sr,g2,g4);
  if(std::abs(g2-.25)>.01||std::abs(g4-.25)>.01)return 1;
  if(HarmonicOversampler4x::latencySamples!=48)return 2;
 }
 {
  constexpr double sr=48000.,freq=10000.;constexpr int n=48000;
  HarmonicOversampler2x x2;HarmonicOversampler4x x4;
  std::vector<float> direct(n),two(n),four(n);
  for(int i=0;i<n;++i){
   const float in=.5f*std::sin(2*pi*freq*i/sr);
   const auto saturate=[](float v){return v*v*v;};
   direct[i]=saturate(in);two[i]=x2.tick(in,saturate);
   four[i]=x4.tick(in,saturate);
  }
  const auto d=amplitude(direct,18000,sr,n/2);
  const auto s2=amplitude(two,18000,sr,n/2);
  const auto s4=amplitude(four,18000,sr,n/2);
  std::printf("cubic alias 18kHz: direct %.8g 2x %.8g 4x %.8g\n",d,s2,s4);
  if(d<.01||s2>=d*.25||s4>=d*.25)return 3;
  // Do not assume 4x improves every 2x stopband frequency; verify each.
  x4.reset();
  for(int i=0;i<4096;++i)if(!std::isfinite(x4.tick(0.f,[](float v){return v*v*v;})))return 4;
 }
}
