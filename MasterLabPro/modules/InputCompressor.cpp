#include "masterlabpro_input_compressor.h"
#include "CompressorCurve.h"
#include "InputCompressorCrossover.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <new>

namespace {
constexpr auto parameters = [] {
 std::array<MlpParameterDescriptor,46> result{};
 auto put = [&](size_t index, uint32_t id, const char* name, const char* unit,
                double min, double max, double neutral, double step=0,
                uint32_t flags=0) {
  result[index]={sizeof(MlpParameterDescriptor),id,flags|MLP_PARAMETER_AUTOMATABLE,0,
                 name,unit,min,max,neutral,neutral,step,10};
 };
 put(0,1,"Input Gain","dB",-18,18,0); put(1,2,"Makeup Gain","dB",-18,18,0);
 put(2,3,"Low / Mid","Hz",40,800,180,0,MLP_PARAMETER_LOGARITHMIC);
 put(3,4,"Mid / High","Hz",900,12000,3200,0,MLP_PARAMETER_LOGARITHMIC);
 put(4,5,"Analog","",0,1,0,1,MLP_PARAMETER_INTEGER);
 put(5,6,"Bypass","",0,1,0,1,MLP_PARAMETER_INTEGER);
 for (uint32_t band=0; band<4; ++band) {
  const auto index=6u+7u*band;
  put(index,100+band,"Threshold","dB",-60,0,0);
  put(index+1,110+band,"Ratio",":1",1,20,1,0,MLP_PARAMETER_LOGARITHMIC);
  put(index+2,120+band,"Attack","ms",0.1,150,15,0,MLP_PARAMETER_LOGARITHMIC);
  put(index+3,130+band,"Release","ms",10,1500,150,0,MLP_PARAMETER_LOGARITHMIC);
  put(index+4,140+band,"Knee","dB",0,24,6);
  put(index+5,150+band,"Mix","%",0,100,100);
  put(index+6,160+band,"Stereo Link","%",0,100,100);
 }
 for(uint32_t band=0;band<4;++band){
  put(34+band,170+band,"Band Makeup","dB",-18,18,0);
  put(38+band,180+band,"Band Active","",0,1,1,1,MLP_PARAMETER_INTEGER);
  put(42+band,190+band,"Stereo Width","%",0,200,100);
 }
 return result;
}();
constexpr MlpModuleDescriptor descriptor{sizeof(MlpModuleDescriptor),MLP_ABI_MAJOR,MLP_ABI_MINOR,
 1,MLP_INPUT_COMPRESSOR_HI,MLP_INPUT_COMPRESSOR_LO,
 MLP_CAP_STATE|MLP_CAP_STEREO|MLP_CAP_NONLINEAR|MLP_CAP_CONCURRENT_STATE_READ,
 1,2,uint32_t(parameters.size()),0,"Input Compressor","Three-band and master-bus dynamics","Dynamics",parameters.data()};
constexpr uint32_t stateBytes=4+4*uint32_t(parameters.size());
static_assert(std::atomic<float>::is_always_lock_free && sizeof(float)==4);

float clean(float x) noexcept {return std::clamp(ml::flushCompressorSample(x),-64.f,64.f);}
float amplitude(float db) noexcept {return std::exp(db*0.1151292546497023f);}
}

struct MlpInstance {
 std::array<std::atomic<float>,parameters.size()> targets{};
 std::array<float,parameters.size()> values{};
 std::array<std::atomic<float>,4> meters{};
 std::array<std::array<float,2>,4> envelopes{};
 std::array<std::array<float,2>,4> rms{},memory{};
 std::array<std::atomic<uint64_t>,4> dynamics{};
 std::array<ml::CompressorCrossover,2> crossovers{};
 float rate=48000, smoothing=0;
 uint32_t maximumFrames=0, channels=0;
 MlpInstance() noexcept {
  for (size_t i=0;i<parameters.size();++i) targets[i].store(float(parameters[i].default_value));
 }
};

