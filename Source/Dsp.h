#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
namespace masterlab {
inline float clean(float x) noexcept { return std::isfinite(x) ? std::clamp(x,-1000000.f,1000000.f) : 0.f; }
struct Biquad {
 double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void peak(double sr,double hz,double db,double q) noexcept {
  hz=std::clamp(std::isfinite(hz)?hz:1000.,20.,sr*.45); q=std::clamp(std::isfinite(q)?q:1.,.1,20.);db=std::clamp(std::isfinite(db)?db:0.,-36.,36.);
  const double w=6.283185307179586*hz/sr,A=std::pow(10.,db/40.),alpha=std::sin(w)/(2*q),d=1+alpha/A;
  b0=(1+alpha*A)/d;b1=-2*std::cos(w)/d;b2=(1-alpha*A)/d;a1=b1;a2=(1-alpha/A)/d;
 }
 float tick(float x) noexcept { double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return clean(static_cast<float>(y)); }
 void reset() noexcept {z1=z2=0;}
};
class Engine {
 std::array<std::array<Biquad,16>,2> eq{};
 std::array<std::vector<float>,2> delay;
 std::vector<float> maxima; std::vector<size_t> stamps;
 size_t queueHead=0,queueTail=0,clock=0;
 size_t position=0; double sr=48000; float gain=1,eqMix=1,limMix=1;
 struct Band {float frequency=1000,db=0,q=1;};
 std::array<Band,16> bands{};float ceiling=.89125094f;
 bool eqOn=true,limOn=true;
public:
 void prepare(double rate) {sr=rate; for(auto& d:delay)d.assign(static_cast<size_t>(std::ceil(rate*.005))+1,0);maxima.resize(delay[0].size()+1);stamps.resize(maxima.size());reset();}
 void reset() noexcept {position=0;queueHead=queueTail=clock=0;gain=1;for(auto& channel:eq)for(auto& b:channel)b=Biquad{};for(auto& d:delay)std::fill(d.begin(),d.end(),0);}
 int latency() const noexcept {return delay[0].empty()?0:static_cast<int>(delay[0].size()-1);}
 void parameters(float hz,float decibels,float quality,float ceilingDb,bool e,bool l) noexcept {
 bands[0]={hz,decibels,quality};ceiling=std::pow(10.f,std::clamp(clean(ceilingDb),-24.f,0.f)/20);eqOn=e;limOn=l;
 }
 void setBand(size_t index,float hz,float db,float q) noexcept {if(index<bands.size())bands[index]={hz,db,q};}
 void process(float* left,float* right,int count,bool limiterFirst=false) noexcept {
  if(delay[0].empty())return;
  const float smooth=static_cast<float>(1-std::exp(-1/(sr*.02)));
  const float release=static_cast<float>(std::exp(-1/(sr*.08)));
  std::array<Biquad,16> targets;for(size_t k=0;k<bands.size();++k)targets[k].peak(sr,bands[k].frequency,bands[k].db,bands[k].q);
  for(int i=0;i<count;++i){
   for(auto& channel:eq)for(size_t k=0;k<bands.size();++k){auto& b=channel[k];const auto& t=targets[k];b.b0+=smooth*(t.b0-b.b0);b.b1+=smooth*(t.b1-b.b1);b.b2+=smooth*(t.b2-b.b2);b.a1+=smooth*(t.a1-b.a1);b.a2+=smooth*(t.a2-b.a2);}
   float x[2]={clean(left[i]),clean(right[i])};eqMix+=smooth*((eqOn?1.f:0.f)-eqMix);limMix+=smooth*((limOn?1.f:0.f)-limMix);
   auto equalize=[&]{for(int c=0;c<2;++c){float wet=x[c];for(auto& filter:eq[c])wet=filter.tick(wet);x[c]+=eqMix*(wet-x[c]);}};
   if(!limiterFirst)equalize();
   const float peak=std::max(std::abs(x[0]),std::abs(x[1]));
   const size_t capacity=maxima.size(), window=delay[0].size()-1;
   while(queueHead!=queueTail && clock-stamps[queueHead]>window)queueHead=(queueHead+1)%capacity;
   while(queueHead!=queueTail){size_t back=(queueTail+capacity-1)%capacity;if(maxima[back]>peak)break;queueTail=back;}
   maxima[queueTail]=peak;stamps[queueTail]=clock++;queueTail=(queueTail+1)%capacity;
   const float futurePeak=maxima[queueHead];
   const float target=futurePeak>ceiling?ceiling/futurePeak:1.f;
   gain=target<gain?target:release*gain+(1-release)*target;
   const size_t read=(position+1)%delay[0].size();
   for(int c=0;c<2;++c){delay[c][position]=x[c];const float dry=delay[c][read];const float wet=std::clamp(dry*gain,-ceiling,ceiling);x[c]=dry+limMix*(wet-dry);}
   position=read;
   if(limiterFirst)equalize();
   left[i]=clean(x[0]);right[i]=clean(x[1]);
  }
 }
};
}
