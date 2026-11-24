#pragma once
// Final stereo-linked lookahead maximizer. Independent 1x/2x/4x/8x true-peak
// detection, fixed (per-sample-rate) host PDC and allocation-free callbacks.
// Original DSP; neither licensed iZotope IRC code nor an IRC emulation.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace masterlab {
struct FinalTruePeakSettings {
 bool enabled=false;
 float ceilingDb=-1.f,releaseMs=120.f,driveDb=0.f;
 int character=0;bool autoRelease=false;float softClip=0.f,outputDb=0.f;
 float attackMs=.25f,lookaheadMs=3.f,transientEmphasis=0.f;
 int oversamplingFactor=4; // 1, 2, 4 or 8: true-peak FIR reconstruction
};
struct FinalTruePeakReading {
 float detectedTruePeak=0.f,outputSamplePeak=0.f,gainReductionDb=0.f;
 std::array<float,64> inputWave{},outputWave{},gainEnvelope{};
 float outputTruePeak=0.f,lookaheadMs=0.f;
 int oversamplingFactor=4;
};
class FinalTruePeak {
public:
 static constexpr int detectorTaps=33,maxOversampling=8;
 static constexpr int maximumDelaySamples=4096;
 static constexpr int ringCapacity=maximumDelaySamples+1;
 static constexpr int peakQueueCapacity=maximumDelaySamples+detectorTaps+4;
 // Old fixed 128-sample PDC is intentionally replaced with ten milliseconds,
 // evaluated in prepare(rate) and held constant during parameter automation.
 int latency()const noexcept{return fixedLatencySamples_;}
private:
 static constexpr double pi=3.141592653589793238462643;
 std::array<std::array<double,detectorTaps>,maxOversampling> kernel_{};
 std::array<std::array<float,detectorTaps>,2> history_{},outputHistory_{};
 std::array<std::array<float,ringCapacity>,2> delay_{};
 std::array<float,ringCapacity> peakHistory_{};
 std::array<float,peakQueueCapacity> peakValues_{};
 std::array<std::uint64_t,peakQueueCapacity> peakStamps_{};
 int histPosition_=0,outputHistPosition_=0,delayPosition_=0;
 int fixedLatencySamples_=480,previousLookaheadSamples_=-1;
 size_t peakHead_=0,peakTail_=0;
 std::uint64_t sampleClock_=0;
 float gain_=1.f,controlledGain_=1.f,transientEnvelope_=0.f;
 double sampleRate_=48000.;
 FinalTruePeakSettings settings_{};
 FinalTruePeakReading reading_{};