namespace {
const MlpModuleDescriptor* MLP_CALL describe() {return &descriptor;}
MlpResult MLP_CALL create(MlpInstance** p) {
 if (!p) return MLP_ERROR_ARGUMENT;
 *p=new(std::nothrow) MlpInstance;
 return *p ? MLP_OK : MLP_ERROR_INTERNAL;
}
void MLP_CALL destroy(MlpInstance* p) {delete p;}
void MLP_CALL reset(MlpInstance* p) {
 if (!p) return;
 p->envelopes={}; p->crossovers={};p->rms={};p->memory={};
 for (auto& meter:p->meters) meter.store(0,std::memory_order_relaxed);
 for(auto& capture:p->dynamics)capture.store(0,std::memory_order_relaxed);
 for (size_t i=0;i<parameters.size();++i) p->values[i]=p->targets[i].load(std::memory_order_relaxed);
}
MlpResult MLP_CALL prepare(MlpInstance* p,double rate,uint32_t frames,uint32_t channels) {
 if (!p||!std::isfinite(rate)||rate<8000||rate>768000||!frames||!channels||channels>2) return MLP_ERROR_ARGUMENT;
 p->rate=float(rate); p->maximumFrames=frames; p->channels=channels;
 p->smoothing=float(-std::expm1(-100./rate)); reset(p);
 return MLP_OK;
}
MlpResult MLP_CALL set(MlpInstance* p,uint32_t id,double value) {
 if (!p||!std::isfinite(value)) return MLP_ERROR_ARGUMENT;
 for (size_t i=0;i<parameters.size();++i) if (parameters[i].id==id) {
  const auto& d=parameters[i];
  p->targets[i].store(float(std::clamp(d.step==1?std::round(value):value,d.minimum,d.maximum)),std::memory_order_relaxed);
  return MLP_OK;
 }
 return MLP_ERROR_ARGUMENT;
}
MlpResult MLP_CALL get(const MlpInstance* p,uint32_t id,double* value) {
 if (!p||!value) return MLP_ERROR_ARGUMENT;
 for (size_t i=0;i<parameters.size();++i) if (parameters[i].id==id) {
  *value=p->targets[i].load(std::memory_order_relaxed);
  return MLP_OK;
 }
 return MLP_ERROR_ARGUMENT;
}
using Pair=std::array<float,2>;
Pair compress(MlpInstance& p,uint32_t band,Pair signal,float analog,Pair& applied) noexcept {
 const float* v=p.values.data()+6+band*7;
 const float threshold=v[0], ratio=v[1], knee=v[4];
 const float maximum=std::max(std::abs(signal[0]),std::abs(signal[1]));
 const auto dry=signal;
 const float attack=float(-std::expm1(-1000./(p.rate*v[2])));
 const float release=float(-std::expm1(-1000./(p.rate*v[3])));
 for (size_t ch=0;ch<2;++ch) {
  const float peak=std::lerp(std::abs(signal[ch]),maximum,v[6]*.01f);
  if(analog>0){
   auto& rms=p.rms[band][ch];
   rms=ml::flushCompressorSample(std::lerp(rms,peak*peak,std::min(1.f,100.f/p.rate)));
  }
  const float detection=analog>0?std::lerp(peak,std::sqrt(p.rms[band][ch]),analog*.32f):peak;
  const float level=20*std::log10(std::max(1e-10f,detection));
  const float target=ml::compressorReduction(level,threshold,ratio,knee,analog);
  auto& envelope=p.envelopes[band][ch];
  auto& memory=p.memory[band][ch];
  if(analog>0)memory=ml::flushCompressorSample(std::lerp(memory,target,std::min(1.f,4.f/p.rate)));
  const float attackScale=1+analog*(.45f+std::min(2.f,(target-envelope)*.15f));
  const float releaseScale=std::lerp(1.f,std::clamp(.55f+memory*.22f,.55f,4.f),analog);
  envelope=std::lerp(envelope,target,target>envelope?std::min(1.f,attack*attackScale):std::min(1.f,release/releaseScale));
  if (envelope<1e-9f) envelope=0;
  const float gain=std::lerp(1.f,amplitude(-envelope),v[5]*.01f);
  signal[ch]*=gain;
  applied[ch]=-20*std::log10(gain);
 }
 const float active=p.values[38+band],makeup=amplitude(p.values[34+band]),width=p.values[42+band]*.01f;
 const float mid=(signal[0]+signal[1])*.5f,side=(signal[0]-signal[1])*.5f*width;
 signal={std::lerp(dry[0],(mid+side)*makeup,active),std::lerp(dry[1],(mid-side)*makeup,active)};
 for(auto& gr:applied)gr*=active;
 return signal;
}
MlpResult MLP_CALL process(MlpInstance* p,const MlpProcessContext* ctx) {
 if (!p||!ctx||ctx->struct_size<sizeof(*ctx)||!ctx->channels||ctx->channels>p->channels||
     ctx->frames>p->maximumFrames||!ctx->audio||!ctx->audio[0]||
     (ctx->channels==2&&(!ctx->audio[1]||ctx->audio[0]==ctx->audio[1]))) return MLP_ERROR_ARGUMENT;
 std::array<float,parameters.size()> target{};
 std::array<Pair,4> reductions{};
 std::array<Pair,4> peaks{};
 for (size_t i=0;i<target.size();++i) target[i]=p->targets[i].load(std::memory_order_relaxed);
 for (uint32_t i=0;i<ctx->frames;++i) {
  for (size_t j=0;j<target.size();++j) {
   auto& value=p->values[j];
   const float next=std::lerp(value,target[j],p->smoothing);
   value=next==value||std::abs(next-target[j])<2e-6f*std::max(1.f,std::abs(target[j]))?target[j]:next;
  }
  const Pair dry{clean(ctx->audio[0][i]),clean(ctx->audio[ctx->channels-1][i])};
  const float input=amplitude(p->values[0]), makeup=amplitude(p->values[1]), analog=p->values[4];
  const float lowCoefficient=ml::CompressorCrossover::coefficient(p->values[2],p->rate);
  const float highCoefficient=ml::CompressorCrossover::coefficient(p->values[3],p->rate);
  Pair low{}, mid{}, high{}, driven{};
  for (size_t ch=0;ch<2;++ch) {
   driven[ch]=dry[ch]*input;
   const auto bands=p->crossovers[ch].process(driven[ch],lowCoefficient,highCoefficient);
   low[ch]=bands[0];mid[ch]=bands[1];high[ch]=bands[2];
  }
  const auto lowWet=compress(*p,0,low,analog,reductions[0]);
  const auto midWet=compress(*p,1,mid,analog,reductions[1]);
  const auto highWet=compress(*p,2,high,analog,reductions[2]);
  Pair sum{};
  for (size_t ch=0;ch<2;++ch)
   sum[ch]=driven[ch]+(lowWet[ch]-low[ch])+(midWet[ch]-mid[ch])+(highWet[ch]-high[ch]);
  const auto sumDry=sum;
  sum=compress(*p,3,sum,analog,reductions[3]);
  const std::array<Pair,4> before{low,mid,high,sumDry},after{lowWet,midWet,highWet,sum};
  for(size_t b=0;b<peaks.size();++b)for(size_t ch=0;ch<2;++ch){
   peaks[b][0]=std::max(peaks[b][0],std::abs(before[b][ch]));
   peaks[b][1]=std::max(peaks[b][1],std::abs(std::lerp(after[b][ch],before[b][ch],p->values[5])));
  }
  for (size_t ch=0;ch<ctx->channels;++ch) {
   const float wet=sum[ch]*makeup;
   const float drive=.76f+.032f*std::min(15.f,reductions[3][ch]);
   constexpr float bias=.19f,biasValue=.1877462f,slope=1-biasValue*biasValue;
   const float colored=analog>0?std::lerp(wet,(std::tanh(wet*drive+bias)-biasValue)/(drive*slope),analog*.31f):wet;
   ctx->audio[ch][i]=std::lerp(colored,dry[ch],p->values[5]);
  }
 }
 if(ctx->frames)for(size_t i=0;i<reductions.size();++i){
  p->meters[i].store(std::max(reductions[i][0],reductions[i][1])*(1-p->values[5]),std::memory_order_relaxed);
  p->dynamics[i].store((uint64_t(std::bit_cast<uint32_t>(peaks[i][0]))<<32)|std::bit_cast<uint32_t>(peaks[i][1]),std::memory_order_relaxed);
 }
 return MLP_OK;
}
uint32_t MLP_CALL latency(const MlpInstance*) {return 0;}
MlpResult MLP_CALL stateSize(const MlpInstance* p,uint32_t* size) {
 if (!p||!size) return MLP_ERROR_ARGUMENT;
 *size=stateBytes; return MLP_OK;
}
MlpResult MLP_CALL save(const MlpInstance* p,void* data,uint32_t capacity,uint32_t* written) {
 if (!p||!written) return MLP_ERROR_ARGUMENT;
 *written=stateBytes;
 if (!data||capacity<stateBytes) return MLP_ERROR_CAPACITY;
 auto* bytes=static_cast<uint8_t*>(data);
 auto write=[&](size_t offset,uint32_t value) {for (unsigned i=0;i<4;++i) bytes[offset+i]=uint8_t(value>>(i*8));};
 write(0,0x31434c4d);
 for (size_t i=0;i<parameters.size();++i) write(4+4*i,std::bit_cast<uint32_t>(p->targets[i].load(std::memory_order_relaxed)));
 return MLP_OK;
}
MlpResult MLP_CALL load(MlpInstance* p,const void* data,uint32_t size) {
 if (!p||!data||(size!=stateBytes&&size!=4+4*34)) return MLP_ERROR_ARGUMENT;
 const auto* bytes=static_cast<const uint8_t*>(data);
 auto read=[&](size_t offset) {uint32_t value=0;for (unsigned i=0;i<4;++i) value|=uint32_t(bytes[offset+i])<<(8*i);return value;};
 if (read(0)!=0x31434c4d) return MLP_ERROR_STATE;
 std::array<float,parameters.size()> values{};
 for (size_t i=0;i<values.size();++i) {
  values[i]=i<(size-4)/4?std::bit_cast<float>(read(4+4*i)):float(parameters[i].default_value);
  if (!std::isfinite(values[i])||values[i]<float(parameters[i].minimum)||values[i]>float(parameters[i].maximum)) return MLP_ERROR_STATE;
 }
 for (size_t i=0;i<values.size();++i) set(p,parameters[i].id,values[i]);
 return MLP_OK;
}
}

extern "C" MLP_EXPORT float MLP_CALL masterlabpro_read_gain_reduction_v1(const MlpInstance* p,uint32_t band) {
 return p&&band<4?p->meters[band].load(std::memory_order_relaxed):0.f;
}
extern "C" MLP_EXPORT uint64_t MLP_CALL masterlabpro_read_dynamics_v1(const MlpInstance* p,uint32_t band) {
 return p&&band<4?p->dynamics[band].load(std::memory_order_relaxed):0;
}
extern "C" MLP_EXPORT MlpResult MLP_CALL masterlabpro_get_module_api(uint32_t major,uint32_t,MlpModuleApi* api) {
 if (!api||api->struct_size<sizeof(*api)) return MLP_ERROR_ARGUMENT;
 if (major!=MLP_ABI_MAJOR) return MLP_ERROR_ABI;
 *api={sizeof(*api),MLP_ABI_MAJOR,MLP_ABI_MINOR,0,describe,create,destroy,prepare,reset,process,set,get,latency,stateSize,save,load};
 return MLP_OK;
}
