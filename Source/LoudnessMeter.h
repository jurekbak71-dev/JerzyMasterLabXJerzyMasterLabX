#pragma once
#include "Dsp.h"
#include <cstdint>
namespace masterlab {
struct LoudnessReading {float momentary=-120,shortTerm=-120,integrated=-120;};
// Stereo BS.1770 K-weighting, 400 ms/3 s windows and dual-gated integrated loudness.
// Integrated gate histogram: 0.01 LU bins, exact energy sums and bounded O(log N) Fenwick queries.
// No LRA or true-peak measurement. All storage is allocated in prepare().
class LoudnessMeter {
 struct Bin {double energy=0;std::uint64_t count=0,generation=0;};
 struct Block {double energy=0;std::uint64_t samples=0;};
 std::vector<Bin> histogram;
 std::array<std::array<Biquad,2>,2> filters;
 std::array<Block,30> blocks{};
 double rate=48000,currentEnergy=0,mEnergy=0,sEnergy=0,absoluteEnergy=0;
 std::uint64_t clock=0,next=4800,number=0,currentSamples=0,mSamples=0,sSamples=0,absoluteCount=0,generation=0;
 LoudnessReading reading;
 static float lufs(double energy) noexcept {return energy>1e-30?static_cast<float>(-.691+10*std::log10(energy)):-120.f;}
 void configure() noexcept {
  const double k=std::tan(3.14159265358979323846*1681.974450955533/rate),q=.7071752369554196;
  const double vh=std::pow(10.,3.999843853973347/20),vb=std::pow(vh,.4996667741545416),a0=1+k/q+k*k;
  Biquad shelf;shelf.b0=(vh+vb*k/q+k*k)/a0;shelf.b1=2*(k*k-vh)/a0;shelf.b2=(vh-vb*k/q+k*k)/a0;shelf.a1=2*(k*k-1)/a0;shelf.a2=(1-k/q+k*k)/a0;
  const double kh=std::tan(3.14159265358979323846*38.13547087602444/rate),qh=.5003270373238773,den=1+kh/qh+kh*kh;
  Biquad highpass;highpass.b0=1;highpass.b1=-2;highpass.b2=1;highpass.a1=2*(kh*kh-1)/den;highpass.a2=(1-kh/qh+kh*kh)/den;
  for(auto& channel:filters){channel[0]=shelf;channel[1]=highpass;}
 }
 Bin prefix(size_t bins) const noexcept {
  Bin result;
  while(bins){const auto& bin=histogram[bins];if(bin.generation==generation){result.energy+=bin.energy;result.count+=bin.count;}bins-=bins&(~bins+1);}
  return result;
 }
 void finishBlock() noexcept {
  const size_t index=static_cast<size_t>(number%30),oldM=static_cast<size_t>((number+26)%30);
  if(number>=4){mEnergy-=blocks[oldM].energy;mSamples-=blocks[oldM].samples;}
  if(number>=30){sEnergy-=blocks[index].energy;sSamples-=blocks[index].samples;}
  blocks[index]={currentEnergy,currentSamples};mEnergy+=currentEnergy;mSamples+=currentSamples;sEnergy+=currentEnergy;sSamples+=currentSamples;
  ++number;
  if(number>=4){
   const double energy=std::max(0.,mEnergy)/mSamples;reading.momentary=lufs(energy);
   if(reading.momentary>-70){
    const int indexBin=std::clamp(static_cast<int>(std::floor((reading.momentary+70)*100)),0,19999);
    for(size_t index=static_cast<size_t>(indexBin)+1;index<histogram.size();index+=index&(~index+1)){auto& bin=histogram[index];if(bin.generation!=generation)bin={0,0,generation};bin.energy+=energy;++bin.count;}absoluteEnergy+=energy;++absoluteCount;
   }
   if(absoluteCount){
    const double gate=std::max(std::pow(10.,(-70+.691)/10),absoluteEnergy/absoluteCount*.1);
    const int first=std::clamp(static_cast<int>(std::floor((lufs(gate)+70)*100)),0,19999);
    const auto below=prefix(static_cast<size_t>(first)),through=prefix(static_cast<size_t>(first)+1);
    double sum=absoluteEnergy-through.energy;std::uint64_t count=absoluteCount-through.count;
    const auto boundaryCount=through.count-below.count;const double boundaryEnergy=through.energy-below.energy;
    if(boundaryCount&&boundaryEnergy/boundaryCount>gate){sum+=boundaryEnergy;count+=boundaryCount;}
    reading.integrated=count?lufs(sum/count):-120.f;
   }
  }
  if(number>=30)reading.shortTerm=lufs(std::max(0.,sEnergy)/sSamples);
  currentEnergy=0;currentSamples=0;next=static_cast<std::uint64_t>(std::ceil((number+1)*rate*.1));
 }
public:
 void prepare(double sampleRate){rate=std::isfinite(sampleRate)&&sampleRate>=8000&&sampleRate<=384000?sampleRate:48000;histogram.assign(20001,Bin{});configure();reset();}
 void reset() noexcept {++generation;clock=number=currentSamples=mSamples=sSamples=absoluteCount=0;currentEnergy=mEnergy=sEnergy=absoluteEnergy=0;blocks={};reading={};next=static_cast<std::uint64_t>(std::ceil(rate*.1));for(auto& channel:filters)for(auto& filter:channel)filter.reset();}
 void push(float left,float right) noexcept {
  if(histogram.empty())return;
  const float l=filters[0][1].tick(filters[0][0].tick(clean(left))),r=filters[1][1].tick(filters[1][0].tick(clean(right)));
  currentEnergy+=static_cast<double>(l)*l+static_cast<double>(r)*r;++currentSamples;
  if(++clock>=next)finishBlock();
 }
 LoudnessReading read() const noexcept {return reading;}
};
}
