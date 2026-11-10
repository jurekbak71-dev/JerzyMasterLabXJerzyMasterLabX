#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>
namespace masterlab {
inline float clean(float x) noexcept { return std::isfinite(x) ? std::clamp(x,-1000000.f,1000000.f) : 0.f; }
enum class FilterType { Bell, LowShelf, HighShelf, HighPass, LowPass, Notch };
struct Biquad {
 double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void peak(double sr,double hz,double db,double q) noexcept {
  hz=std::clamp(std::isfinite(hz)?hz:1000.,20.,sr*.45); q=std::clamp(std::isfinite(q)?q:1.,.1,20.);db=std::clamp(std::isfinite(db)?db:0.,-36.,36.);
  const double w=6.283185307179586*hz/sr,A=std::pow(10.,db/40.),alpha=std::sin(w)/(2*q),d=1+alpha/A;
  b0=(1+alpha*A)/d;b1=-2*std::cos(w)/d;b2=(1-alpha*A)/d;a1=b1;a2=(1-alpha/A)/d;
 }
 // Public RBJ cookbook forms. Shelves use Q (resonance), not a dB/oct slope control.
 void configure(double sr,double hz,double db,double q,FilterType type) noexcept {
  if(type==FilterType::Bell){peak(sr,hz,db,q);return;}
  hz=std::clamp(std::isfinite(hz)?hz:1000.,20.,sr*.45);q=std::clamp(std::isfinite(q)?q:1.,.1,20.);db=std::clamp(std::isfinite(db)?db:0.,-36.,36.);
  const double w=6.283185307179586*hz/sr,c=std::cos(w),alpha=std::sin(w)/(2*q),A=std::pow(10.,db/40.);
  double a0=1+alpha;
  a1=-2*c;a2=1-alpha;
  switch(type){
   case FilterType::LowPass:b0=(1-c)*.5;b1=1-c;b2=b0;break;
   case FilterType::HighPass:b0=(1+c)*.5;b1=-(1+c);b2=b0;break;
   case FilterType::Notch:b0=1;b1=-2*c;b2=1;break;
   case FilterType::LowShelf:{const double t=2*std::sqrt(A)*alpha;a0=(A+1)+(A-1)*c+t;b0=A*((A+1)-(A-1)*c+t);b1=2*A*((A-1)-(A+1)*c);b2=A*((A+1)-(A-1)*c-t);a1=-2*((A-1)+(A+1)*c);a2=(A+1)+(A-1)*c-t;break;}
   case FilterType::HighShelf:{const double t=2*std::sqrt(A)*alpha;a0=(A+1)-(A-1)*c+t;b0=A*((A+1)+(A-1)*c+t);b1=-2*A*((A-1)+(A+1)*c);b2=A*((A+1)+(A-1)*c-t);a1=2*((A-1)-(A+1)*c);a2=(A+1)-(A-1)*c-t;break;}
   default:peak(sr,hz,db,q);return;
  }
  b0/=a0;b1/=a0;b2/=a0;a1/=a0;a2/=a0;
 }
 float tick(float x) noexcept { double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return clean(static_cast<float>(y)); }
 std::complex<double> response(double rate,double hz) const noexcept {
  const double w=6.283185307179586*hz/rate,c=std::cos(w),s=std::sin(w),c2=std::cos(2*w),s2=std::sin(2*w);
  const double nr=b0+b1*c+b2*c2,ni=b1*s+b2*s2,dr=1+a1*c+a2*c2,di=a1*s+a2*s2;
  if(dr*dr+di*di<1e-30)return {1,0};
  return std::complex<double>(nr,-ni)/std::complex<double>(dr,-di);
 }
 double magnitude(double rate,double hz) const noexcept {return std::abs(response(rate,hz));}
 void reset() noexcept {z1=z2=0;}
};
class Engine {
 std::array<std::array<Biquad,16>,2> eq{};
 std::array<std::vector<float>,2> delay;
 std::vector<float> maxima; std::vector<size_t> stamps;
 size_t queueHead=0,queueTail=0,clock=0;
 size_t position=0; double sr=48000; float gain=1,eqMix=1,limMix=1,drive=1,driveTarget=1,releaseSeconds=.08f,releaseCoefficient=0;
 struct Band {float frequency=1000,db=0,q=1;FilterType type=FilterType::Bell;bool enabled=true;};
 std::array<float,16> bandMix{};
 std::array<Band,16> bands{};float ceiling=.89125094f;
 bool eqOn=true,limOn=true;
public:
 void prepare(double rate) {sr=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:48000;rate=sr; for(auto& d:delay)d.assign(static_cast<size_t>(std::ceil(rate*.005))+1,0);maxima.resize(delay[0].size()+1);stamps.resize(maxima.size());reset();}
 void reset() noexcept {position=0;queueHead=queueTail=clock=0;gain=1;drive=driveTarget;releaseCoefficient=static_cast<float>(std::exp(-1/(sr*releaseSeconds)));eqMix=eqOn?1.f:0.f;limMix=limOn?1.f:0.f;for(size_t k=0;k<bandMix.size();++k)bandMix[k]=bands[k].enabled?1.f:0.f;for(auto& channel:eq)for(auto& b:channel)b=Biquad{};for(auto& d:delay)std::fill(d.begin(),d.end(),0);}
 float eqBlend() const noexcept {return eqMix;}
 void copyEqCoefficients(std::array<Biquad,16>& destination) const noexcept {destination=eq[0];}
 void copyBandBlends(std::array<float,16>& destination) const noexcept {destination=bandMix;}
 int latency() const noexcept {return delay[0].empty()?0:static_cast<int>(delay[0].size()-1);}
 void parameters(float hz,float decibels,float quality,float ceilingDb,bool e,bool l) noexcept {
 bands[0].frequency=hz;bands[0].db=decibels;bands[0].q=quality;ceiling=std::pow(10.f,std::clamp(clean(ceilingDb),-24.f,0.f)/20);eqOn=e;limOn=l;
 }
 void limiterParameters(float driveDb,float releaseMs) noexcept {driveTarget=std::pow(10.f,std::clamp(clean(driveDb),0.f,24.f)/20);releaseSeconds=std::clamp(clean(releaseMs),10.f,1000.f)*.001f;}
 void setBand(size_t index,float hz,float db,float q) noexcept {if(index<bands.size()){bands[index].frequency=hz;bands[index].db=db;bands[index].q=q;}}
 void setBandMode(size_t index,FilterType type,bool enabled) noexcept {if(index<bands.size()){bands[index].type=type;bands[index].enabled=enabled;}}
 void process(float* left,float* right,int count,bool limiterFirst=false,float* gainTrace=nullptr,int stages=3) noexcept {
  if(delay[0].empty())return;
  const float smooth=static_cast<float>(1-std::exp(-1/(sr*.02)));
  const float release=static_cast<float>(std::exp(-1/(sr*releaseSeconds)));
  std::array<Biquad,16> targets;
  // Do not redesign sixteen EQ filters for a limiter-only processing stage.
  if(stages&1)for(size_t k=0;k<bands.size();++k)
   targets[k].configure(sr,bands[k].frequency,bands[k].db,bands[k].q,bands[k].type);
  for(int i=0;i<count;++i){
   if(stages&1)for(auto& channel:eq)for(size_t k=0;k<bands.size();++k){auto& b=channel[k];const auto& t=targets[k];b.b0+=smooth*(t.b0-b.b0);b.b1+=smooth*(t.b1-b.b1);b.b2+=smooth*(t.b2-b.b2);b.a1+=smooth*(t.a1-b.a1);b.a2+=smooth*(t.a2-b.a2);}
   if(stages&1)for(size_t k=0;k<bandMix.size();++k)bandMix[k]+=smooth*((bands[k].enabled?1.f:0.f)-bandMix[k]);
   float x[2]={clean(left[i]),clean(right[i])};if(stages&1)eqMix+=smooth*((eqOn?1.f:0.f)-eqMix);if(stages&2)limMix+=smooth*((limOn?1.f:0.f)-limMix);
   auto equalize=[&]{for(int c=0;c<2;++c){float wet=x[c];for(size_t k=0;k<16;++k){float filtered=eq[c][k].tick(wet);wet+=bandMix[k]*(filtered-wet);}x[c]+=eqMix*(wet-x[c]);}};
   if((stages&1)&&!limiterFirst)equalize();
   if(stages&2){
   drive+=smooth*(driveTarget-drive);releaseCoefficient+=smooth*(release-releaseCoefficient);
   const float peak=std::max(std::abs(x[0]),std::abs(x[1]))*drive;
   const size_t capacity=maxima.size(), window=delay[0].size()-1;
   while(queueHead!=queueTail && clock-stamps[queueHead]>window)queueHead=(queueHead+1)%capacity;
   while(queueHead!=queueTail){size_t back=(queueTail+capacity-1)%capacity;if(maxima[back]>peak)break;queueTail=back;}
   maxima[queueTail]=peak;stamps[queueTail]=clock++;queueTail=(queueTail+1)%capacity;
   const float futurePeak=maxima[queueHead];
   const float target=futurePeak>ceiling?ceiling/futurePeak:1.f;
   gain=target<gain?target:releaseCoefficient*gain+(1-releaseCoefficient)*target;
   const size_t read=(position+1)%delay[0].size();
   for(int c=0;c<2;++c){delay[c][position]=x[c];const float dry=delay[c][read];const float wet=std::clamp(dry*drive*gain,-ceiling,ceiling);x[c]=dry+limMix*(wet-dry);}
   position=read;
   if(gainTrace)gainTrace[i]=1+limMix*(gain-1);
   }
   if((stages&1)&&limiterFirst)equalize();
   left[i]=clean(x[0]);right[i]=clean(x[1]);
  }
 }
};

// Both serial orders run continuously, so switching never resets an active path.
// Scratch capacity is fixed; larger and changing host buffers are processed in chunks.
class RoutedEngine {
 std::array<Engine,2> paths;
 std::array<std::array<float,128>,2> scratch{};
 std::array<std::array<float,128>,2> gainScratch{};
 float routeMix=0,routeStep=1.f/960.f;
 bool targetOrder=false;
public:
 void prepare(double rate,bool limiterFirst=false){
  for(auto& path:paths)path.prepare(rate);
  const double validRate=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:48000;
  routeStep=static_cast<float>(1/(validRate*.02));targetOrder=limiterFirst;routeMix=limiterFirst?1.f:0.f;
 }
 void reset() noexcept {for(auto& path:paths)path.reset();routeMix=targetOrder?1.f:0.f;}
 int latency() const noexcept {return paths[0].latency();}
 float eqBlend() const noexcept {return paths[0].eqBlend();}
 void copyEqCoefficients(std::array<Biquad,16>& destination) const noexcept {paths[0].copyEqCoefficients(destination);}
 void copyBandBlends(std::array<float,16>& destination) const noexcept {paths[0].copyBandBlends(destination);}
 void parameters(float hz,float db,float q,float ceiling,bool eq,bool limiter) noexcept {
  for(auto& path:paths)path.parameters(hz,db,q,ceiling,eq,limiter);
 }
 void limiterParameters(float driveDb,float releaseMs) noexcept {for(auto& path:paths)path.limiterParameters(driveDb,releaseMs);}
 void setBand(size_t index,float hz,float db,float q) noexcept {for(auto& path:paths)path.setBand(index,hz,db,q);}
 void setBandMode(size_t index,FilterType type,bool enabled) noexcept {for(auto& path:paths)path.setBandMode(index,type,enabled);}
 void process(float* left,float* right,int count,bool limiterFirst=false,float* gainTrace=nullptr) noexcept {
  targetOrder=limiterFirst;
  for(int offset=0;offset<count;){
   const int length=std::min(128,count-offset);
   std::copy_n(left+offset,length,scratch[0].data());std::copy_n(right+offset,length,scratch[1].data());
   paths[0].process(left+offset,right+offset,length,false,gainScratch[0].data());
   paths[1].process(scratch[0].data(),scratch[1].data(),length,true,gainScratch[1].data());
   for(int i=0;i<length;++i){
    routeMix=limiterFirst?std::min(1.f,routeMix+routeStep):std::max(0.f,routeMix-routeStep);
    if(gainTrace)gainTrace[offset+i]=gainScratch[0][i]+routeMix*(gainScratch[1][i]-gainScratch[0][i]);
    left[offset+i]+=routeMix*(scratch[0][i]-left[offset+i]);
    right[offset+i]+=routeMix*(scratch[1][i]-right[offset+i]);
   }
   offset+=length;
  }
 }
};
}
