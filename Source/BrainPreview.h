#pragma once
#include "BrainRules.h"
#include "GraphConfiguration.h"
#include "LoudnessMeter.h"
#include "FinalTruePeak.h"
namespace masterlab {
struct BrainPreviewResult {float lufs=-120,peak=0,truePeakDb=-120,spread=0,gr95=0,correlation=0,crestDb=0;bool complete=false;
 std::array<float,128> outputEnvelope{};};
template<class Cancel> BrainPreviewResult renderBrainPreview(const std::vector<float>& left,const std::vector<float>& right,double rate,const std::array<float,parameterCount>& values,Cancel cancel){
 SerialGraph graph;auto get=[&](size_t i){return values[i];};configureGraph(graph,get);graph.prepare(rate,values[6]>.5f,static_cast<int>(values[138]));
 LoudnessMeter loudness;loudness.prepare(rate);BrainDetailAnalyzer dynamics;dynamics.prepare(rate,left.size()/rate);
 FinalTruePeak truePeakMeter;truePeakMeter.prepare(rate);truePeakMeter.settings({false,-1.f,120.f,0.f});float oversampledPeak=0;
 std::array<float,128> l{},r{},trace{};std::vector<float> reductions;reductions.reserve(left.size()/128+2);BrainPreviewResult out;
 double ll=0.,rr=0.,lr=0.;std::uint64_t measured=0;
 const size_t latency=static_cast<size_t>(graph.latency()),total=left.size()+latency;
 for(size_t offset=0;offset<total;offset+=128){if(cancel())return out;int count=static_cast<int>(std::min(size_t{128},total-offset));for(int i=0;i<count;++i){size_t index=offset+i;l[i]=index<left.size()?left[index]:0;r[i]=index<right.size()?right[index]:0;}
  graph.process(l.data(),r.data(),count,values[6]>.5f,trace.data());
  for(int i=0;i<count;++i)if(offset+i>=latency){if(!std::isfinite(l[i])||!std::isfinite(r[i]))return out;loudness.push(l[i],r[i]);dynamics.push(l[i],r[i]);out.peak=std::max(out.peak,std::max(std::abs(l[i]),std::abs(r[i])));
   ll+=static_cast<double>(l[i])*l[i];rr+=static_cast<double>(r[i])*r[i];lr+=static_cast<double>(l[i])*r[i];
   const auto time=offset+static_cast<size_t>(i)-latency;
   const size_t bin=std::min<size_t>(127,time*128/left.size());
   out.outputEnvelope[bin]=std::max(out.outputEnvelope[bin],std::max(std::abs(l[i]),std::abs(r[i])));
   ++measured;}
  // Meter sees the rendered audio but does not affect the values sent to loudness.
  truePeakMeter.process(l.data(),r.data(),count);
  oversampledPeak=std::max(oversampledPeak,truePeakMeter.reading().detectedTruePeak);
  // The fixed final stage performs gain reduction independently of the
  // historical limiter trace. Score what the listener actually hears.
  const auto tpReduction=graph.finalTruePeakRead().gainReductionDb;
  reductions.push_back(values[221]>.5f&&values[222]>.5f?
      std::max(0.f,tpReduction):
      std::max(0.f,-20*std::log10(std::max(1e-9f,*std::min_element(trace.begin(),trace.begin()+count)))));
 }
 out.correlation=ll*rr>1.e-20?static_cast<float>(std::clamp(lr/std::sqrt(ll*rr),-1.,1.)):0.f;
 const float outputRms=measured?static_cast<float>(std::sqrt((ll+rr)/(2.*measured))):0.f;
 out.crestDb=20.f*std::log10(std::max(1.e-9f,out.peak)/std::max(1.e-9f,outputRms));
 out.truePeakDb=20.f*std::log10(std::max(1.e-9f,oversampledPeak));out.lufs=loudness.read().integrated;out.spread=dynamics.finish().sectionSpreadDb;std::sort(reductions.begin(),reductions.end());out.gr95=reductions[static_cast<size_t>((reductions.size()-1)*.95)];out.complete=std::isfinite(out.lufs)&&out.lufs>-100;return out;
}
template<class Cancel> void verifyBrainProposal(BrainProposal& proposal,const BrainFeatures& features,const BrainConfig& config,const std::vector<float>& left,const std::vector<float>& right,Cancel cancel){
 if(!proposal.valid||!config.hasBaseline)return;
 auto values=config.baseline;std::array<std::string,parameterCount> ids;for(size_t i=0;i<ids.size();++i){ids[i]=parameterId(i);for(const auto& change:proposal.changes)if(change.first==ids[i])values[i]=change.second;}
 auto render=[&](const auto& v){++proposal.previewPasses;return renderBrainPreview(left,right,features.rate,v,cancel);};
 auto best=render(values);if(!best.complete){proposal.valid=false;proposal.warnings.push_back("DSP verification failed; Apply disabled.");return;}
 // Test stronger compression only when sustained level changes were measured.
 if(config.useDynamics&&values[98]>.5f&&features.detail.sectionSpreadDb>8&&best.spread>6){auto candidate=values;candidate[101]=std::max(-48.f,values[101]-4);candidate[102]=std::min(5.f,values[102]+1);candidate[108]=std::min(8.f,values[108]+2);auto result=render(candidate);if(!result.complete)return;if(result.spread<best.spread-.5f){values=candidate;best=result;proposal.reasons.push_back("Actual DSP preview selected stronger compression after measuring a lower section-level spread.");}}
 // A loud input may require attenuation rather than any extra limiter drive.
 // Use the visible existing makeup control; no hidden gain is applied to audio.
 if(config.useDynamics&&best.lufs>config.targetLufs+.5f){auto candidate=values;
  if(candidate[98]<.5f){candidate[98]=candidate[99]=1;candidate[102]=1;candidate[100]=0;candidate[108]=0;candidate[107]=1;}
  candidate[106]=std::clamp(values[106]+config.targetLufs-best.lufs,-12.f,12.f);auto result=render(candidate);if(!result.complete)return;
  if(std::abs(result.lufs-config.targetLufs)<std::abs(best.lufs-config.targetLufs)){values=candidate;best=result;proposal.reasons.push_back("Measured input was above the loudness target. Visible compressor makeup supplies attenuation; no hidden post-render gain is used.");}
 }
 const float budget=std::array<float,4>{4,5,7,3}[config.style];
 // When the dedicated True-Peak Maximizer is active, legacy limiterDrive
 // is deliberately ignored by configureGraph. Optimise the REAL parameter.
 const bool dedicatedFinal=values[221]>.5f&&values[222]>.5f;
 const size_t driveIndex=dedicatedFinal?225u:52u;
 const float driveMax=dedicatedFinal?18.f:24.f;
 if(config.useLimiter||config.useTruePeak){
  for(int pass=0;pass<5;++pass){
   float correction=std::clamp(config.targetLufs-best.lufs,-3.f,3.f);
   if(best.gr95>budget)correction=std::min(correction,-.75f);
   if(std::abs(correction)<.2f)break;
   auto candidate=values;
   candidate[driveIndex]=std::clamp(values[driveIndex]+correction,0.f,driveMax);
   if(candidate[driveIndex]==values[driveIndex])break;
   auto result=render(candidate);
   if(!result.complete)return;
   // Preserve musical transients: never accept excess TP or extra gain
   // reduction simply to chase an unattainable LUFS target.
   const bool peakSafe=result.truePeakDb<=config.ceiling+.1f;
   const bool dynamicsSafe=result.gr95<=budget+.25f;
   const bool loudnessBetter=std::abs(result.lufs-config.targetLufs)+.1f<
                             std::abs(best.lufs-config.targetLufs);
   if(peakSafe&&((dynamicsSafe&&loudnessBetter)||
      (best.gr95>budget&&result.gr95<best.gr95-.2f))){
     values=candidate;best=result;
   }else break;
  }
 }
 if(cancel())return;
 bool driveChanged=false;
 for(auto& change:proposal.changes)for(size_t i=0;i<ids.size();++i)
  if(change.first==ids[i]){change.second=values[i];if(i==driveIndex)driveChanged=true;}
 if(!driveChanged&&(config.useLimiter||config.useTruePeak))
  proposal.changes.emplace_back(ids[driveIndex],values[driveIndex]);
 proposal.drive=values[driveIndex];proposal.previewVerified=true;proposal.previewLufs=best.lufs;proposal.previewPeak=best.peak;proposal.previewTruePeakDb=best.truePeakDb;proposal.previewCorrelation=best.correlation;proposal.previewOutputEnvelope=best.outputEnvelope;proposal.previewCrestDb=best.crestDb;proposal.previewSectionSpread=best.spread;proposal.previewGr95=best.gr95;
 proposal.reasons.push_back("Measured full-chain DSP preview: "+std::to_string(best.lufs)+" LUFS; section spread "+std::to_string(best.spread)+" dB; limiter reduction p95 "+std::to_string(best.gr95)+" dB. No host parameters were changed during verification.");
 if(best.truePeakDb>config.ceiling+.1f){proposal.valid=false;proposal.warnings.push_back("4x true-peak overshoot: candidate rejected. Do not Apply.");}
 if(std::abs(best.lufs-config.targetLufs)>1)proposal.warnings.push_back("Requested loudness was not reached within the limiting budget. The measured preview value takes precedence over the target.");
 if(best.gr95>budget+.5f)proposal.warnings.push_back("Limiting exceeds the preferred reduction budget even at the tested drive. Review the ceiling and input level.");
 if(features.seconds>left.size()/features.rate+.01)proposal.warnings.push_back("Full-length diagnostics cover every 3-second section. DSP audition samples four 4-second windows distributed across the capture; it is not a full-song render. Measure the finished song after Apply.");
}
}
