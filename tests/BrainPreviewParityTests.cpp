#include "../Source/Processor.h"
#include "../Source/BrainOptimizer.h"
#include <cstdio>

int main(){
 bool ok=true;
 for(double rate:{44100.,48000.,96000.,192000.})for(bool parallel:{false,true})for(bool insert:{false,true}){
  Processor host;
  auto set=[&](const char* id,float value){auto* p=host.state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(value));};
  set("eq",0);set("lim",0);set("truePeakOn",0);set("routingMode",parallel?1:0);
  set("workMode",insert?1:0);set("insertBlend",.35f);
  set("analogizerPresent",1);set("analogizerOn",1);set("analogizerDrive",12);set("analogizerWarmth",.7f);set("analogExcite",.6f);
  set("restorationPresent",1);set("restorationOn",1);set("restoreHum",1);set("restoreMix",.8f);
  std::array<float,masterlab::parameterCount> values{};
  for(size_t i=0;i<values.size();++i)values[i]=host.parameters[i]->load();
  const int size=static_cast<int>(rate);std::vector<float> left(size),right(size);
  for(int i=0;i<size;++i){left[i]=.16f*std::sin(6.283185307179586*997*i/rate)+.08f*std::sin(6.283185307179586*50*i/rate);right[i]=.8f*left[i];}
  const auto preview=masterlab::renderBrainPreview(left,right,rate,values,[]{return false;});
  const auto partitioned=masterlab::renderBrainPreview(left,right,rate,values,[]{return false;},17);
  host.prepareToPlay(rate,128);const int latency=host.getLatencySamples();
  masterlab::LoudnessMeter meter;meter.prepare(rate);float peak=0;juce::MidiBuffer midi;
  for(int base=0;base<size+latency;base+=128){
   const int count=std::min(128,size+latency-base);juce::AudioBuffer<float> buffer(2,count);
   for(int i=0;i<count;++i){buffer.setSample(0,i,base+i<size?left[base+i]:0);buffer.setSample(1,i,base+i<size?right[base+i]:0);}
   host.processBlock(buffer,midi);
   for(int i=0;i<count;++i)if(base+i>=latency){const float a=buffer.getSample(0,i),b=buffer.getSample(1,i);meter.push(a,b);peak=std::max(peak,std::max(std::abs(a),std::abs(b)));}
  }
  const float loudnessError=std::abs(meter.read().integrated-preview.lufs),peakError=std::abs(peak-preview.peak);
  std::printf("%.0f %s %s preview-host LUFS error=%.7f peak error=%.9f\n",rate,parallel?"parallel":"serial",insert?"INSERT":"MASTER",loudnessError,peakError);
  ok=preview.complete&&partitioned.complete&&loudnessError<1.e-4f&&peakError<1.e-6f&&
     std::abs(partitioned.lufs-preview.lufs)<1.e-5f&&std::abs(partitioned.truePeak-preview.truePeak)<1.e-6f&&ok;
 }
 // The optimizer must retain a valid, non-worsening render and remain bounded
 // on an already-loud clipped fragment.  This is the failure mode where extra
 // drive can make a naive loudness-only optimiser appear "better".
 {
  Processor host;
  masterlab::BrainConfig config;config.hasBaseline=true;config.seconds=8;
  for(size_t i=0;i<config.baseline.size();++i)
   config.baseline[i]=host.state.getRawParameterValue(masterlab::parameterId(i))->load();
  masterlab::BrainFeatures features;features.rate=48000;features.seconds=8;
  features.integrated=-8.f;features.peak=1.35f;features.rms=.55f;
  features.correlation=.98f;features.stereoAnalysed=true;features.clips=400;
  features.energy={1.,1.,1.,1.,1.};
  auto proposal=masterlab::proposeMastering(features,config);
  std::vector<float> left(48000),right(48000);
  for(size_t i=0;i<left.size();++i){
   const float raw=1.35f*std::sin(juce::MathConstants<double>::twoPi*997.*i/48000.);
   left[i]=right[i]=std::clamp(raw,-1.f,1.f);
  }
  const auto optimized=masterlab::optimizeBrainChain(proposal,features,config,left,right,[]{return false;});
  const auto repeated=masterlab::optimizeBrainChain(proposal,features,config,left,right,[]{return false;});
  ok=optimized.complete&&optimized.trials>=1&&optimized.trials<=6&&
     optimized.best.complete&&optimized.best.truePeak>0.f&&
     std::isfinite(optimized.bestScore.total)&&
     optimized.bestScore.total<=optimized.initialScore.total+1.e-5f&&
     optimized.bestScore.peakExcessDb<=.15f&&
     repeated.complete&&repeated.trials==optimized.trials&&
     repeated.proposal.changes==optimized.proposal.changes&&
     std::abs(repeated.bestScore.total-optimized.bestScore.total)<1.e-6f&&ok;
  std::printf("optimizer trials=%d initial=%.4f best=%.4f dBTP-excess=%.4f\n",
              optimized.trials,optimized.initialScore.total,optimized.bestScore.total,
              optimized.bestScore.peakExcessDb);
  auto fixture=[&](const char* name,auto signal,float correlation,std::array<double,5> energy){
   masterlab::BrainFeatures measured=features;measured.integrated=-15.f;measured.peak=.8f;measured.rms=.2f;measured.clips=0;measured.correlation=correlation;measured.bandCorrelation.fill(correlation);measured.energy=energy;
   std::vector<float> a(48000),b(48000);for(size_t i=0;i<a.size();++i){a[i]=signal(i);b[i]=correlation<0?-a[i]:a[i]*.93f;}
   auto recipe=masterlab::proposeMastering(measured,config);auto result=masterlab::optimizeBrainChain(recipe,measured,config,a,b,[]{return false;});
   const bool valid=result.complete&&result.trials<=6&&std::isfinite(result.bestScore.total)&&result.bestScore.total<=result.initialScore.total+1.e-5f&&result.bestScore.peakExcessDb<=.15f;
   std::printf("fixture %s trials=%d score=%.4f valid=%d\n",name,result.trials,result.bestScore.total,valid?1:0);ok=valid&&ok;
  };
  fixture("bass-heavy",[](size_t i){return static_cast<float>(.55*std::sin(juce::MathConstants<double>::twoPi*55.*i/48000.)+.08*std::sin(juce::MathConstants<double>::twoPi*900.*i/48000.));},.95f,{8.,3.,1.,.5,.25});
  fixture("harsh",[](size_t i){return static_cast<float>(.25*std::sin(juce::MathConstants<double>::twoPi*9200.*i/48000.)+.12*std::sin(juce::MathConstants<double>::twoPi*1200.*i/48000.));},.8f,{.5,1.,1.,3.,8.});
  fixture("transient",[](size_t i){return i%2400<8?.75f:static_cast<float>(.08*std::sin(juce::MathConstants<double>::twoPi*440.*i/48000.));},.9f,{1.,1.,2.,2.,1.});
  fixture("anti-phase",[](size_t i){return static_cast<float>(.3*std::sin(juce::MathConstants<double>::twoPi*90.*i/48000.));},-.95f,{5.,3.,1.,.5,.25});
 }
 return ok?0:1;
}
