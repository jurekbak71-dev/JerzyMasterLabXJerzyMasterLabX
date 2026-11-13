#pragma once
#include "BrainRules.h"
#include "GraphConfiguration.h"
#include "LoudnessMeter.h"
namespace masterlab {
class BrainPreviewTruePeakMeter {
 static constexpr int taps=25,phases=4;std::array<std::array<double,taps>,phases> kernel{};std::array<std::array<float,taps>,2> history{};int position=0;
public:
 BrainPreviewTruePeakMeter(){constexpr double pi=3.14159265358979323846;for(int phase=0;phase<phases;++phase){double sum=0;for(int tap=0;tap<taps;++tap){const double t=tap-(taps-1)/2.+static_cast<double>(phase)/phases;const double sinc=std::abs(t)<1e-12?1.:std::sin(pi*t)/(pi*t);const double window=.42-.5*std::cos(2*pi*tap/(taps-1))+.08*std::cos(4*pi*tap/(taps-1));kernel[phase][tap]=sinc*window;sum+=kernel[phase][tap];}for(auto& value:kernel[phase])value/=sum;}}
 float push(float left,float right){history[0][position]=left;history[1][position]=right;position=(position+1)%taps;float peak=0;for(int channel=0;channel<2;++channel)for(int phase=0;phase<phases;++phase){double value=0;int cursor=position;for(int tap=0;tap<taps;++tap){cursor=(cursor+taps-1)%taps;value+=kernel[phase][tap]*history[channel][cursor];}peak=std::max(peak,static_cast<float>(std::abs(value)));}return peak;}
};
struct BrainPreviewResult {float lufs=-120,peak=0,truePeak=0,spread=0,gr95=0;bool complete=false;};
template<class Cancel> BrainPreviewResult renderBrainPreview(const std::vector<float>& left,const std::vector<float>& right,double rate,const std::array<float,parameterCount>& values,Cancel cancel,int requestedBlockSize=128){
 SerialGraph graph;auto get=[&](size_t i){return values[i];};configureGraph(graph,get);graph.prepare(rate,values[6]>.5f,static_cast<int>(values[138]),values[302]>.5f);
 LoudnessMeter loudness;loudness.prepare(rate);BrainDetailAnalyzer dynamics;dynamics.prepare(rate,left.size()/rate);BrainPreviewTruePeakMeter truePeak;
 const size_t blockSize=static_cast<size_t>(std::clamp(requestedBlockSize,1,128));std::array<float,128> l{},r{},trace{};std::vector<float> reductions;reductions.reserve(left.size()/blockSize+2);BrainPreviewResult out;
 const size_t latency=static_cast<size_t>(graph.latency()),total=left.size()+latency;
 for(size_t offset=0;offset<total;offset+=blockSize){if(cancel())return out;int count=static_cast<int>(std::min(blockSize,total-offset));for(int i=0;i<count;++i){size_t index=offset+i;l[i]=index<left.size()?left[index]:0;r[i]=index<right.size()?right[index]:0;}
  graph.process(l.data(),r.data(),count,values[6]>.5f,trace.data());
  // The legacy parallel profile blends after the complete latency-aligned
  // chain in Processor. Reproduce that same operation in the preview.
  if(values[161]>.5f&&values[302]>.5f){
   const float mix=std::clamp(values[303],0.f,1.f);
   for(int i=0;i<count;++i){
    const size_t position=offset+static_cast<size_t>(i);
    const float dryL=position>=latency&&position-latency<left.size()?left[position-latency]:0.f;
    const float dryR=position>=latency&&position-latency<right.size()?right[position-latency]:0.f;
    l[i]=dryL+mix*(l[i]-dryL);r[i]=dryR+mix*(r[i]-dryR);
   }
  }
  for(int i=0;i<count;++i)if(offset+i>=latency){if(!std::isfinite(l[i])||!std::isfinite(r[i]))return out;loudness.push(l[i],r[i]);dynamics.push(l[i],r[i]);out.peak=std::max(out.peak,std::max(std::abs(l[i]),std::abs(r[i])));out.truePeak=std::max(out.truePeak,truePeak.push(l[i],r[i]));}
  reductions.push_back(std::max(0.f,-20*std::log10(std::max(1e-9f,*std::min_element(trace.begin(),trace.begin()+count)))));
 }
 out.lufs=loudness.read().integrated;out.spread=dynamics.finish().sectionSpreadDb;std::sort(reductions.begin(),reductions.end());out.gr95=reductions[static_cast<size_t>((reductions.size()-1)*.95)];out.complete=std::isfinite(out.lufs)&&out.lufs>-100;return out;
}
}
