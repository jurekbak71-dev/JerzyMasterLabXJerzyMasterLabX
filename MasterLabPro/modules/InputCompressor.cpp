#include "masterlabpro_input_compressor.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <new>
#include <numbers>

namespace {
constexpr auto parameters = [] {
 std::array<MlpParameterDescriptor,34> result{};
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
 return result;
}();
constexpr MlpModuleDescriptor descriptor{sizeof(MlpModuleDescriptor),MLP_ABI_MAJOR,MLP_ABI_MINOR,
 1,MLP_INPUT_COMPRESSOR_HI,MLP_INPUT_COMPRESSOR_LO,
 MLP_CAP_STATE|MLP_CAP_STEREO|MLP_CAP_NONLINEAR|MLP_CAP_CONCURRENT_STATE_READ,
 1,2,uint32_t(parameters.size()),0,"Input Compressor","Three-band and master-bus dynamics","Dynamics",parameters.data()};
constexpr uint32_t stateBytes=4+4*uint32_t(parameters.size());
static_assert(std::atomic<float>::is_always_lock_free && sizeof(float)==4);

float reduction(float level, float threshold, float ratio, float knee) noexcept {
 const float over=level-threshold;
 const float slope=1-1/ratio;
 if (knee>0 && std::abs(over)<knee*.5f) {
  const float offset=over+knee*.5f;
  return slope*offset*offset/(2*knee);
 }
 return slope*std::max(0.f,over);
}
float clean(float x) noexcept {return std::isfinite(x)&&std::abs(x)>=1e-25f ? std::clamp(x,-64.f,64.f) : 0.f;}
float amplitude(float db) noexcept {return std::exp(db*0.1151292546497023f);}
}

