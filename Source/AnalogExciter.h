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
 void update()noexcept {
  harmonic_.settings(harmonics_);
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
