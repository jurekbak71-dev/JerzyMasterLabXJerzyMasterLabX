#pragma once
#include "BrainPreview.h"
namespace masterlab {
struct BrainScore {float total=1.e9f,loudnessError=0,peakExcessDb=0,grExcessDb=0;bool valid=false;};
struct BrainOptimizationResult {BrainProposal proposal;BrainPreviewResult best;BrainScore initialScore,bestScore;int trials=0;bool complete=false,converged=false;};
inline BrainScore scoreBrainRender(const BrainPreviewResult& render,float targetLufs,float ceilingDbtp,float grBudget){
 BrainScore score;if(!render.complete||!std::isfinite(render.lufs)||!std::isfinite(render.truePeak)||render.truePeak<=0||!std::isfinite(render.gr95))return score;
 score.loudnessError=std::abs(render.lufs-targetLufs);score.peakExcessDb=std::max(0.f,20.f*std::log10(std::max(1.e-9f,render.truePeak))-ceilingDbtp);score.grExcessDb=std::max(0.f,render.gr95-grBudget);
 score.total=score.loudnessError+12.f*score.peakExcessDb+3.f*score.grExcessDb;score.valid=std::isfinite(score.total);return score;
}
template<class Cancel> BrainOptimizationResult optimizeBrainChain(BrainProposal proposal,const BrainFeatures& features,const BrainConfig& config,const std::vector<float>& left,const std::vector<float>& right,Cancel cancel){
 BrainOptimizationResult out;out.proposal=std::move(proposal);if(!out.proposal.valid||!config.hasBaseline||left.empty()||left.size()!=right.size())return out;
 auto values=config.baseline;std::array<std::string,parameterCount> ids;for(size_t i=0;i<ids.size();++i){ids[i]=parameterId(i);for(const auto& change:out.proposal.changes)if(change.first==ids[i])values[i]=change.second;}
 auto index=[&](const char* id){for(size_t i=0;i<ids.size();++i)if(ids[i]==id)return i;return ids.size();};
 const auto driveIndex=index("truePeakDrive"),makeupIndex=index("dynMakeup"),onIndex=index("truePeakOn");
 const float budget=std::array<float,4>{4,5,7,3}[std::clamp(config.style,0,3)];
 auto render=[&](const auto& candidate){++out.trials;return renderBrainPreview(left,right,features.rate,candidate,cancel);};
 out.best=render(values);out.initialScore=out.bestScore=scoreBrainRender(out.best,out.proposal.targetLufs,out.proposal.targetCeilingDbtp,budget);if(!out.bestScore.valid)return out;
 for(int pass=1;pass<6&&!cancel();++pass){auto candidate=values;const float loudnessDelta=out.proposal.targetLufs-out.best.lufs;float driveDelta=loudnessDelta;
  if(out.bestScore.peakExcessDb>0||out.bestScore.grExcessDb>0||features.clips>0)driveDelta=std::min(driveDelta,-std::max({.5f,out.bestScore.peakExcessDb,out.bestScore.grExcessDb}));
  if(features.clips>0)driveDelta=std::min(driveDelta,0.f);
  bool changed=false;if(driveIndex<values.size()){const float next=std::clamp(values[driveIndex]+driveDelta,0.f,18.f);changed=next!=values[driveIndex];candidate[driveIndex]=next;if(onIndex<values.size())candidate[onIndex]=1.f;}
  if(!changed&&makeupIndex<values.size()&&out.best.lufs>out.proposal.targetLufs+.25f){const float next=std::clamp(values[makeupIndex]+loudnessDelta,-12.f,12.f);changed=next!=values[makeupIndex];candidate[makeupIndex]=next;}
  if(!changed){out.converged=true;break;}const auto result=render(candidate);if(!result.complete)break;const auto score=scoreBrainRender(result,out.proposal.targetLufs,out.proposal.targetCeilingDbtp,budget);
  if(score.valid&&score.total+1.e-5f<out.bestScore.total){values=candidate;out.best=result;out.bestScore=score;if(score.loudnessError<.25f&&score.peakExcessDb<.05f&&score.grExcessDb<.1f){out.converged=true;break;}}else break;
 }
 if(cancel())return out;for(auto& change:out.proposal.changes)for(size_t i=0;i<ids.size();++i)if(change.first==ids[i])change.second=values[i];
 out.proposal.drive=driveIndex<values.size()?values[driveIndex]:out.proposal.drive;out.proposal.previewVerified=true;out.proposal.previewPasses=out.trials;out.proposal.previewLufs=out.best.lufs;out.proposal.previewPeak=out.best.truePeak;out.proposal.previewSectionSpread=out.best.spread;out.proposal.previewGr95=out.best.gr95;
 out.proposal.reasons.push_back("Analysis-by-synthesis selected the best valid full-chain render after "+std::to_string(out.trials)+" bounded trials.");
 if(out.bestScore.loudnessError>1)out.proposal.warnings.push_back("Automatic loudness target was not reached within the true-peak and gain-reduction budgets.");
 if(out.bestScore.peakExcessDb>.15f){out.proposal.valid=false;out.proposal.warnings.push_back("Measured true peak exceeds the automatic ceiling; Apply disabled.");}
 if(features.seconds>left.size()/features.rate+.01)out.proposal.warnings.push_back("Full-capture diagnostics and distributed DSP audition cover different durations. Measure the finished song after Apply.");
 out.complete=out.proposal.valid&&out.proposal.previewVerified;return out;
}
}
