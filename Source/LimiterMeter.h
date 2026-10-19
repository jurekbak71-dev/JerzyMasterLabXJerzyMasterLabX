#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace masterlab {
// Audio-clocked 10 ms observations. No allocation, and no dependence on host block size.
struct LimiterFrame {std::uint64_t endSample=0,generation=0;float minimumGain=1,outputPeak=0;};
class LimiterMeter {
 double rate=48000;std::uint64_t clock=0,next=480,generation=0;unsigned long long frameNumber=1;
 float minimumGain=1,outputPeak=0;
public:
 void prepare(double sampleRate) noexcept {rate=std::isfinite(sampleRate)&&sampleRate>=8000&&sampleRate<=384000?sampleRate:48000;clock=0;++generation;frameNumber=1;next=static_cast<std::uint64_t>(std::ceil(rate*.01));minimumGain=1;outputPeak=0;}
 bool push(float gain,float left,float right,LimiterFrame& frame) noexcept {
  minimumGain=std::min(minimumGain,std::clamp(std::isfinite(gain)?gain:1.f,0.f,1.f));
  outputPeak=std::max(outputPeak,std::max(std::abs(left),std::abs(right)));
  if(++clock<next)return false;
  frame={clock,generation,minimumGain,outputPeak};minimumGain=1;outputPeak=0;
  next=static_cast<std::uint64_t>(std::ceil(++frameNumber*rate*.01));return true;
 }
};
}