struct MlpInstance {
 std::array<std::atomic<float>,parameters.size()> targets{};
 std::array<float,parameters.size()> values{};
 std::array<std::atomic<float>,4> meters{};
 std::array<std::array<float,2>,4> envelopes{};
 std::array<float,2> lowState{}, upperState{};
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
 p->envelopes={}; p->lowState={}; p->upperState={};
 for (auto& meter:p->meters) meter.store(0,std::memory_order_relaxed);
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
 const float threshold=v[0], ratio=v[1], knee=v[4]+analog*4;
 const float maximum=std::max(std::abs(signal[0]),std::abs(signal[1]));
 if(ratio==1.f&&p.envelopes[band][0]==0&&p.envelopes[band][1]==0){applied={};return signal;}
 const float attack=float(-std::expm1(-1000./(p.rate*v[2])));
 const float release=float(-std::expm1(-1000./(p.rate*v[3])));
 for (size_t ch=0;ch<2;++ch) {
  const float peak=std::lerp(std::abs(signal[ch]),maximum,v[6]*.01f);
  const float level=20*std::log10(std::max(1e-10f,peak));
  const float target=reduction(level,threshold,ratio,knee);
  auto& envelope=p.envelopes[band][ch];
  const float releaseScale=1+analog*std::min(2.f,envelope*.13f);
  envelope=std::lerp(envelope,target,target>envelope?attack:release/releaseScale);
  if (envelope<1e-9f) envelope=0;
  const float gain=std::lerp(1.f,amplitude(-envelope),v[5]*.01f);
  signal[ch]*=gain;
  applied[ch]=-20*std::log10(gain);
 }
 return signal;
}
MlpResult MLP_CALL process(MlpInstance* p,const MlpProcessContext* ctx) {
 if (!p||!ctx||ctx->struct_size<sizeof(*ctx)||!ctx->channels||ctx->channels>p->channels||
     ctx->frames>p->maximumFrames||!ctx->audio||!ctx->audio[0]||
     (ctx->channels==2&&(!ctx->audio[1]||ctx->audio[0]==ctx->audio[1]))) return MLP_ERROR_ARGUMENT;
 std::array<float,parameters.size()> target{};
 std::array<Pair,4> reductions{};
 for (size_t i=0;i<target.size();++i) target[i]=p->targets[i].load(std::memory_order_relaxed);
 for (uint32_t i=0;i<ctx->frames;++i) {
  for (size_t j=0;j<target.size();++j) {
   auto& value=p->values[j];
   const float next=std::lerp(value,target[j],p->smoothing);
   value=next==value||std::abs(next-target[j])<2e-6f*std::max(1.f,std::abs(target[j]))?target[j]:next;
  }
  const Pair dry{clean(ctx->audio[0][i]),clean(ctx->audio[ctx->channels-1][i])};
  const float input=amplitude(p->values[0]), makeup=amplitude(p->values[1]), analog=p->values[4];
  const auto lowpass=[&](float hz) {
   const float g=std::tan(std::numbers::pi_v<float>*std::min(hz,p->rate*.45f)/p->rate);
   return g/(1+g);
  };
  const float lowCoefficient=lowpass(p->values[2]), highCoefficient=lowpass(p->values[3]);
  Pair low{}, mid{}, high{}, driven{};
  for (size_t ch=0;ch<2;++ch) {
   driven[ch]=dry[ch]*input;
   const float l=(driven[ch]-p->lowState[ch])*lowCoefficient;
   low[ch]=l+p->lowState[ch]; p->lowState[ch]=clean(low[ch]+l);
   const float h=(driven[ch]-p->upperState[ch])*highCoefficient;
   const float upper=h+p->upperState[ch]; p->upperState[ch]=clean(upper+h);
   mid[ch]=upper-low[ch]; high[ch]=driven[ch]-upper;
  }
  const auto lowWet=compress(*p,0,low,analog,reductions[0]);
  const auto midWet=compress(*p,1,mid,analog,reductions[1]);
  const auto highWet=compress(*p,2,high,analog,reductions[2]);
  Pair sum{};
  for (size_t ch=0;ch<2;++ch)
   sum[ch]=driven[ch]+(lowWet[ch]-low[ch])+(midWet[ch]-mid[ch])+(highWet[ch]-high[ch]);
  sum=compress(*p,3,sum,analog,reductions[3]);
  for (size_t ch=0;ch<ctx->channels;++ch) {
   const float wet=sum[ch]*makeup;
   // Mild parallel saturation. Unity slope near zero, no hard clipping at 0 dBFS.
   const float colored=analog>0?std::lerp(wet,2.f*std::tanh(wet*.5f),analog*.18f):wet;
   ctx->audio[ch][i]=std::lerp(colored,dry[ch],p->values[5]);
  }
 }
 if(ctx->frames)for(size_t i=0;i<reductions.size();++i)
  p->meters[i].store(std::max(reductions[i][0],reductions[i][1])*(1-p->values[5]),std::memory_order_relaxed);
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
 if (!p||!data||size!=stateBytes) return MLP_ERROR_ARGUMENT;
 const auto* bytes=static_cast<const uint8_t*>(data);
 auto read=[&](size_t offset) {uint32_t value=0;for (unsigned i=0;i<4;++i) value|=uint32_t(bytes[offset+i])<<(8*i);return value;};
 if (read(0)!=0x31434c4d) return MLP_ERROR_STATE;
 std::array<float,parameters.size()> values{};
 for (size_t i=0;i<values.size();++i) {
  values[i]=std::bit_cast<float>(read(4+4*i));
  if (!std::isfinite(values[i])||values[i]<float(parameters[i].minimum)||values[i]>float(parameters[i].maximum)) return MLP_ERROR_STATE;
 }
 for (size_t i=0;i<values.size();++i) set(p,parameters[i].id,values[i]);
 return MLP_OK;
}
}

extern "C" MLP_EXPORT float MLP_CALL masterlabpro_read_gain_reduction_v1(const MlpInstance* p,uint32_t band) {
 return p&&band<4?p->meters[band].load(std::memory_order_relaxed):0.f;
}
extern "C" MLP_EXPORT MlpResult MLP_CALL masterlabpro_get_module_api(uint32_t major,uint32_t,MlpModuleApi* api) {
 if (!api||api->struct_size<sizeof(*api)) return MLP_ERROR_ARGUMENT;
 if (major!=MLP_ABI_MAJOR) return MLP_ERROR_ABI;
 *api={sizeof(*api),MLP_ABI_MAJOR,MLP_ABI_MINOR,0,describe,create,destroy,prepare,reset,process,set,get,latency,stateSize,save,load};
 return MLP_OK;
}
