#pragma once
#include "Dsp.h"
#include <cstdint>
namespace masterlab {
struct MeterReading {
 float peak=0,rms=0,correlation=0,balance=0,midRms=0,sideRms=0;
 std::uint32_t clippedSamples=0;
 bool ready=false;
};
// Exact sliding 400 ms sample window. All storage is allocated in prepare().
class WindowMeter {
 std::vector<std::array<float,2>> history;
 std::vector<float> peaks;
 std::vector<std::uint64_t> stamps;
 size_t position=0,head=0,tail=0,count=0;
 std::uint64_t clock=0;
 std::uint32_t clips=0;
 double ll=0,rr=0,lr=0;
 static bool clipped(float l,float r) noexcept {return std::max(std::abs(l),std::abs(r))>1.f;}
public:
 void prepare(double rate){rate=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:48000;history.resize(static_cast<size_t>(std::ceil(rate*.4)));peaks.resize(history.size()+1);stamps.resize(peaks.size());reset();}
 void reset() noexcept {position=head=tail=count=0;clock=0;clips=0;ll=rr=lr=0;}
 void push(float left,float right) noexcept {
  if(history.empty())return;
  left=clean(left);right=clean(right);
  const auto old=count<history.size()?std::array<float,2>{0,0}:history[position];ll+=static_cast<double>(left)*left-static_cast<double>(old[0])*old[0];rr+=static_cast<double>(right)*right-static_cast<double>(old[1])*old[1];lr+=static_cast<double>(left)*right-static_cast<double>(old[0])*old[1];
  clips+=clipped(left,right)?1:0;clips-=clipped(old[0],old[1])?1:0;
  history[position]={left,right};position=(position+1)%history.size();count=std::min(count+1,history.size());
  const float peak=std::max(std::abs(left),std::abs(right));const size_t capacity=peaks.size();
  while(head!=tail&&clock-stamps[head]>=history.size())head=(head+1)%capacity;
  while(head!=tail){size_t back=(tail+capacity-1)%capacity;if(peaks[back]>peak)break;tail=back;}
  peaks[tail]=peak;stamps[tail]=clock++;tail=(tail+1)%capacity;
 }
 MeterReading read() const noexcept {
  if(count==0)return {};
  const double l=std::max(0.,ll),r=std::max(0.,rr),energy=l+r;
  MeterReading result;result.peak=head==tail?0:peaks[head];result.rms=static_cast<float>(std::sqrt(energy/(2*count)));
  result.correlation=l*r>1e-24?static_cast<float>(std::clamp(lr/std::sqrt(l*r),-1.,1.)):0;
  result.balance=energy>1e-24?static_cast<float>((r-l)/energy):0;
  result.midRms=static_cast<float>(std::sqrt(std::max(0.,energy+2*lr)/(4*count)));
  result.sideRms=static_cast<float>(std::sqrt(std::max(0.,energy-2*lr)/(4*count)));
  result.clippedSamples=clips;result.ready=count==history.size();return result;
 }
};
}
