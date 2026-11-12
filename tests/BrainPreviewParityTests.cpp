#include "../Source/Processor.h"
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
  ok=preview.complete&&loudnessError<1.e-4f&&peakError<1.e-6f&&ok;
 }
 return ok?0:1;
}
