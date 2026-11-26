#pragma once
#include "../sdk/masterlabpro_module_abi.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>

namespace ml {
template<class T,unsigned N> class GraphSpsc{
 static_assert(N>1&&std::atomic<unsigned>::is_always_lock_free);
 std::array<T,N> values{};std::atomic<unsigned> read{0},write{0};
public:
 bool push(const T& v)noexcept{auto w=write.load(std::memory_order_relaxed),next=(w+1)%N;if(next==read.load(std::memory_order_acquire))return false;values[w]=v;write.store(next,std::memory_order_release);return true;}
 bool pop(T& v)noexcept{auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return false;v=values[r];read.store((r+1)%N,std::memory_order_release);return true;}
 bool canPush()const noexcept{const auto w=write.load(std::memory_order_relaxed);return (w+1)%N!=read.load(std::memory_order_acquire);}
};
using ReadAutomation=float(*)(const void*,uint32_t) noexcept;
inline double automationValue(const MlpParameterDescriptor& d,float input) noexcept {
 const double n=std::clamp(std::isfinite(input)?double(input):0.,0.,1.);
 double v=d.minimum+n*(d.maximum-d.minimum);
 if((d.flags&MLP_PARAMETER_LOGARITHMIC)&&d.minimum>0&&d.maximum>0)v=d.minimum*std::pow(d.maximum/d.minimum,n);
 if(d.step>0)v=d.minimum+std::round((v-d.minimum)/d.step)*d.step;
 return std::clamp(v,d.minimum,d.maximum);
}
struct GraphNode{uint64_t instanceId=0;const MlpModuleApi* api=nullptr;MlpInstance* instance=nullptr;bool bypass=false;const MlpModuleDescriptor* descriptor=nullptr;uint32_t lane=10;const void* automationContext=nullptr;ReadAutomation readAutomation=nullptr;};
struct GraphSnapshot{uint64_t revision=0;std::array<GraphNode,10> nodes{};uint32_t count=0;};
struct GraphDiagnostics{uint64_t blocks=0,nanSamples=0,queueOverloads=0,latencyChanges=0;uint32_t latency=0;uint64_t maximumNanoseconds=0;};
struct GraphTap{uint64_t instanceId=0;float* alignedBeforeLeft=nullptr;float* alignedBeforeRight=nullptr;float* afterLeft=nullptr;float* afterRight=nullptr;bool found=false;uint32_t latency=0;};

class StereoDelayAligner{
 static constexpr uint32_t capacity=65537;
 using Storage=std::array<std::array<float,capacity>,2>;
 std::unique_ptr<Storage> line=std::make_unique<Storage>();uint32_t position=0,delay=0,valid=0;
public:
 bool setDelay(uint32_t samples)noexcept{if(samples>=capacity)return false;delay=samples;return true;}
 void reset()noexcept{position=0;valid=0;}
 void process(float inL,float inR,float& outL,float& outR)noexcept{
  const uint32_t read=(position+capacity-delay)%capacity;
  outL=delay?(valid>=delay?(*line)[0][read]:0.f):inL;outR=delay?(valid>=delay?(*line)[1][read]:0.f):inR;
  (*line)[0][position]=inL;(*line)[1][position]=inR;position=(position+1)%capacity;if(valid<capacity)++valid;
 }
};

class ModularGraph{
public:
 bool submit(const GraphSnapshot& graph)noexcept{
  if(graph.count>graph.nodes.size())return false;
  for(uint32_t i=0;i<graph.count;++i){const auto& n=graph.nodes[i];if(!n.instanceId||!n.api||!n.instance||!n.api->process)return false;if(n.readAutomation&&(!n.descriptor||n.descriptor->parameter_count>64||n.lane>=10||!n.api->set_parameter))return false;for(uint32_t j=0;j<i;++j)if(graph.nodes[j].instanceId==n.instanceId)return false;}
  if(!incoming.push(graph)){queueOverloads.fetch_add(1,std::memory_order_relaxed);return false;}return true;
 }
 bool pollRetired(GraphSnapshot& graph)noexcept{return retired.pop(graph);}
 void prepare(uint32_t maximumFrames)noexcept{maxFrames=std::min<uint32_t>(maximumFrames,2048);phase=Phase::running;aligner.reset();}
 void resetWhenStopped()noexcept{GraphSnapshot value;while(incoming.pop(value)){}while(retired.pop(value)){}active={};pending={};activeRevision.store(0,std::memory_order_release);phase=Phase::running;transition=0;pendingLatency=0;latency.store(0,std::memory_order_relaxed);aligner.reset();tapAligner.reset();tapSource=0;tapLatency=0;bypassIds.fill(0);for(auto& line:bypassLines)line.reset();}
 bool process(float* left,float* right,uint32_t frames,float* deltaLeft=nullptr,float* deltaRight=nullptr,
              float* alignedLeft=nullptr,float* alignedRight=nullptr,GraphTap* tap=nullptr)noexcept{
  if(!left||frames>maxFrames)return false;
  const auto started=std::chrono::steady_clock::now();
  GraphSnapshot next;
  if(phase==Phase::running&&retired.canPush()&&incoming.pop(next)){pending=next;phase=Phase::fadeOutGraph;transition=0;}
  for(uint32_t i=0;i<active.count;++i){const auto& n=active.nodes[i];if(n.readAutomation&&n.descriptor&&n.lane<10){for(uint32_t p=0;p<n.descriptor->parameter_count;++p){const auto& d=n.descriptor->parameters[p];n.api->set_parameter(n.instance,d.id,automationValue(d,n.readAutomation(n.automationContext,n.lane*64+p)));}}}
  if(phase==Phase::running){const auto reported=reportedLatency();if(reported!=latency.load(std::memory_order_relaxed)){pendingLatency=reported;phase=Phase::fadeOutLatency;transition=0;}}
  for(uint32_t i=0;i<frames;++i){before[0][i]=std::isfinite(left[i])?left[i]:0.f;before[1][i]=right&&std::isfinite(right[i])?right[i]:before[0][i];left[i]=before[0][i];if(right)right[i]=before[1][i];}
  float* channels[]{left,right?right:left};MlpProcessContext context{sizeof(context),frames,right?2u:1u,0,channels};
  if(tap)tap->found=false;
  for(uint32_t node=0;node<active.count;++node){
   const auto& current=active.nodes[node];const bool selected=tap&&tap->instanceId==current.instanceId;
   if(selected)for(uint32_t i=0;i<frames;++i){tapRaw[0][i]=left[i];tapRaw[1][i]=right?right[i]:left[i];}
   auto& dry=bypassLines[node];const auto delay=current.api->get_latency_samples(current.instance);
   if(delay>65536)return false;
   if(bypassIds[node]!=current.instanceId){dry.reset();bypassIds[node]=current.instanceId;}
   dry.setDelay(delay);
   for(uint32_t i=0;i<frames;++i){float dl=0,dr=0;dry.process(left[i],right?right[i]:left[i],dl,dr);if(current.bypass){dryBlock[0][i]=dl;dryBlock[1][i]=dr;}}
   // Keep the native processor warm even during bypass, discarding its wet output.
   if(current.api->process(current.instance,&context)!=MLP_OK)return false;
   if(current.bypass)for(uint32_t i=0;i<frames;++i){left[i]=dryBlock[0][i];if(right)right[i]=dryBlock[1][i];}
   if(selected){tap->found=true;tap->latency=current.api->get_latency_samples(current.instance);for(uint32_t i=0;i<frames;++i){if(tap->afterLeft)tap->afterLeft[i]=left[i];if(tap->afterRight)tap->afterRight[i]=right?right[i]:left[i];}}
  }
  if(phase!=Phase::running){for(uint32_t i=0;i<frames;++i){const float x=float(std::min<uint32_t>(transition++,transitionSamples))/float(transitionSamples);const bool fadingOut=phase==Phase::fadeOutGraph||phase==Phase::fadeOutLatency;const float gain=fadingOut?1.f-x:x;left[i]*=gain;if(right)right[i]*=gain;}}
  for(uint32_t i=0;i<frames;++i){if(!std::isfinite(left[i])){left[i]=0;nanSamples.fetch_add(1,std::memory_order_relaxed);}if(right&&!std::isfinite(right[i])){right[i]=0;nanSamples.fetch_add(1,std::memory_order_relaxed);}}
  for(uint32_t i=0;i<frames;++i){float al=0,ar=0;aligner.process(before[0][i],before[1][i],al,ar);if(alignedLeft)alignedLeft[i]=al;if(alignedRight)alignedRight[i]=ar;if(deltaLeft)deltaLeft[i]=left[i]-al;if(deltaRight)deltaRight[i]=(right?right[i]:left[i])-ar;}
  if(tap&&tap->found){if(tapSource!=tap->instanceId||tapLatency!=tap->latency){tapSource=tap->instanceId;tapLatency=tap->latency;tapAligner.setDelay(tapLatency);tapAligner.reset();}for(uint32_t i=0;i<frames;++i){float al=0,ar=0;tapAligner.process(tapRaw[0][i],tapRaw[1][i],al,ar);if(tap->alignedBeforeLeft)tap->alignedBeforeLeft[i]=al;if(tap->alignedBeforeRight)tap->alignedBeforeRight[i]=ar;}}
  if(phase==Phase::fadeOutGraph&&transition>=transitionSamples){
   if(retired.push(active)){active=pending;activeRevision.store(active.revision,std::memory_order_release);phase=Phase::fadeIn;transition=0;refreshLatency();}
   else queueOverloads.fetch_add(1,std::memory_order_relaxed);
  }
  else if(phase==Phase::fadeOutLatency&&transition>=transitionSamples){latency.store(pendingLatency,std::memory_order_release);aligner.setDelay(pendingLatency);aligner.reset();latencyChanges.fetch_add(1,std::memory_order_relaxed);phase=Phase::fadeIn;transition=0;}
  else if(phase==Phase::fadeIn&&transition>=transitionSamples){phase=Phase::running;transition=0;}
  blocks.fetch_add(1,std::memory_order_relaxed);const auto elapsed=uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-started).count());auto old=maximumNs.load(std::memory_order_relaxed);while(old<elapsed&&!maximumNs.compare_exchange_weak(old,elapsed,std::memory_order_relaxed)){}
  return true;
 }
 GraphDiagnostics diagnostics()const noexcept{return {blocks.load(),nanSamples.load(),queueOverloads.load(),latencyChanges.load(),latency.load(),maximumNs.load()};}
 uint64_t revision()const noexcept{return activeRevision.load(std::memory_order_acquire);}
private:
 enum class Phase{running,fadeOutGraph,fadeOutLatency,fadeIn};
 uint32_t reportedLatency()const noexcept{uint32_t total=0;for(uint32_t i=0;i<active.count;++i){const auto value=active.nodes[i].api->get_latency_samples(active.nodes[i].instance);total=value>65536-total?65536:total+value;}return total;}
 void refreshLatency()noexcept{const auto total=reportedLatency();latency.store(total,std::memory_order_release);aligner.setDelay(total);aligner.reset();}
 GraphSpsc<GraphSnapshot,8> incoming,retired;GraphSnapshot active{},pending{};Phase phase=Phase::running;uint32_t transition=0,maxFrames=2048,pendingLatency=0;static constexpr uint32_t transitionSamples=128;
 std::array<std::array<float,2048>,2> before{},tapRaw{},dryBlock{};std::array<StereoDelayAligner,10> bypassLines;std::array<uint64_t,10> bypassIds{};StereoDelayAligner aligner,tapAligner;uint64_t tapSource=0;uint32_t tapLatency=0;
 std::atomic<uint64_t> activeRevision{0},blocks{0},nanSamples{0},queueOverloads{0},latencyChanges{0},maximumNs{0};std::atomic<uint32_t> latency{0};
};
}
