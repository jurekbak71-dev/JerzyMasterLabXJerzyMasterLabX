#pragma once
// Unified Analog Exciter: one four-band harmonic generator plus one mutually
// exclusive Tube/FET/Tape colour transfer (not three serial saturators).
// Legacy host parameter IDs are adapted here without allocating in audio.
#include "HarmonicExciter.h"
#include "Analogizer.h"
#include "AnalogColor.h"
#include "ScopeCapture.h"

namespace masterlab {
struct AnalogExciterReading {
 float inputRms=0.f,outputRms=0.f,deltaRms=0.f;
 ScopeCapture::Wave inputWave{},outputWave{};
};
class AnalogExciter {
 ScopeCapture scope_;
 AnalogExciterReading reading_{};
 float mixTarget_=1.f,mix_=1.f,delta_=0.f,slew_=.001f;
 bool deltaTarget_=false;
 HarmonicExciter harmonic_;
 Analogizer colour_;
 std::array<std::array<float,Analogizer::latencySamples()>,2> dryDelay_{};
 size_t dryPosition_=0;
 HarmonicExciterSettings harmonics_{};
 AnalogColorSettings legacyColour_{};
 AnalogizerSettings modernColour_{};
 float excitement_=0.f;
 void update()noexcept {
  auto harmonics=harmonics_;
  // Master excitement is a parameter macro, not output gain: at 0 legacy
  // presets retain their existing behaviour. At 1, all four harmonic
  // bands contribute, while their existing Amount/Even/Odd remain editable.
  const float excite=std::clamp(excitement_,0.f,1.f);
  const bool colourActive=modernColour_.enabled||legacyColour_.enabled;
  harmonics.enabled=harmonics.enabled||(excite>1.e-5f&&colourActive);
  for(auto& band:harmonics.amount)band=std::clamp(band+(1.f-band)*.65f*excite,0.f,1.f);
  harmonics.impact=std::clamp(harmonics.impact+.45f*excite,0.f,1.f);
  harmonics.driveDb=std::clamp(harmonics.driveDb+10.f*excite,0.f,24.f);
  harmonics.even=std::clamp(harmonics.even+.25f*excite,0.f,1.f);
  harmonics.odd=std::clamp(harmonics.odd+.25f*excite,0.f,1.f);
  harmonic_.settings(harmonics);
  auto settings=modernColour_;
  settings.enabled=modernColour_.enabled||legacyColour_.enabled;
  if(legacyColour_.enabled && !modernColour_.enabled){
   settings.mode=legacyColour_.mode==AnalogColorMode::Tape?AnalogizerMode::Tape:
                 legacyColour_.mode==AnalogColorMode::Tube?AnalogizerMode::Tube:
                 AnalogizerMode::FET;
   settings.driveDb=std::min(18.f,legacyColour_.driveDb);
   settings.mix=legacyColour_.mix;
   settings.outputDb=legacyColour_.outputDb;
  }
  settings.driveDb=std::clamp(settings.driveDb+9.f*excite,0.f,18.f);
  settings.bias=legacyColour_.bias;
  settings.tone=legacyColour_.tone;
  settings.dynamics=legacyColour_.dynamics;
  settings.autoGain=legacyColour_.autoGain;
  settings.impact=std::clamp(legacyColour_.impact+.26f*excite,0.f,1.f);
  colour_.settings(settings);
 }
public:
 void prepare(double rate)noexcept {
  rate=std::isfinite(rate)?std::clamp(rate,8000.,384000.):48000.;
  slew_=static_cast<float>(1.-std::exp(-1./(rate*.02)));
  update();harmonic_.prepare(rate);colour_.prepare(rate);scope_.prepare(rate);dryDelay_={};dryPosition_=0;
  mix_=mixTarget_;delta_=deltaTarget_?1.f:0.f;reading_={};
 }
 void reset()noexcept {harmonic_.reset();colour_.reset();scope_.reset();dryDelay_={};dryPosition_=0;reading_={};mix_=mixTarget_;delta_=deltaTarget_?1.f:0.f;}
 void monitor(float mix,bool delta)noexcept {
  mixTarget_=std::isfinite(mix)?std::clamp(mix,0.f,1.f):1.f;deltaTarget_=delta;
 }
 AnalogExciterReading reading()const noexcept{return reading_;}
 void harmonicSettings(HarmonicExciterSettings settings)noexcept {harmonics_=settings;update();}
 void legacySettings(AnalogColorSettings settings)noexcept {legacyColour_=settings;update();}
 void colourSettings(AnalogizerSettings settings)noexcept {modernColour_=settings;update();}
 void excitement(float value)noexcept {excitement_=std::isfinite(value)?std::clamp(value,0.f,1.f):0.f;update();}
 void process(float* left,float* right,int count)noexcept {
  if(!left||!right||count<=0)return;
  std::array<std::array<float,128>,2> dry{};
  double in2=0.,out2=0.,difference2=0.;
  for(int base=0;base<count;base+=128){
   const int n=std::min(128,count-base);
   for(int i=0;i<n;++i){
    for(int c=0;c<2;++c){
     auto& input=(c==0?left:right)[base+i];if(!std::isfinite(input))input=0.f;
     dry[c][i]=dryDelay_[c][dryPosition_];dryDelay_[c][dryPosition_]=input;
    }
    dryPosition_=(dryPosition_+1)%latencySamples();
   }
   harmonic_.process(left+base,right+base,n);
   colour_.process(left+base,right+base,n);
   for(int i=0;i<n;++i){
    mix_+=slew_*(mixTarget_-mix_);delta_+=slew_*((deltaTarget_?1.f:0.f)-delta_);
    for(int c=0;c<2;++c){
     float& output=(c==0?left:right)[base+i];const float input=dry[c][i];
     const float mixed=mix_==1.f?output:(mix_==0.f?input:input+mix_*(output-input));
     const float difference=input-mixed;
     output=mixed+delta_*(difference-mixed);
     in2+=static_cast<double>(input)*input;out2+=static_cast<double>(output)*output;
     difference2+=static_cast<double>(difference)*difference;
    }
    scope_.push(.5f*(dry[0][i]+dry[1][i]),.5f*(left[base+i]+right[base+i]));
   }
  }
  const double scale=1./(2.*count);
  reading_.inputRms=static_cast<float>(std::sqrt(in2*scale));reading_.outputRms=static_cast<float>(std::sqrt(out2*scale));
  reading_.deltaRms=static_cast<float>(std::sqrt(difference2*scale));
  reading_.inputWave=scope_.input();reading_.outputWave=scope_.output();
 }
 HarmonicExciterReading harmonicReading()const noexcept{return harmonic_.reading();}
 AnalogizerReading colourReading()const noexcept{return colour_.reading();}
 // Existing telemetry is retained while the dedicated Color engine is retired.
 AnalogColorReading legacyReading()const noexcept {
  const auto r=colour_.reading();
  return {r.inputRms,r.outputRms,r.deltaRms,0.f};
 }
 static constexpr int latencySamples()noexcept{return Analogizer::latencySamples();}
};
}
