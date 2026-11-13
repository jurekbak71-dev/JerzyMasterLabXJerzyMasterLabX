#pragma once
namespace masterlab {
struct BrainPeak {float frequency=1000,q=1,excess=0,persistence=0,bandDb=-60;};
inline std::vector<BrainPeak> findBrainPeaks(const BrainFeatures& f){
 std::vector<BrainPeak> peaks;if(!f.detail.analysed||f.detail.coverage<12)return peaks;
 std::vector<size_t> candidates;const auto& d=f.detail;
 for(size_t i=10;i+10<brainSpectrumBins;++i){double hz=brainFrequency(i);if(hz>std::min(16000.,f.rate*.4)||d.prominence[i]<6||d.persistence[i]<(hz>4000?.45f:.6f)||d.power[i]<1e-12)continue;
  if(hz<250&&((f.stereoAnalysed&&f.bandCorrelation[0]<.2f)||(20*std::log10(f.peak/std::max(1e-9f,f.rms))>16&&d.persistence[i]<.85f)))continue;
  if(d.prominence[i]>=d.prominence[i-1]&&d.prominence[i]>=d.prominence[i+1])candidates.push_back(i);
 }
 std::sort(candidates.begin(),candidates.end(),[&](size_t a,size_t b){return d.prominence[a]>d.prominence[b];});
 for(size_t i:candidates){bool nearby=false;for(auto peak:peaks)nearby|=std::abs(std::log2(brainFrequency(i)/peak.frequency))<.75;if(nearby)continue;
  const float floor=std::max(5.f,d.prominence[i]*.65f);size_t lo=i,hi=i;while(lo>9&&d.prominence[lo-1]>floor)--lo;while(hi+10<brainSpectrumBins&&d.prominence[hi+1]>floor)++hi;
  double weighted=0,weights=0;for(size_t k=lo;k<=hi;++k){double weight=std::max(.1f,d.prominence[k]-floor);weighted+=weight*std::log(brainFrequency(k));weights+=weight;}
  const float frequency=static_cast<float>(std::exp(weighted/weights)),octaves=std::max(.35f,(hi-lo+1)/6.f),q=std::clamp(1.f/(std::pow(2.f,octaves*.5f)-std::pow(2.f,-octaves*.5f)),.7f,frequency<250?1.4f:5.f);
  const float halfOctave=std::asinh(1/(2*q))/static_cast<float>(std::log(2.));double power=0;for(size_t k=0;k<brainSpectrumBins;++k)if(std::abs(std::log2(brainFrequency(k)/frequency))<=halfOctave+.1)power+=d.power[k];
  peaks.push_back({frequency,q,d.prominence[i],d.persistence[i],static_cast<float>(10*std::log10(std::max(1e-20,power)))});if(peaks.size()==3)break;
 }
 std::sort(peaks.begin(),peaks.end(),[](auto a,auto b){return a.frequency<b.frequency;});return peaks;
}
inline void refineDetailedProposal(BrainProposal& p,const BrainFeatures& f,const BrainConfig& c){
 if(!f.detail.analysed)return;
 const auto peaks=findBrainPeaks(f);auto set=[&](const std::string& id,float value){for(auto& change:p.changes)if(change.first==id){change.second=value;return;}};
 const bool uneven=f.detail.sectionSpreadDb>8&&f.detail.sectionJumpDb>5&&f.detail.sectionLowDb>std::max(-65.f,f.detail.sectionHighDb-36)&&c.style!=3;
 p.reasons.clear();p.warnings.clear();p.gains.fill(0);
 if(c.useEq){for(size_t i=0;i<16;++i){const auto suffix=std::to_string(i+1),id=i?suffix:std::string{};const bool active=i<peaks.size();float cut=active?-std::min(c.style==2?8.f:6.f,std::max(0.f,peaks[i].excess-4)*c.intensity):0;
   set("frequency"+id,active?peaks[i].frequency:1000);set("gain"+id,cut);set("q"+id,active?peaks[i].q:1);set("bandType"+suffix,0);set("bandEnabled"+suffix,active?1.f:0.f);if(i<5)p.gains[i]=cut;
  }set("eqPresent",!c.automaticModules||!peaks.empty()?1.f:0.f);
  p.warnings.push_back("Apply replaces all 16 EQ bands. Undo restores the previous state.");
 }
 if(c.useDynamicEq){for(size_t i=0;i<3;++i){const auto suffix=std::to_string(i+1);bool active=i<peaks.size();float range=active?-std::min(c.useEq?4.f:8.f,std::max(0.f,peaks[i].excess-(c.useEq?6.f:3.f))*c.intensity):0;
   set("deqFreq"+suffix,active?peaks[i].frequency:1000);set("deqQ"+suffix,active?peaks[i].q:1);set("deqGain"+suffix,0);set("deqThreshold"+suffix,active?std::clamp(peaks[i].bandDb-8,-60.f,-6.f):-24);set("deqRange"+suffix,range);set("deqAttack"+suffix,active&&peaks[i].frequency>2000?2.f:10.f);set("deqRelease"+suffix,active&&peaks[i].frequency>2000?90.f:150.f);set("deqBandOn"+suffix,active?1.f:0.f);
  }set("dynamicEqPresent",!c.automaticModules||!peaks.empty()?1.f:0.f);
 }
 if(c.useStabilizer&&c.automaticModules)set("stabilizerPresent",0);
 if(c.useDynamics){float strength=std::clamp((f.detail.sectionSpreadDb-6)/6,0.f,1.f);
  set("dynThreshold",uneven?std::clamp(f.detail.sectionHighDb-5,-48.f,-10.f):-18);set("dynRatio",uneven?2+2*strength:1.2f);set("dynAttack",uneven?3.f:30.f);set("dynRelease",uneven?120.f:180.f);set("dynKnee",6);set("dynMakeup",0);set("dynMix",1);set("upwardAmount",uneven?std::min(6.f,(f.detail.sectionSpreadDb-6)*.5f):0);set("upwardThreshold",std::clamp(f.detail.sectionHighDb-3,-60.f,-6.f));set("dynamicsPresent",!c.automaticModules||uneven?1.f:0.f);
 }
 if(c.useStereo){const auto corr=f.stereoAnalysed?f.bandCorrelation:std::array<float,4>{f.correlation,f.correlation,f.correlation,f.correlation};bool needed=false;
  const std::array<const char*,4> ids{"stereoLowWidth","stereoMidWidth","stereoHighWidth","stereoAirWidth"};
  for(size_t i=0;i<4;++i){float width=1;if(corr[i]<.2f){needed=true;const float limit=i==0?.38f:.75f;width=f.stereoAnalysed?std::clamp(limit/std::max(.01f,f.bandWidth[i]),.2f,1.f):.75f;}set(ids[i],width);}
  float split=140;if(corr[0]<0){for(size_t i=9;i<25;++i)if(f.detail.prominence[i]>8&&f.detail.persistence[i]>.8f)split=std::max(split,static_cast<float>(std::clamp(brainFrequency(i)*4,140.,500.)));}
  set("stereoLowSplit",split);set("stereoMonoBass",corr[0]<0?1.f:0.f);set("stereoPresent",!c.automaticModules||needed?1.f:0.f);
 }
 // Estimate only static EQ here; the actual complete chain is rendered by the worker.
 double before=0,after=0;std::array<double,5> regions{};const std::array<double,6> edges{20,80,250,2000,6000,20000};
 for(size_t i=0;i<brainSpectrumBins;++i){double hz=brainFrequency(i),amplitude=1;before+=f.detail.power[i];if(c.useEq){for(size_t k=0;k<peaks.size();++k){Biquad filter;filter.configure(f.rate,peaks[k].frequency,p.gains[k],peaks[k].q,FilterType::Bell);amplitude*=filter.magnitude(f.rate,hz);}}else if(c.existingEqEnabled){for(auto band:c.existingEq)if(band.enabled){Biquad filter;filter.configure(f.rate,band.frequency,band.gain,band.q,static_cast<FilterType>(band.type));amplitude*=filter.magnitude(f.rate,hz);}}
  double power=f.detail.power[i]*amplitude*amplitude;after+=power;for(size_t region=0;region<5;++region)if(hz>=edges[region]&&hz<edges[region+1])regions[region]+=power;
 }p.eqEstimate=static_cast<float>(10*std::log10(std::max(1e-20,after/std::max(1e-20,before))));double sum=0;for(double value:regions)sum+=value;for(size_t i=0;i<5;++i)p.estimatedShares[i]=static_cast<float>(regions[i]/std::max(1e-20,sum));
 if(c.useLimiter){p.drive=std::clamp(p.targetLufs-f.integrated-p.eqEstimate,0.f,c.style==2?16.f:12.f);set("truePeakDrive",p.drive);set("truePeakCeiling",p.targetCeilingDbtp);set("truePeakOn",!c.automaticModules||p.drive>.25f||20*std::log10(f.peak)>p.targetCeilingDbtp?1.f:0.f);}
 p.reasons.push_back("Detailed analysis: 60 logarithmic spectral bands, local spectral prominence and temporal persistence. No blind genre-curve boosts. Isolated tones and short drum resonances are protected by coverage, persistence and crest checks.");
 for(auto peak:peaks)p.reasons.push_back("Persistent spectral excess near "+std::to_string(static_cast<int>(peak.frequency))+" Hz; prominence "+std::to_string(peak.excess)+" dB; persistence "+std::to_string(static_cast<int>(peak.persistence*100))+"%. Static and dynamic attenuation share a bounded budget; detector thresholds use measured band energy.");
 if(peaks.empty())p.reasons.push_back("No sufficiently persistent local spectral excess: automatic static/dynamic EQ is omitted. A musical timbre is not assumed to be a fault.");
 p.reasons.push_back("Section dynamics: 3-second level spread "+std::to_string(f.detail.sectionSpreadDb)+" dB; largest adjacent section change "+std::to_string(f.detail.sectionJumpDb)+" dB. "+(uneven?"Compression and bounded upward gain target sustained level changes.":"No strong sustained level jumps: automatic compression is omitted to preserve musical dynamics."));
 p.reasons.push_back("Stereo uses measured Side/Mid energy and correlation. Negative low-band correlation can extend Mono Bass crossover around a persistent low-frequency side component.");
 if(c.automaticModules)p.reasons.push_back("Stabilizer is not stacked automatically on targeted EQ corrections. Manually allowed families remain individually controllable.");
 p.warnings.push_back("Explainable signal-analysis rules, not a trained machine-learning model. Spectral prominence can also be intentional timbre; review and audition before Apply.");
 if(uneven)p.warnings.push_back("A sustained section-level change may be an intentional arrangement. Review the dynamics proposal before applying it.");
 if(f.clips)p.warnings.push_back("Input clipping was detected; lost waveform information cannot be reconstructed.");
 p.warnings.push_back("The preview verifies a captured fragment only. Re-measure and audition the whole song; limiter ceiling is sample peak, not certified True Peak.");
}
}
