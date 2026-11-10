#pragma once
// Unified Analog Exciter: one four-band harmonic generator plus one mutually
// exclusive Tube/FET/Tape colour transfer (not three serial saturators).
// Legacy host parameter IDs are adapted here without allocating in audio.
#include "HarmonicExciter.h"
#include "Analogizer.h"
#include "AnalogColor.h"

namespace masterlab {
class AnalogExciter {
 HarmonicExciter harmonic_;
 Analogizer colour_;
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
  settings.impact=std::clamp(settings.impact+.26f*excite,0.f,1.f);
  settings.bias=legacyColour_.bias;
  settings.tone=legacyColour_.tone;
  settings.dynamics=legacyColour_.dynamics;
  settings.autoGain=legacyColour_.autoGain;
  settings.impact=legacyColour_.impact;
  colour_.settings(settings);
 }
public:
 void prepare(double rate)noexcept {
  update();harmonic_.prepare(rate);colour_.prepare(rate);
 }
 void reset()noexcept {harmonic_.reset();colour_.reset();}
 void harmonicSettings(HarmonicExciterSettings settings)noexcept {harmonics_=settings;update();}
 void legacySettings(AnalogColorSettings settings)noexcept {legacyColour_=settings;update();}
 void colourSettings(AnalogizerSettings settings)noexcept {modernColour_=settings;update();}
 void excitement(float value)noexcept {excitement_=std::isfinite(value)?std::clamp(value,0.f,1.f):0.f;update();}
 void process(float* left,float* right,int count)noexcept {
  harmonic_.process(left,right,count);
  colour_.process(left,right,count);
 }
 HarmonicExciterReading harmonicReading()const noexcept{return harmonic_.reading();}
 AnalogizerReading colourReading()const noexcept{return colour_.reading();}
 // Existing telemetry is retained while the dedicated Color engine is retired.
 AnalogColorReading legacyReading()const noexcept {
  const auto r=colour_.reading();
  return {r.inputRms,r.outputRms,r.deltaRms,0.f};
 }
 int latencySamples()const noexcept{return 0;}
};
}
