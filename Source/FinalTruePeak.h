#pragma once
// Phase 5D: 4x bandlimited final-stage detector, stereo linked, fixed host PDC.
// Monotone sliding-window peak queue replaces a 129-element scan per sample.
// No heap allocation, locks or system calls in process().
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
namespace masterlab {
struct FinalTruePeakSettings {
 bool enabled=false;
 float ceilingDb=-1.f,releaseMs=120.f,driveDb=0.f;
};
struct FinalTruePeakReading {
 float detectedTruePeak=0.f,outputSamplePeak=0.f,gainReductionDb=0.f;
};
class FinalTruePeak {
public:
 static constexpr int detectorTaps=25,oversampling=4,latencySamples=128;
private:
 static constexpr double pi=3.14159265358979323846;
 std::array<std::array<double,detectorTaps>,oversampling> kernel_{};
 std::array<std::array<float,detectorTaps>,2> history_{};
 std::array<std::array<float,latencySamples+1>,2> delay_{};
 // Monotone deque: oldest peak at peakHead_, newest at tail-1.
 // Ring capacity exceeds window length by one to distinguish full/empty.
 static constexpr size_t peakQueueCapacity=latencySamples+2;
 std::array<float,peakQueueCapacity> peakValues_{};
 std::array<std::uint64_t,peakQueueCapacity> peakStamps_{};
 size_t peakHead_=0,peakTail_=0;
 std::uint64_t sampleClock_=0;
 int histPosition_=0,delayPosition_=0;
 float gain_=1.f;
 double sampleRate_=48000.;
 FinalTruePeakSettings settings_{};
 FinalTruePeakReading reading_{};
 static float sane(float x,float a,float b)noexcept{
  return std::clamp(std::isfinite(x)?x:a,a,b);
 }
 void prepareKernel()noexcept{
  // 4 polyphase fractional-delay FIR kernels, unity response at DC.
  // Peak estimates refer to the input at a fixed group delay of 12 samples.
  for(int phase=0;phase<oversampling;++phase){
   const double fraction=static_cast<double>(phase)/oversampling;
   double sum=0.;
   for(int tap=0;tap<detectorTaps;++tap){
    const double t=static_cast<double>(tap)-(detectorTaps-1)/2.+fraction;
    const double sinc=std::abs(t)<1e-12?1.:std::sin(pi*t)/(pi*t);
    const double blackman=.42-.5*std::cos(2*pi*tap/(detectorTaps-1))
                         +.08*std::cos(4*pi*tap/(detectorTaps-1));
    kernel_[phase][tap]=sinc*blackman;
    sum+=kernel_[phase][tap];
   }
   if(std::abs(sum)<1e-12)sum=1.;
   for(auto& coefficient:kernel_[phase])coefficient/=sum;
  }
 }
public:
 FinalTruePeak()noexcept{prepareKernel();}
 void settings(FinalTruePeakSettings value)noexcept{settings_=value;}
 void prepare(double rate)noexcept{
  sampleRate_=std::isfinite(rate)&&rate>=8000.&&rate<=384000.?rate:48000.;
  reset();
 }
 void reset()noexcept{
  history_={};delay_={};peakValues_={};peakStamps_={};
  histPosition_=delayPosition_=0;peakHead_=peakTail_=0;sampleClock_=0;
  gain_=1.f;reading_={};
 }
 FinalTruePeakReading reading()const noexcept{return reading_;}
 void process(float* left,float* right,int count)noexcept{
  if(!left||!right||count<=0)return;
  // Experimental engine operates with fixed latency even in disabled state.
  // Caller must align the global dry bypass path with latencySamples.
  const float ceiling=std::pow(10.f,sane(settings_.ceilingDb,-12.f,-.1f)*.05f);
  const float drive=std::pow(10.f,sane(settings_.driveDb,0.f,18.f)*.05f);
  const float release=std::exp(-1.f/(static_cast<float>(sampleRate_)*.001f*
                                      sane(settings_.releaseMs,10.f,1000.f)));
  float detected=0.f,outputPeak=0.f,minGain=1.f;
  for(int i=0;i<count;++i){
   const float input[2]{
    sane(left[i],-16.f,16.f)*drive,
    sane(right[i],-16.f,16.f)*drive
   };
   for(int c=0;c<2;++c){
    history_[c][histPosition_]=input[c];
    delay_[c][delayPosition_]=input[c];
   }
   histPosition_=(histPosition_+1)%detectorTaps;
   float peak=0.f;
   for(int c=0;c<2;++c)for(int p=0;p<oversampling;++p){
    double value=0.;
    int cursor=histPosition_;
    for(int tap=0;tap<detectorTaps;++tap){
     cursor=(cursor+detectorTaps-1)%detectorTaps;
     value+=kernel_[p][tap]*history_[c][cursor];
    }
    peak=std::max(peak,static_cast<float>(std::abs(value)));
   }
   detected=std::max(detected,peak);
   // Sliding maximum of the most recent 129 peak estimates.
   // Equal peaks evict older duplicates. No allocation or linear scan.
   // Evict expired indices before insertion (inclusive 129-sample window).
   while(peakHead_!=peakTail_ &&
         sampleClock_-peakStamps_[peakHead_]>=latencySamples+1)
    peakHead_=(peakHead_+1)%peakQueueCapacity;
   while(peakHead_!=peakTail_){
    const size_t back=(peakTail_+peakQueueCapacity-1)%peakQueueCapacity;
    if(peakValues_[back]>peak)break;
    peakTail_=back;
   }
   peakValues_[peakTail_]=peak;
   peakStamps_[peakTail_]=sampleClock_++;
   peakTail_=(peakTail_+1)%peakQueueCapacity;
   const float futurePeak=peakValues_[peakHead_];
   // Conservative true-peak reconstruction safety margin: the 4x input
   // detector alone cannot bound the interpolated output after gain changes.
   // Measured independently against 8x / 49-tap reconstruction stress cases.
   const float threshold=ceiling*.90f;
   const float desired=futurePeak>threshold?threshold/futurePeak:1.f;
   gain_=desired<gain_?desired:release*gain_+(1.f-release)*desired;
   minGain=std::min(minGain,gain_);
   const int outputPosition=(delayPosition_+1)%static_cast<int>(delay_[0].size());
   const float y[2]{
    delay_[0][outputPosition]*gain_,
    delay_[1][outputPosition]*gain_
   };
   delayPosition_=outputPosition;
   left[i]=settings_.enabled?std::clamp(y[0],-ceiling,ceiling):
                               delay_[0][outputPosition];
   right[i]=settings_.enabled?std::clamp(y[1],-ceiling,ceiling):
                                delay_[1][outputPosition];
   outputPeak=std::max(outputPeak,std::max(std::abs(left[i]),std::abs(right[i])));
  }
  reading_.detectedTruePeak=detected;
  reading_.outputSamplePeak=outputPeak;
  reading_.gainReductionDb=-20.f*std::log10(std::max(1e-9f,minGain));
 }
};
}
