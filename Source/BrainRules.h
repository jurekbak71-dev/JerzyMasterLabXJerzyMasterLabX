#pragma once
#include "Dsp.h"
#include <string>
#include <cstdint>
#include <utility>
namespace masterlab {
struct BrainBand {float frequency=1000,gain=0,q=1;int type=0;bool enabled=true;};
struct BrainConfig {
 int genre=0,style=0,seconds=8;float targetLufs=-14,ceiling=-1,intensity=1;
 bool useEq=true,useLimiter=true,existingEqEnabled=true;
 std::array<BrainBand,16> existingEq{};
};
struct BrainFeatures {
 double rate=48000,seconds=0;float integrated=-120,peak=0,rms=0,correlation=0;
 std::array<double,5> energy{};std::uint64_t clips=0;
};
struct BrainProposal {
 bool valid=false;std::vector<std::pair<std::string,float>> changes;
 std::array<float,5> gains{},shares{};float drive=0,eqEstimate=0;
 std::vector<std::string> reasons,warnings;
};
inline constexpr std::array<const char*,9> brainGenres{"Rock","Punk","Post-Punk","Dark Wave","Electronic","Techno","Ambient","Pop","Acoustic"};
inline constexpr std::array<const char*,4> brainStyles{"Transparent","Analog","Aggressive","Atmospheric"};
inline BrainConfig cleanBrainConfig(BrainConfig c){
 c.genre=std::clamp(c.genre,0,8);c.style=std::clamp(c.style,0,3);if(c.seconds!=8&&c.seconds!=16&&c.seconds!=30&&c.seconds!=60)c.seconds=8;
 c.targetLufs=std::clamp(clean(c.targetLufs),-24.f,-8.f);c.ceiling=std::clamp(clean(c.ceiling),-6.f,-.1f);c.intensity=std::clamp(clean(c.intensity),.25f,1.5f);return c;
}
inline BrainProposal proposeMastering(const BrainFeatures& f,BrainConfig config){
 const auto c=cleanBrainConfig(config);BrainProposal p;
 if(!std::isfinite(f.seconds)||!std::isfinite(f.rms)||!std::isfinite(f.correlation)||!std::isfinite(f.integrated)||f.integrated<=-70||!std::isfinite(f.peak)||f.peak<1e-5f||f.seconds<c.seconds-.01||!std::isfinite(f.rate)||f.rate<8000){p.warnings.push_back("Insufficient audible material. Play a full analysis interval.");return p;}
 double sum=0;for(double e:f.energy){if(!std::isfinite(e)||e<0){p.warnings.push_back("Invalid spectral analysis.");return p;}sum+=e;}
 if(sum<1e-20){p.warnings.push_back("No reliable spectrum. No proposal generated.");return p;}
 const std::array<std::array<double,5>,9> weights{{{.65,1.05,1.10,1.,.85},{.55,1.,1.20,1.10,.85},{.75,1.10,1.15,.95,.75},{1.05,1.15,1.,.85,.65},{1.30,1.20,.85,.95,1.10},{1.40,1.20,.85,.90,1.},{.95,1.,1.,.80,.85},{.85,1.05,1.,1.05,1.},{.60,.90,1.15,1.,.90}}};
 const std::array<double,6> edges{20,80,250,2000,6000,20000};
 std::array<double,5> target{};double totalTarget=0;
 for(size_t i=0;i<5;++i){double upper=std::min(edges[i+1],f.rate*.5),lower=std::min(edges[i],upper);target[i]=lower<upper?std::log2(upper/lower)*weights[c.genre][i]:0;totalTarget+=target[i];p.shares[i]=static_cast<float>(f.energy[i]/sum);}
 const std::array<float,5> frequencies{70,180,800,3500,9000},qualities{.7071f,.8f,.65f,.7f,.7071f};
 const std::array<int,5> types{1,0,0,0,2};
 for(size_t i=0;i<5;++i){
  double desired=target[i]/totalTarget;
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
  add("eq",1);p.reasons.push_back("EQ: compare five octave-normalized energy regions with a hand-authored genre profile; corrections are capped at +/-3 dB.");
  p.warnings.push_back("Applying replaces all 16 EQ band settings. Undo restores the previous settings.");
 }else p.reasons.push_back("EQ excluded: existing EQ settings are preserved and included in the level estimate.");
 const std::array<double,5> centres{40,140,700,3500,11000};double predicted=0;float positiveBoost=0;
 for(size_t region=0;region<5;++region){double amplitude=1;
  for(size_t band=0;band<(c.useEq?5:16);++band){
   if(!c.useEq&&(!c.existingEqEnabled||!c.existingEq[band].enabled))continue;
   Biquad filter;float gain=c.useEq?p.gains[band]:c.existingEq[band].gain;
   filter.configure(f.rate,c.useEq?frequencies[band]:c.existingEq[band].frequency,gain,c.useEq?qualities[band]:c.existingEq[band].q,static_cast<FilterType>(c.useEq?types[band]:c.existingEq[band].type));
   amplitude*=filter.magnitude(f.rate,std::min(centres[region],f.rate*.45));positiveBoost=std::max(positiveBoost,std::max(0.f,gain));
  }
  predicted+=f.energy[region]*amplitude*amplitude;
 }
 p.eqEstimate=static_cast<float>(10*std::log10(std::max(1e-20,predicted/sum)));
 if(c.useLimiter){
  const float wanted=c.targetLufs-f.integrated-p.eqEstimate;
  const float peakDb=20*std::log10(f.peak),budget=std::array<float,4>{3,4,6,3}[c.style],maximumDrive=std::array<float,4>{8,10,12,6}[c.style];
  const float peakBudget=std::max(0.f,c.ceiling-peakDb-positiveBoost+budget);
  p.drive=std::clamp(wanted,0.f,std::min(maximumDrive,peakBudget));
  add("limiterDrive",p.drive);add("limiterRelease",std::array<float,4>{120,160,70,220}[c.style]);add("ceiling",c.ceiling);add("lim",1);add("order",0);
  p.reasons.push_back("Limiter: target minus measured input LUFS and estimated EQ level change. Drive is capped by style and estimated peak-reduction budget; limiter moves after EQ.");
  if(wanted<0)p.warnings.push_back("Input is already above the selected target. Drive cannot attenuate it; lower input gain in the DAW.");
  if(wanted>p.drive+.5f)p.warnings.push_back("Drive was restricted to protect dynamics. The requested loudness may not be reached.");
 }else p.reasons.push_back("Limiter excluded: its settings and chain order are preserved.");
 if(f.correlation<0)p.warnings.push_back("Negative input correlation: check mono compatibility. This assistant does not repair phase or stereo width.");
 if(f.clips)p.warnings.push_back("Input samples exceeded 0 dBFS. Reduce upstream level; lost transients cannot be recovered by these rules.");
 p.warnings.push_back("Genre/style profiles are explicit DSP heuristics, not a trained AI model. Analog selects gentle settings; no analog processor is added.");
 p.warnings.push_back("LUFS and EQ predictions describe this fragment only. Re-measure the full processed song. Ceiling is sample peak, not True Peak.");
 p.valid=(c.useEq||c.useLimiter);return p;
}
}
