#pragma once
#include "Dsp.h"
#include "BrainDetail.h"
#include "ModuleParameters.h"
#include <string>
#include <cstdint>
#include <utility>
namespace masterlab {
struct BrainBand {float frequency=1000,gain=0,q=1;int type=0;bool enabled=true;};
struct BrainConfig {
 int genre=0,style=0,seconds=8;float targetLufs=-14,ceiling=-1,intensity=1;
 bool automaticModules=true;
 bool useStabilizer=true;
 bool useEq=true,useLimiter=true,existingEqEnabled=true,useStereo=true,useDynamics=true,useDynamicEq=true;int dynamicsMode=0;
 std::array<BrainBand,16> existingEq{};
 bool hasBaseline=false;std::array<float,parameterCount> baseline{};
};
struct BrainFeatures {
 BrainDetail detail;double rate=48000,seconds=0;float integrated=-120,peak=0,rms=0,correlation=0;
 std::array<double,5> energy{};std::array<float,4> bandCorrelation{},bandWidth{};bool stereoAnalysed=false;std::uint64_t clips=0;
};
struct BrainProposal {
 bool previewVerified=false;float previewLufs=-120,previewPeak=0,previewSectionSpread=0,previewGr95=0;int previewPasses=0;
 bool valid=false;std::vector<std::pair<std::string,float>> changes;
 std::array<float,5> gains{},shares{},targetShares{},estimatedShares{};float drive=0,eqEstimate=0;
 std::vector<std::string> reasons,warnings;
};
inline constexpr std::array<const char*,9> brainGenres{"Rock","Punk","Post-Punk","Dark Wave","Electronic","Techno","Ambient","Pop","Acoustic"};
inline constexpr std::array<const char*,4> brainStyles{"Transparent","Analog","Aggressive","Atmospheric"};
inline BrainConfig cleanBrainConfig(BrainConfig c){
 c.genre=std::clamp(c.genre,0,8);c.style=std::clamp(c.style,0,3);
 // Long-form sectional analysis, bounded to ten minutes of continuous DAW input.
 if(c.seconds!=8&&c.seconds!=16&&c.seconds!=30&&c.seconds!=60&&
    c.seconds!=120&&c.seconds!=180&&c.seconds!=300&&c.seconds!=600)c.seconds=8;
 c.targetLufs=std::clamp(clean(c.targetLufs),-24.f,-8.f);c.ceiling=std::clamp(clean(c.ceiling),-6.f,-.1f);c.intensity=std::clamp(clean(c.intensity),.25f,1.5f);return c;
}
inline void refineDetailedProposal(BrainProposal&,const BrainFeatures&,const BrainConfig&);
inline BrainProposal proposeMastering(const BrainFeatures& f,BrainConfig config){
 const auto c=cleanBrainConfig(config);BrainProposal p;
 if(!std::isfinite(f.seconds)||!std::isfinite(f.rms)||!std::isfinite(f.correlation)||!std::isfinite(f.integrated)||f.integrated<=-70||!std::isfinite(f.peak)||f.peak<1e-5f||f.seconds<c.seconds-.01||!std::isfinite(f.rate)||f.rate<8000){p.warnings.push_back("Insufficient audible material. Play a full analysis interval.");return p;}
 if(f.detail.analysed){const auto& d=f.detail;bool valid=std::isfinite(d.coverage)&&d.coverage>=0&&d.coverage<=60&&std::isfinite(d.sectionLowDb)&&std::isfinite(d.sectionHighDb)&&std::isfinite(d.sectionSpreadDb)&&d.sectionSpreadDb>=0&&std::isfinite(d.sectionJumpDb)&&d.sectionJumpDb>=0;
  for(size_t i=0;i<brainSpectrumBins;++i)valid=valid&&std::isfinite(d.power[i])&&d.power[i]>=0&&std::isfinite(d.prominence[i])&&std::isfinite(d.persistence[i])&&d.persistence[i]>=0&&d.persistence[i]<=1;
  if(!valid){p.warnings.push_back("Invalid detailed analysis. No proposal generated.");return p;}
 }
 double sum=0;for(double e:f.energy){if(!std::isfinite(e)||e<0){p.warnings.push_back("Invalid spectral analysis.");return p;}sum+=e;}
 if(sum<1e-20){p.warnings.push_back("No reliable spectrum. No proposal generated.");return p;}
 if(f.stereoAnalysed)for(float correlation:f.bandCorrelation)if(!std::isfinite(correlation)||std::abs(correlation)>1){p.warnings.push_back("Invalid stereo analysis.");return p;}
 const std::array<std::array<double,5>,9> weights{{{.65,1.05,1.10,1.,.85},{.55,1.,1.20,1.10,.85},{.75,1.10,1.15,.95,.75},{1.05,1.15,1.,.85,.65},{1.30,1.20,.85,.95,1.10},{1.40,1.20,.85,.90,1.},{.95,1.,1.,.80,.85},{.85,1.05,1.,1.05,1.},{.60,.90,1.15,1.,.90}}};
 const std::array<double,6> edges{20,80,250,2000,6000,20000};
 std::array<double,5> target{};double totalTarget=0;
 for(size_t i=0;i<5;++i){double upper=std::min(edges[i+1],f.rate*.5),lower=std::min(edges[i],upper);target[i]=lower<upper?std::log2(upper/lower)*weights[c.genre][i]:0;totalTarget+=target[i];p.shares[i]=static_cast<float>(f.energy[i]/sum);}
 const std::array<float,5> frequencies{70,180,800,3500,9000},qualities{.7071f,.8f,.65f,.7f,.7071f};
 const std::array<int,5> types{1,0,0,0,2};
 for(size_t i=0;i<5;++i){
  double desired=target[i]/totalTarget;p.targetShares[i]=static_cast<float>(desired);
  if(p.shares[i]>1e-5&&desired>1e-5){double deviation=10*std::log10(p.shares[i]/desired);float correction=std::abs(deviation)>1?static_cast<float>(-.35*deviation):0;
   const float styleScale=std::array<float,4>{.75f,.85f,1.f,.65f}[c.style];p.gains[i]=std::clamp(correction*c.intensity*styleScale,-3.f,3.f);
  }
 }
 auto add=[&](std::string id,float value){p.changes.emplace_back(std::move(id),value);};
 if(c.useEq){
  for(size_t i=0;i<16;++i){auto suffix=std::to_string(i+1),bandSuffix=i==0?std::string{}:suffix;
   add("bandType"+suffix,i<5?static_cast<float>(types[i]):0);add("bandEnabled"+suffix,i<5&&std::abs(p.gains[i])>.01f?1.f:0.f);
   add("frequency"+bandSuffix,i<5?frequencies[i]:1000);add("gain"+bandSuffix,i<5?p.gains[i]:0);add("q"+bandSuffix,i<5?qualities[i]:1);
  }
  add("eq",1);add("eqPresent",1);p.reasons.push_back("EQ: compare five octave-normalized energy regions with a hand-authored genre profile; corrections are capped at +/-3 dB.");
  p.warnings.push_back("Applying replaces all 16 EQ band settings. Undo restores the previous settings.");
 }else p.reasons.push_back("EQ excluded: existing EQ settings are preserved and included in the level estimate.");
 const std::array<double,5> centres{40,140,700,3500,11000};double predicted=0;float positiveBoost=0;
 for(size_t region=0;region<5;++region){double amplitude=1;
  for(size_t band=0;band<(c.useEq?5:16);++band){
   if(c.useEq&&std::abs(p.gains[band])<=.01f)continue;
   if(!c.useEq&&(!c.existingEqEnabled||!c.existingEq[band].enabled))continue;
   Biquad filter;float gain=c.useEq?p.gains[band]:c.existingEq[band].gain;
   filter.configure(f.rate,c.useEq?frequencies[band]:c.existingEq[band].frequency,gain,c.useEq?qualities[band]:c.existingEq[band].q,static_cast<FilterType>(c.useEq?types[band]:c.existingEq[band].type));
   amplitude*=filter.magnitude(f.rate,std::min(centres[region],f.rate*.45));positiveBoost=std::max(positiveBoost,std::max(0.f,gain));
  }
  const double regionEnergy=f.energy[region]*amplitude*amplitude;
  p.estimatedShares[region]=static_cast<float>(regionEnergy/sum);predicted+=regionEnergy;
 }
 for(auto& share:p.estimatedShares)share=static_cast<float>(share/(predicted/sum));
 p.eqEstimate=static_cast<float>(10*std::log10(std::max(1e-20,predicted/sum)));
 if(c.useLimiter){
  const float wanted=c.targetLufs-f.integrated-p.eqEstimate;
  const float peakDb=20*std::log10(f.peak),budget=std::array<float,4>{3,4,6,3}[c.style],maximumDrive=std::array<float,4>{8,10,12,6}[c.style];
  const float peakBudget=std::max(0.f,c.ceiling-peakDb-positiveBoost+budget);
  p.drive=std::clamp(wanted,0.f,std::min(maximumDrive,peakBudget));
  add("limiterDrive",p.drive);add("limiterRelease",std::array<float,4>{120,160,70,220}[c.style]);add("ceiling",c.ceiling);add("lim",1);add("limPresent",1);add("order",0);
  p.reasons.push_back("Limiter: target minus measured input LUFS and estimated EQ level change. Drive is capped by style and estimated peak-reduction budget; limiter moves after EQ.");
  if(wanted<0)p.warnings.push_back("Input is already above the selected target. Drive cannot attenuate it; lower input gain in the DAW.");
  if(wanted>p.drive+.5f)p.warnings.push_back("Drive was restricted to protect dynamics. The requested loudness may not be reached.");
 }else p.reasons.push_back("Limiter excluded: its settings and chain order are preserved.");
 if(f.correlation<0)p.warnings.push_back("Negative input correlation: check mono compatibility. Narrowing can help mono compatibility but cannot reconstruct missing phase information.");
 if(f.clips)p.warnings.push_back("Input samples exceeded 0 dBFS. Reduce upstream level; lost transients cannot be recovered by these rules.");
 p.warnings.push_back("Genre/style profiles are explicit DSP heuristics, not a trained AI model. Analog selects gentle settings and Analog compressor when selected; no standalone Analog Laboratory is added.");
 p.warnings.push_back("Limiter loudness estimate excludes compressor, Dynamic EQ, Stabilizer and stereo changes. Their interaction requires a full output measurement after Apply.");
 p.warnings.push_back("LUFS and EQ predictions describe this fragment only. Re-measure the full processed song. Ceiling is sample peak, not True Peak.");
 if(c.useStereo){
  add("stereoPresent",1);add("stereoOn",1);const auto correlation=f.stereoAnalysed?f.bandCorrelation:std::array<float,4>{f.correlation,f.correlation,f.correlation,f.correlation};add("stereoLowWidth",correlation[0]<.2f?.65f:1.f);add("stereoMidWidth",correlation[1]<.2f?.85f:1.f);add("stereoHighWidth",correlation[2]<.2f?.9f:1.f);add("stereoLowSplit",140);add("stereoHighSplit",2500);add("stereoMonoBass",correlation[0]<0?1.f:0.f);add("stereoMono",0);add("stereoBalance",0);add("stereoAirWidth",correlation[3]<.2f?.9f:1.f);add("stereoTopSplit",8000);for(int band=1;band<=4;++band){add("stereoBandOn"+std::to_string(band),1);add("stereoSolo"+std::to_string(band),0);}add("stereoAmount",1);add("stereoDelta",0);
  p.reasons.push_back("Stereo: preserve width when correlated; narrow conservatively for low/negative measured band correlation (broadband fallback if band data is unavailable). No artificial widening. Inspect per-band correlation and mono after Apply.");
 }
 if(c.useDynamics){
  const float rmsDb=20*std::log10(std::max(1e-9f,f.rms)),crest=20*std::log10(f.peak)-rmsDb;
  add("dynamicsPresent",1);add("dynamicsOn",1);add("dynamicsMode",c.style==1||c.dynamicsMode>.5f?1.f:0.f);add("dynThreshold",std::clamp(rmsDb+8,-36.f,-8.f));add("dynRatio",crest>14?1.6f:1.2f);add("dynAttack",30);add("dynRelease",180);add("dynKnee",6);add("dynMakeup",0);add("dynMix",1);add("upwardAmount",c.style==2?2.f:0.f);add("upwardThreshold",std::clamp(rmsDb-10,-60.f,-12.f));
  p.reasons.push_back("Compressor: Analog style selects Analog character, otherwise preserves the chosen mode. Bounded ratio from measured crest factor, no makeup; upward at most 2 dB in Aggressive style. Re-measure loudness: compressor interaction is not included in the limiter estimate.");
 }
 if(c.useDynamicEq){
  add("dynamicEqPresent",1);add("dynamicEqOn",1);
  for(size_t band=0;band<3;++band){auto id=std::to_string(band+1);const size_t region=std::array<size_t,3>{1,3,4}[band];add("deqFreq"+id,std::array<float,3>{180,3500,7500}[band]);add("deqGain"+id,0);add("deqQ"+id,1);add("deqThreshold"+id,std::clamp(20*std::log10(std::max(1e-9f,f.rms))-6,-48.f,-12.f));add("deqRange"+id,p.shares[region]>p.targetShares[region]*1.4f?-2.f:0.f);add("deqAttack"+id,15);add("deqRelease"+id,160);add("deqBandOn"+id,1);}
  p.reasons.push_back("Dynamic EQ: up to 2 dB attenuation in broad regions with excess measured energy. This is not resonance detection; static EQ and Dynamic EQ may overlap. Listen and adjust.");
 }
 if(c.useStabilizer){add("stabilizerPresent",1);add("stabilizerOn",1);add("stabilizerMode",0);add("stabilizerProfile",c.genre==4||c.genre==5?1.f:(c.genre==8?2.f:0.f));add("stabilizerAmount",std::min(35.f,20*c.intensity));add("stabilizerSpeed",65);add("stabilizerSmoothing",70);add("stabilizerLow",70);add("stabilizerMid",100);add("stabilizerHigh",70);add("stabilizerTame",1);add("stabilizerDelta",0);p.reasons.push_back("Stabilizer: original adaptive octave-band EQ, conservative intensity and slow response. Three explicit profiles, no learned reference. Re-measure the complete output.");}
 // Select only processors justified by the measured fragment. Excluded families
 // remain untouched. Every decision is reviewable before Apply.
 if(c.automaticModules){
  auto replace=[&](const char* id,bool enabled){for(auto& change:p.changes)if(change.first==id)change.second=enabled?1.f:0.f;};
  bool tonal=false,spectral=false,stereo=false;for(float gain:p.gains)tonal|=std::abs(gain)>.25f;
  for(size_t band:std::array<size_t,3>{1,3,4})spectral|=p.shares[band]>p.targetShares[band]*1.4f;
  const auto correlations=f.stereoAnalysed?f.bandCorrelation:std::array<float,4>{f.correlation,f.correlation,f.correlation,f.correlation};for(float correlation:correlations)stereo|=correlation<.2f;
  const float crest=20*std::log10(f.peak/std::max(1e-9f,f.rms));
  const bool dynamics=crest>14||(c.style==2&&crest>10),limiting=p.drive>.25f||20*std::log10(f.peak)>c.ceiling;
  int largeRegions=0;for(float gain:p.gains)largeRegions+=std::abs(gain)>1.5f;const bool stabilizing=largeRegions>=3;
  if(c.useStabilizer)replace("stabilizerPresent",stabilizing);
  if(c.useEq)replace("eqPresent",tonal);if(c.useDynamicEq)replace("dynamicEqPresent",spectral);if(c.useStereo)replace("stereoPresent",stereo);if(c.useDynamics)replace("dynamicsPresent",dynamics);if(c.useLimiter)replace("limPresent",limiting);
  const int count=(c.useStabilizer&&stabilizing)+(c.useEq&&tonal)+(c.useDynamicEq&&spectral)+(c.useStereo&&stereo)+(c.useDynamics&&dynamics)+(c.useLimiter&&limiting);
  p.reasons.push_back("Automatic module selection: "+std::to_string(count)+" processors from the allowed families. EQ: correction above 0.25 dB; Dynamic EQ: regional energy above 1.4 times target; Imager: correlation below 0.2; Dynamics: crest above 14 dB (10 dB Aggressive); Limiter: required drive above 0.25 dB or input above ceiling. Stabilizer: at least three broad corrections above 1.5 dB. Unneeded allowed modules are removed, with settings retained.");
 }
 // Only change the full topology when all families are under Brain control.
 if(c.useEq&&c.useLimiter&&c.useStereo&&c.useDynamics&&c.useDynamicEq&&c.useStabilizer){add("chainOrder",0);p.reasons.push_back("Proposed order: Stabilizer, Dynamic EQ, Dynamics, Imager, EQ, Limiter; absent modules are skipped. Limiter is last. One instance per family is currently supported.");}
 refineDetailedProposal(p,f,c);
 // Sectional whole-track diagnostics from every captured 3-second window.
 if(f.detail.sectionLevelsDb.size()>1){
  const auto& levels=f.detail.sectionLevelsDb;
  float loudest=*std::max_element(levels.begin(),levels.end());
  float quietest=*std::min_element(levels.begin(),levels.end());
  int peakSection=static_cast<int>(std::max_element(levels.begin(),levels.end())-levels.begin());
  int lowSection=static_cast<int>(std::min_element(levels.begin(),levels.end())-levels.begin());
  p.reasons.push_back("Whole capture: "+std::to_string(levels.size())+
    " consecutive ~3-second sections; loudest section near "+std::to_string(peakSection*3)+
    "s ("+std::to_string(loudest)+" dB), quietest near "+std::to_string(lowSection*3)+
    "s ("+std::to_string(quietest)+" dB).");
  if(loudest-quietest>12.f)p.warnings.push_back(
   "Large section-to-section level contrast (>12 dB): review quiet and loud parts before applying one master.");
  if(f.detail.sectionJumpDb>9.f)p.warnings.push_back(
   "Abrupt level transition between adjacent sections: review arrangements and limiter pumping.");
 }
 if(f.detail.windowSpreadDb<2.f&&f.peak>0.2f)p.warnings.push_back(
  "Low short-window dynamic contrast: potential overcompression, verify perceived punch.");
 if(f.correlation<-.15f)p.warnings.push_back(
  "Strong negative stereo correlation: check phase and mono fold-down.");
 // Quantitative comparison to authored genre tonal reference (not a copied song).
 constexpr std::array<const char*,5> referenceBands{"sub 20-80 Hz","bass 80-250 Hz",
                                      "mids 250 Hz-2 kHz","presence 2-6 kHz","air 6-20 kHz"};
 p.reasons.push_back(std::string("GENRE REFERENCE: ")+brainGenres[c.genre]+
  " tonal distribution from five authored octave-normalized weightings (not external copyrighted audio).");
 for(size_t region=0;region<5;++region){
  if(p.targetShares[region]<1e-5f)continue;
  const float delta=10.f*std::log10(std::max(1e-5f,p.shares[region])/
                                   std::max(1e-5f,p.targetShares[region]));
  p.reasons.push_back(std::string(referenceBands[region])+": "+
                      std::to_string(delta)+" dB relative to genre target.");
  if(std::abs(delta)>6.f)
   p.warnings.push_back(std::string("Large genre-reference deviation in ")+
      referenceBands[region]+"; check whether this is an intentional arrangement or a mix issue.");
 }
 p.valid=(c.useEq||c.useLimiter||c.useStereo||c.useDynamics||c.useDynamicEq||c.useStabilizer);return p;
}
}

#include "BrainRefinement.h"