 static float sane(float x,float a,float b)noexcept{
  return std::clamp(std::isfinite(x)?x:a,a,b);
 }
 static int selectedOversampling(int x)noexcept{
  return x==1||x==2||x==4||x==8?x:4;
 }
 static int advance(int index,int capacity)noexcept{
  return index+1==capacity?0:index+1;
 }
 void prepareKernel()noexcept{
  // 33-tap fractional-delay polyphase Blackman-windowed sinc. Coefficients
  // are precomputed (not redesigned in the real-time thread).
  for(int phase=0;phase<maxOversampling;++phase){
   const double fraction=static_cast<double>(phase)/maxOversampling;
   double sum=0.;
   for(int tap=0;tap<detectorTaps;++tap){
    const double t=static_cast<double>(tap)-(detectorTaps-1)/2.+fraction;
    const double sinc=std::abs(t)<1.e-12?1.:std::sin(pi*t)/(pi*t);
    const double blackman=.42-.5*std::cos(2*pi*tap/(detectorTaps-1))
                         +.08*std::cos(4*pi*tap/(detectorTaps-1));
    kernel_[phase][tap]=sinc*blackman;
    sum+=kernel_[phase][tap];
   }
   if(std::abs(sum)<1.e-12)sum=1.;
   for(double& coefficient:kernel_[phase])coefficient/=sum;
  }
 }
 float polyphasePeak(const std::array<std::array<float,detectorTaps>,2>& hist,
                     int nextPosition,int factor)const noexcept{
  float peak=0.f;
  for(int c=0;c<2;++c){
   for(int phase=0;phase<factor;++phase){
    // Reuse the precomputed 8-phase kernels at 1x, 2x and 4x.
    const int p=phase*(maxOversampling/factor);
    double value=0.;
    int cursor=nextPosition;
    for(int tap=0;tap<detectorTaps;++tap){
     cursor=cursor==0?detectorTaps-1:cursor-1;
     value+=kernel_[p][tap]*hist[c][cursor];
    }
    peak=std::max(peak,static_cast<float>(std::abs(value)));
   }
  }
  return peak;
 }
 void queuePeak(float value)noexcept{
  // Monotone queue: amortized O(1) per incoming peak, no allocations.
  while(peakHead_!=peakTail_){
   const size_t last=(peakTail_+peakQueueCapacity-1)%peakQueueCapacity;
   if(peakValues_[last]>value)break;
   peakTail_=last;
  }
  peakValues_[peakTail_]=value;
  peakStamps_[peakTail_]=sampleClock_;
  peakTail_=(peakTail_+1)%peakQueueCapacity;
 }
public:
 FinalTruePeak()noexcept{prepareKernel();}
 void settings(FinalTruePeakSettings value)noexcept{settings_=value;}
 void prepare(double rate)noexcept{
  sampleRate_=std::isfinite(rate)&&rate>=8000.&&rate<=384000.?rate:48000.;
  fixedLatencySamples_=std::clamp(static_cast<int>(std::ceil(sampleRate_*.010)),128,maximumDelaySamples);
  reset();
 }
 void reset()noexcept{
  history_={};outputHistory_={};delay_={};peakHistory_={};
  peakValues_={};peakStamps_={};
  histPosition_=outputHistPosition_=delayPosition_=0;
  previousLookaheadSamples_=-1;peakHead_=peakTail_=0;sampleClock_=0;
  gain_=controlledGain_=1.f;transientEnvelope_=0.f;reading_={};
 }
 FinalTruePeakReading reading()const noexcept{return reading_;}
 void process(float* left,float* right,int count)noexcept{
  if(!left||!right||count<=0)return;
  const int factor=selectedOversampling(settings_.oversamplingFactor);
  const int lookahead=std::clamp(
    static_cast<int>(std::lround(sampleRate_*.001*sane(settings_.lookaheadMs,.5f,10.f))),
    detectorTaps/2+2,fixedLatencySamples_);
  const float actualLookaheadMs=1000.f*lookahead/static_cast<float>(sampleRate_);
  const int detectorOffset=fixedLatencySamples_-lookahead;
  // Rebuild the monotone queue from fixed ring storage whenever the effective
  // lookahead changes; the transient buffer is never resized in process().
  if(previousLookaheadSamples_!=lookahead){
   peakHead_=peakTail_=0;
   previousLookaheadSamples_=lookahead;
  }
  if(!settings_.enabled){
   for(int i=0;i<count;++i){
    delay_[0][delayPosition_]=sane(left[i],-16.f,16.f);
    delay_[1][delayPosition_]=sane(right[i],-16.f,16.f);
    const int read=(delayPosition_+ringCapacity-fixedLatencySamples_)%ringCapacity;
    left[i]=delay_[0][read];right[i]=delay_[1][read];
    delayPosition_=advance(delayPosition_,ringCapacity);
   }
   history_={};outputHistory_={};peakHistory_={};
   histPosition_=outputHistPosition_=0;peakHead_=peakTail_=0;
   previousLookaheadSamples_=-1;
   sampleClock_+=static_cast<std::uint64_t>(count);
   gain_=controlledGain_=1.f;transientEnvelope_=0.f;reading_={};
   return;
  }

  const float ceiling=std::pow(10.f,.05f*sane(settings_.ceilingDb,-12.f,-.1f));
  const float drive=std::pow(10.f,.05f*sane(settings_.driveDb,0.f,18.f));
  const float outputGain=std::pow(10.f,.05f*sane(settings_.outputDb,-12.f,6.f));
  const float clip=sane(settings_.softClip,0.f,1.f);
  const float transient=sane(settings_.transientEmphasis,0.f,1.f);
  const float attackMs=sane(settings_.attackMs,.05f,10.f);
  const float characterScale=settings_.character==1?1.35f:
                             (settings_.character==2?.58f:1.f);
  const float releaseMs=sane(settings_.releaseMs*characterScale,10.f,1400.f);
  const float attackCoeff=std::exp(-1.f/(static_cast<float>(sampleRate_)*.001f*attackMs));
  const float envelopeCoeff=std::exp(-1.f/(static_cast<float>(sampleRate_)*.008f));
  const float clipKnee=std::max(.1f,ceiling*.85f);
  const float clipSpan=std::max(.04f,ceiling*.25f);
  auto preShape=[&](float x)noexcept {
   if(clip<=0.f||std::abs(x)<=clipKnee)return x;
   const float a=std::abs(x);
   const float rounded=clipKnee+clipSpan*std::tanh((a-clipKnee)/clipSpan);
   return std::copysign(a+clip*.40f*(rounded-a),x);
  };
  const float threshold=ceiling*(factor==1?.82f:.90f)/
                        std::max(1.e-6f,outputGain);
  std::array<float,64> inputWave{},outputWave{},gainEnvelope{};
  float peakIn=0.f,peakOut=0.f,peakOutTP=0.f,minGain=1.f;

  for(int i=0;i<count;++i){
   const float input[2]{
    preShape(sane(left[i],-16.f,16.f)*drive),
    preShape(sane(right[i],-16.f,16.f)*drive)
   };
   const float amplitude=std::max(std::abs(input[0]),std::abs(input[1]));
   const float oldEnvelope=transientEnvelope_;
   transientEnvelope_=envelopeCoeff*transientEnvelope_+
                      (1.f-envelopeCoeff)*amplitude;
   const float onset=std::clamp((amplitude-oldEnvelope)/
                               std::max(.05f,oldEnvelope+.05f),0.f,1.f);
   for(int c=0;c<2;++c){
    history_[c][histPosition_]=input[c];
    delay_[c][delayPosition_]=input[c];
   }
   histPosition_=advance(histPosition_,detectorTaps);
   const float interpolated=polyphasePeak(history_,histPosition_,factor);
   const float peak=std::max(amplitude,interpolated);
   peakIn=std::max(peakIn,peak);
   peakHistory_[delayPosition_]=peak;
   const int detectorRead=(delayPosition_+ringCapacity-detectorOffset)%ringCapacity;
   const float delayedPeak=peakHistory_[detectorRead];
   const int queueWindow=lookahead+detectorTaps/2+1;
   while(peakHead_!=peakTail_&&
         sampleClock_-peakStamps_[peakHead_]>=static_cast<std::uint64_t>(queueWindow))
     peakHead_=(peakHead_+1)%peakQueueCapacity;
   queuePeak(delayedPeak);
   const float futurePeak=peakHead_!=peakTail_?peakValues_[peakHead_]:delayedPeak;
   const float desired=futurePeak>threshold?threshold/futurePeak:1.f;

   float recoveryMs=releaseMs;
   if(settings_.autoRelease){
    const float reduction=-20.f*std::log10(std::max(.001f,controlledGain_));
    recoveryMs*=1.f+.12f*std::clamp(reduction,0.f,12.f);
    // Transient emphasis speeds recovery following a rising percussion hit,
    // never increases gain above the true-peak safety request.
    recoveryMs*=1.f-.48f*transient*onset;
   }
   const float release=std::exp(-1.f/(static_cast<float>(sampleRate_)*.001f*
                                      std::max(10.f,recoveryMs)));
   controlledGain_=desired<controlledGain_?
     attackCoeff*controlledGain_+(1.f-attackCoeff)*desired:
     release*controlledGain_+(1.f-release)*desired;
   // The user-selected attack shapes a predictive gain envelope, while
   // sample-aligned reconstructed peak detection forms the independent hard
   // safety gate. This leaves quieter samples ahead of the transient free
   // to ramp down musically without risking a late overshoot.
   const int read=(delayPosition_+ringCapacity-fixedLatencySamples_)%ringCapacity;
   const int peakAtOutput=(read+detectorTaps/2)%ringCapacity;
   const float atOutput=std::max(peakHistory_[peakAtOutput],
       std::max(std::abs(delay_[0][read]),std::abs(delay_[1][read])));
   const float safeGain=atOutput>threshold?threshold/atOutput:1.f;
   gain_=std::min(controlledGain_,safeGain);
   minGain=std::min(minGain,gain_);
   const float y[2]{
    delay_[0][read]*gain_*outputGain,
    delay_[1][read]*gain_*outputGain
   };
   delayPosition_=advance(delayPosition_,ringCapacity);
   left[i]=std::clamp(y[0],-ceiling,ceiling);
   right[i]=std::clamp(y[1],-ceiling,ceiling);
   peakOut=std::max(peakOut,std::max(std::abs(left[i]),std::abs(right[i])));
   for(int c=0;c<2;++c)
    outputHistory_[c][outputHistPosition_]=c==0?left[i]:right[i];
   outputHistPosition_=advance(outputHistPosition_,detectorTaps);
   peakOutTP=std::max(peakOutTP,
     std::max(peakOut,polyphasePeak(outputHistory_,outputHistPosition_,factor)));
   const size_t index=std::min<size_t>(63,static_cast<size_t>(i)*64/
                                             static_cast<size_t>(count));
   inputWave[index]=.5f*(input[0]+input[1]);
   outputWave[index]=.5f*(left[i]+right[i]);
   gainEnvelope[index]=gain_;
   ++sampleClock_;
  }
  reading_.detectedTruePeak=peakIn;
  reading_.outputSamplePeak=peakOut;
  reading_.outputTruePeak=peakOutTP;
  reading_.gainReductionDb=-20.f*std::log10(std::max(1.e-9f,minGain));
  reading_.inputWave=inputWave;
  reading_.outputWave=outputWave;
  reading_.gainEnvelope=gainEnvelope;
  reading_.lookaheadMs=actualLookaheadMs;
  reading_.oversamplingFactor=factor;
 }
};
}
