#pragma once
#include "GainDSP.h"
#include "AnalysisCapture.h"
#include "MeasurementModel.h"
#include <atomic>
#include <cstdint>
#include <array>
namespace ml {
struct GainStage {std::uint64_t id=0;float db=0;bool operator==(const GainStage&)const=default;};
struct RackConfig {std::array<GainStage,10> stages{};std::uint64_t deltaId=0;bool moduleAnalysis=false,listenDelta=false;bool operator==(const RackConfig&)const=default;};
// One producer (control thread), one consumer (audio thread).
// Bounded queue; full means retry on control thread, never overwrite unread data.
template<class T,unsigned N=8> class Spsc {
 static_assert(N>1 && std::atomic<unsigned>::is_always_lock_free);
 std::array<T,N> data{};std::atomic<unsigned> read{0},write{0};
public:
 bool push(const T& v) noexcept {auto w=write.load(std::memory_order_relaxed);auto n=(w+1)%N;if(n==read.load(std::memory_order_acquire))return false;data[w]=v;write.store(n,std::memory_order_release);return true;}
 bool pop(T& v) noexcept {auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return false;v=data[r];read.store((r+1)%N,std::memory_order_release);return true;}
};
struct RackMeters {StereoLevels input,output,delta;};
// prepare only while stopped. Configuration snapshots are copied at block boundaries.
// Removed stages fade to unity before retirement. Gain stages commute; this policy
// is NOT a general solution for reordering nonlinear/latent future processors.
class AudioRack {
 struct Node {std::uint64_t id=0;GainDSP dsp;unsigned retire=0;};
 std::array<Node,20> nodes{};
 RackConfig config{};
 Spsc<RackConfig> commands;
 Spsc<RackMeters> readings;
 AnalysisCapture capture;
 Spsc<AnalysisFrame,5> analysisFrames;
 double rate=44100.;unsigned ramp=882;float monitorMix=0;
public:
 bool submit(const RackConfig& c) noexcept {
  for(std::size_t i=0;i<c.stages.size();++i)if(c.stages[i].id){
   if(!std::isfinite(c.stages[i].db)||std::abs(c.stages[i].db)>24)return false;
   for(std::size_t j=0;j<i;++j)if(c.stages[i].id==c.stages[j].id)return false;
  }
  return commands.push(c);
 }
 bool poll(RackMeters& m) noexcept {return readings.pop(m);}
 bool pollAnalysis(AnalysisFrame& frame) noexcept {return analysisFrames.pop(frame);}
 void prepare(double sr) noexcept {
  rate=std::clamp(std::isfinite(sr)?sr:44100.,1.,768000.);ramp=std::max(1,int(rate*.02));
  capture.restart(rate);monitorMix=0;
  nodes={};
  // Device restart: retain last applied configuration; do not consume the GUI's telemetry queue.
  for(std::size_t i=0;i<config.stages.size();++i){auto s=config.stages[i];if(s.id){nodes[i].id=s.id;nodes[i].dsp.prepare(rate);nodes[i].dsp.setGainDb(s.db);}}
 }
 // Only when BOTH threads are stopped (e.g. between playback sessions).
 void resetWhenStopped() noexcept {
  monitorMix=0;nodes={};config={};RackConfig c;while(commands.pop(c)){};RackMeters m;while(readings.pop(m)){};AnalysisFrame frame;while(analysisFrames.pop(frame)){};capture.restart(rate);
 }
 void process(float* l,float* r,std::size_t n) noexcept {
  if(!n)return;
  RackConfig next;
  // Bound work and defer topology changes until retiring nodes have completed.
  bool retiring=false;for(auto& x:nodes)retiring|=x.retire!=0;
  if(!retiring&&commands.pop(next)){
   for(auto& x:nodes)if(x.id){
    bool keep=false;for(auto s:next.stages)if(s.id==x.id)keep=true;
    if(!keep){x.dsp.setGainDb(0);x.retire=ramp;}
   }
   for(auto s:next.stages)if(s.id){
    Node* node=find(s.id);
    if(!node)for(auto& x:nodes)if(!x.id){x.id=s.id;x.dsp.prepare(rate);node=&x;break;}
    if(node)node->dsp.setGainDb(s.db);
   }
   config=next;
  }
  std::array<Node*,10> active{};
  for(std::size_t j=0;j<config.stages.size();++j)if(config.stages[j].id)active[j]=find(config.stages[j].id);
  const bool module=config.moduleAnalysis && config.deltaId && std::any_of(active.begin(),active.end(),[this](const Node* x){return x&&x->id==config.deltaId;});
  capture.setSource(module?config.deltaId:0);
  RackMeters m;m.input=measure(l,r,n);
  double squares[2]{};float peaks[2]{},outPeaks[2]{};double outSquares[2]{};
  for(std::size_t i=0;i<n;++i){
   float* a=l?l+i:nullptr;float* b=r?r+i:nullptr;
   float dl=0,dr=0;
   float bl=a?*a:0,br=b?*b:0,al=bl,ar=br;
   for(std::size_t j=0;j<active.size();++j)if(auto* x=active[j]){
    const float beforeL=a?*a:0,beforeR=b?*b:0;
    x->dsp.process(a,b,1);
    if(x->id==config.deltaId){dl=a?*a-beforeL:0;dr=b?*b-beforeR:0;if(module){bl=beforeL;br=beforeR;al=a?*a:0;ar=b?*b:0;}}
   }
   for(auto& x:nodes)if(x.retire){x.dsp.process(a,b,1);if(--x.retire==0)x.id=0;}
   if(!module){al=a?*a:0;ar=b?*b:0;}
   capture.push(bl,br,al,ar,[this](const AnalysisFrame& frame){analysisFrames.push(frame);});
   // Telemetry is always pre-monitor. DELTA monitoring is not a processor.
   const float normalL=a?*a:0,normalR=b?*b:0;
   const float out[]{normalL,normalR};
   for(int ch=0;ch<2;++ch)if(std::isfinite(out[ch])){outPeaks[ch]=std::max(outPeaks[ch],std::abs(out[ch]));outSquares[ch]+=double(out[ch])*out[ch];}
   const float target=config.listenDelta&&(!config.moduleAnalysis||module)?1.f:0.f;
   const float step=1.f/float(ramp);
   monitorMix+=std::clamp(target-monitorMix,-step,step);
   if(monitorMix>0){
    const float diffL=al-bl,diffR=ar-br;
    if(a)*a=normalL+(diffL-normalL)*monitorMix;
    if(b&&b!=a)*b=normalR+(diffR-normalR)*monitorMix;
   }
   peaks[0]=std::max(peaks[0],std::abs(dl));peaks[1]=std::max(peaks[1],std::abs(dr));
   squares[0]+=double(dl)*dl;squares[1]+=double(dr)*dr;
  }
  for(int c=0;c<2;++c){m.output.peak[c]=amplitudeDb(outPeaks[c]);m.output.rms[c]=amplitudeDb(float(std::sqrt(outSquares[c]/n)));m.output.clip[c]=outPeaks[c]>=1.f;}
  for(int c=0;c<2;++c){m.delta.peak[c]=amplitudeDb(peaks[c]);m.delta.rms[c]=amplitudeDb(float(std::sqrt(squares[c]/n)));m.delta.clip[c]=peaks[c]>=1;}
  readings.push(m); // drop telemetry if GUI is behind
 }
private:
 Node* find(std::uint64_t id) noexcept{for(auto& x:nodes)if(x.id==id)return &x;return nullptr;}
};
}
