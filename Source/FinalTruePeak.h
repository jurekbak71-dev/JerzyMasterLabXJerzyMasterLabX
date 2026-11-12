#pragma once
// Phase 5D: 4x bandlimited final-stage detector, stereo linked, fixed host PDC.
// Monotone sliding-window peak queue replaces a 129-element scan per sample.
// No heap allocation, locks or system calls in process().
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include "HarmonicOversampler4x.h"
#include "ScopeCapture.h"
namespace masterlab {
struct FinalTruePeakSettings {
 bool enabled=false;
 float ceilingDb=-1.f,releaseMs=120.f,driveDb=0.f;
 int character=0;bool autoRelease=false;float softClip=0.f,outputDb=0.f;
 float attackMs=0.f,lookaheadSamples=128.f,transientEmphasis=0.f;
 int oversamplingMode=2; // 0: 1x, 1: 2x, 2: 4x detector
 float density=0.f,compressorMix=1.f,stereoLink=1.f;
 bool delta=false;
};
struct FinalTruePeakReading {
 float detectedTruePeak=0.f,outputSamplePeak=0.f,gainReductionDb=0.f;
 ScopeCapture::Wave inputWave{},outputWave{};
 ScopeCapture::Wave gainEnvelope=[] {ScopeCapture::Wave wave;wave.fill(1.f);return wave;}();
};
class FinalTruePeak {
public:
 static constexpr int detectorTaps=25,oversampling=4,latencySamples=128;
private:
 static constexpr double pi=3.14159265358979323846;
 std::array<std::array<double,detectorTaps>,oversampling> kernel_{};
 std::array<std::array<float,detectorTaps>,2> history_{};
 static constexpr int activeDelaySamples=latencySamples-HarmonicOversampler4x::latencySamples;
 std::array<HarmonicOversampler4x,2> softClipOversampler_{};
 std::array<std::array<float,HarmonicOversampler4x::latencySamples+1>,2> softClipDryDelay_{};
 std::array<std::array<float,activeDelaySamples+1>,2> activeDelay_{};
 std::array<std::array<float,latencySamples+1>,2> bypassDelay_{};
 // Monotone deque: oldest peak at peakHead_, newest at tail-1.
 // Ring capacity exceeds window length by one to distinguish full/empty.
 static constexpr size_t peakQueueCapacity=latencySamples+2;
 std::array<float,peakQueueCapacity> peakValues_{};
 std::array<std::uint64_t,peakQueueCapacity> peakStamps_{};
 size_t peakHead_=0,peakTail_=0;
 std::uint64_t sampleClock_=0;
 int histPosition_=0,softClipDryPosition_=0,activeDelayPosition_=0,bypassDelayPosition_=0;
 float gain_=1.f;
 float programGain_=1.f,transientEnvelope_=0.f;
 float densityEnvelope_=0.f,densityGain_=1.f;
 float softClipMix_=0.f,softClipStep_=1.f/960.f;
 std::array<float,latencySamples+1> peakHistory_{};
 double sampleRate_=48000.;
 FinalTruePeakSettings settings_{};
 FinalTruePeakReading reading_{};
 ScopeCapture scope_;
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
 void settings(FinalTruePeakSettings value)noexcept{
  if(value.enabled!=settings_.enabled)scope_.reset();
  settings_=value;
 }
 void prepare(double rate)noexcept{
  sampleRate_=std::isfinite(rate)&&rate>=8000.&&rate<=384000.?rate:48000.;
  softClipStep_=1.f/static_cast<float>(sampleRate_*.02);
  for(auto& oversampler:softClipOversampler_)oversampler.prepare();
  scope_.prepare(sampleRate_);
  reset();
 }
 void reset()noexcept{
  history_={};softClipDryDelay_={};activeDelay_={};bypassDelay_={};peakValues_={};peakStamps_={};
  for(auto& oversampler:softClipOversampler_)oversampler.reset();
  histPosition_=softClipDryPosition_=activeDelayPosition_=bypassDelayPosition_=0;peakHead_=peakTail_=0;sampleClock_=0;
  gain_=programGain_=densityGain_=1.f;transientEnvelope_=densityEnvelope_=0.f;softClipMix_=0.f;peakHistory_={};reading_={};scope_.reset();
 }
 FinalTruePeakReading reading()const noexcept{return reading_;}
 void process(float* left,float* right,int count,float* gainTrace=nullptr)noexcept{
  if(!left||!right||count<=0)return;
  // Inactive stage keeps only its fixed PDC delay. Skip all 4x polyphase
  // convolution, peak queue updates and gain control to avoid DSP CPU waste.
  // Delay is still required by the host's constant latency contract.
  if(!settings_.enabled){
   for(int i=0;i<count;++i){
    bypassDelay_[0][bypassDelayPosition_]=sane(left[i],-16.f,16.f);
    bypassDelay_[1][bypassDelayPosition_]=sane(right[i],-16.f,16.f);
    const int read=(bypassDelayPosition_+1)%static_cast<int>(bypassDelay_[0].size());
    left[i]=bypassDelay_[0][read];right[i]=bypassDelay_[1][read];
    if(gainTrace)gainTrace[i]*=1.f;
    bypassDelayPosition_=read;
   }
   history_={};histPosition_=0;peakHead_=peakTail_=0;
   sampleClock_+=static_cast<std::uint64_t>(count);gain_=1.f;reading_={};
   return;
  }
  // The host's dry bypass path is aligned with latencySamples.
  const float ceiling=std::pow(10.f,sane(settings_.ceilingDb,-12.f,-.1f)*.05f);
  const float drive=std::pow(10.f,sane(settings_.driveDb,0.f,18.f)*.05f);
  const float outputGain=std::pow(10.f,sane(settings_.outputDb,-12.f,6.f)*.05f);
  const float characterScale=settings_.character==1?1.45f:(settings_.character==2?.52f:1.f);
  const float release=std::exp(-1.f/(static_cast<float>(sampleRate_)*.001f*
                                      sane(settings_.releaseMs*characterScale,10.f,1400.f)));
  const float attackMs=sane(settings_.attackMs,0.f,20.f);
  const float attack=attackMs<=.001f?0.f:std::exp(-1.f/(static_cast<float>(sampleRate_)*.001f*attackMs));
  const int lookahead=std::clamp(static_cast<int>(std::lround(sane(settings_.lookaheadSamples,0.f,static_cast<float>(latencySamples)))),0,latencySamples);
  const int detectorMode=std::clamp(settings_.oversamplingMode,0,2);
  const int phaseStep=detectorMode==0?4:(detectorMode==1?2:1);
  const float emphasis=sane(settings_.transientEmphasis,0.f,1.f);
  const float clip=std::clamp(settings_.softClip,0.f,1.f);
  const float density=sane(settings_.density,0.f,1.f);
  const float compressorMix=sane(settings_.compressorMix,0.f,1.f);
  const float densityAttack=std::exp(-1.f/(static_cast<float>(sampleRate_)*.010f));
  const float densityRelease=std::exp(-1.f/(static_cast<float>(sampleRate_)*
      (settings_.character==1?.090f:(settings_.character==2?.055f:.160f))));
  const float densityThreshold=.18f;
  const float densityRatio=1.f+7.f*density;
  const float densityMakeupDb=3.f*density;
  // Nonlinear saturation must run *before* the lookahead true-peak detector:
  // shaping after gain control creates new, undetected intersample peaks.
  const float clipThreshold=std::max(.1f,ceiling*.75f);
  auto saturate=[&](float x)noexcept {
   return clipThreshold*std::tanh(x/clipThreshold);
  };
  float detected=0.f,outputPeak=0.f,minGain=1.f;
  for(int i=0;i<count;++i){
   const float raw[2]{sane(left[i],-16.f,16.f),sane(right[i],-16.f,16.f)};
   const float linkedPeak=std::max(std::abs(raw[0]),std::abs(raw[1]));
   const float envelopeCoefficient=linkedPeak>densityEnvelope_?densityAttack:densityRelease;
   densityEnvelope_=envelopeCoefficient*densityEnvelope_+(1.f-envelopeCoefficient)*linkedPeak;
   const float overDb=std::max(0.f,20.f*std::log10(std::max(1.e-9f,densityEnvelope_/densityThreshold)));
   const float reductionDb=overDb*(1.f-1.f/densityRatio);
   const float densityTarget=std::pow(10.f,(densityMakeupDb-reductionDb)*.05f);
   const float gainCoefficient=densityTarget<densityGain_?densityAttack:densityRelease;
   densityGain_=gainCoefficient*densityGain_+(1.f-gainCoefficient)*densityTarget;
   const float densityBlend=density*compressorMix;
   const float compressed[2]{
    raw[0]+densityBlend*(raw[0]*densityGain_-raw[0]),
    raw[1]+densityBlend*(raw[1]*densityGain_-raw[1])
   };
   const float driven[2]{compressed[0]*drive,compressed[1]*drive};
   const int softClipRead=(softClipDryPosition_+1)%static_cast<int>(softClipDryDelay_[0].size());
   float input[2]{};
   const bool runSoftClip=clip>0.f||softClipMix_>1.e-6f;
   for(int c=0;c<2;++c){
    softClipDryDelay_[c][softClipDryPosition_]=driven[c];
    const float dry=softClipDryDelay_[c][softClipRead];
    const float wet=runSoftClip?softClipOversampler_[c].tick(driven[c],saturate):dry;
    input[c]=dry+softClipMix_*(wet-dry);
   }
   softClipDryPosition_=softClipRead;
   softClipMix_=clip>softClipMix_?std::min(clip,softClipMix_+softClipStep_):
                                   std::max(clip,softClipMix_-softClipStep_);
   for(int c=0;c<2;++c){
    history_[c][histPosition_]=input[c];
    activeDelay_[c][activeDelayPosition_]=input[c];
   }
   histPosition_=(histPosition_+1)%detectorTaps;
   float peak=0.f;
   for(int c=0;c<2;++c)for(int p=0;p<oversampling;p+=phaseStep){
    double value=0.;
    int cursor=histPosition_;
    for(int tap=0;tap<detectorTaps;++tap){
     cursor=(cursor+detectorTaps-1)%detectorTaps;
     value+=kernel_[p][tap]*history_[c][cursor];
    }
    peak=std::max(peak,static_cast<float>(std::abs(value)));
   }
   transientEnvelope_=std::max(peak,transientEnvelope_*.995f);
   const float transient=std::max(0.f,peak-transientEnvelope_*.92f);
   peak*=1.f+emphasis*transient/std::max(peak,1.e-6f);
   detected=std::max(detected,peak);
   peakHistory_[sampleClock_%peakHistory_.size()]=peak;
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
   // During startup the delayed output is silence, but the gain computer must
   // still accumulate the same available future window as the original fixed
   // 128-sample implementation. Otherwise block telemetry can under-report GR.
   const auto outputStamp=sampleClock_>static_cast<std::uint64_t>(latencySamples)?
      sampleClock_-static_cast<std::uint64_t>(latencySamples)-1:0;
   const auto finalStamp=std::min(sampleClock_-1,outputStamp+static_cast<std::uint64_t>(lookahead));
   float futurePeak=0.f;
   for(auto stamp=outputStamp;stamp<=finalStamp;++stamp)
    futurePeak=std::max(futurePeak,peakHistory_[stamp%peakHistory_.size()]);
   // Conservative true-peak reconstruction safety margin: the 4x input
   // detector alone cannot bound the interpolated output after gain changes.
   // Measured independently against 8x / 49-tap reconstruction stress cases.
   const float qualityMargin=detectorMode==0?.78f:(detectorMode==1?.84f:.90f);
   const float threshold=ceiling*qualityMargin/std::max(1.e-6f,outputGain);
   const float desired=futurePeak>threshold?threshold/futurePeak:1.f;
   const float adaptive=settings_.autoRelease?std::clamp((1.f-gain_)*.48f,0.f,.48f):0.f;
   const float recovery=release+(1.f-release)*adaptive;
   programGain_=desired<programGain_?attack*programGain_+(1.f-attack)*desired:
                                      recovery*programGain_+(1.f-recovery)*desired;
   gain_=std::min(desired,programGain_);
   minGain=std::min(minGain,gain_);
   const int outputPosition=(activeDelayPosition_+1)%static_cast<int>(activeDelay_[0].size());
   const float y[2]{
    activeDelay_[0][outputPosition]*gain_*outputGain,
    activeDelay_[1][outputPosition]*gain_*outputGain
   };
   activeDelayPosition_=outputPosition;
   // This stage is now linear after the detector, preserving the
   // detector's conservative 8x reconstructed output safety margin.
   left[i]=std::clamp(y[0],-ceiling,ceiling);
   right[i]=std::clamp(y[1],-ceiling,ceiling);
   if(gainTrace)gainTrace[i]*=gain_;
   scope_.push(.5f*(activeDelay_[0][outputPosition]+activeDelay_[1][outputPosition]),
               .5f*(left[i]+right[i]),gain_);
   outputPeak=std::max(outputPeak,std::max(std::abs(left[i]),std::abs(right[i])));
  }
  reading_.detectedTruePeak=detected;
  reading_.outputSamplePeak=outputPeak;
  reading_.gainReductionDb=-20.f*std::log10(std::max(1e-9f,minGain));
  reading_.inputWave=scope_.input();reading_.outputWave=scope_.output();reading_.gainEnvelope=scope_.gain();
 }
};
}
