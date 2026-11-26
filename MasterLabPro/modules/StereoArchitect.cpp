#include "StereoArchitectSettings.h"
#include "InputCompressorCrossover.h"
#include <atomic>
#include <bit>
#include <new>

namespace {
constexpr auto parameters=[] {
 std::array<MlpParameterDescriptor,14> result{};
 for(uint32_t i=0;i<7;++i)
  result[i]={sizeof(MlpParameterDescriptor),i+1,MLP_PARAMETER_AUTOMATABLE,0,ml::stereoBands[i],"%",0,200,100,100,0,20};
 constexpr const char* names[]{"SUB / LOW","LOW / LOW MID","LOW MID / MID","MID / HIGH MID","HIGH MID / HIGH","HIGH / AIR"};
 for(uint32_t i=0;i<6;++i)
  result[7+i]={sizeof(MlpParameterDescriptor),8+i,MLP_PARAMETER_AUTOMATABLE|MLP_PARAMETER_LOGARITHMIC,0,names[i],"Hz",20,19000,ml::stereoSplits[i],ml::stereoSplits[i],0,30};
 result[13]={sizeof(MlpParameterDescriptor),14,MLP_PARAMETER_AUTOMATABLE|MLP_PARAMETER_INTEGER,0,"Bypass","",0,1,0,0,1,20};
 return result;
}();
constexpr auto bytes=4u+4u*uint32_t(parameters.size());
constexpr MlpModuleDescriptor descriptor{sizeof(descriptor),MLP_ABI_MAJOR,MLP_ABI_MINOR,1,ml::stereoTypeHi,ml::stereoTypeLo,
 MLP_CAP_STEREO|MLP_CAP_STATE|MLP_CAP_CONCURRENT_STATE_READ,2,2,uint32_t(parameters.size()),0,
 "Stereo Architect","Seven-band Mid/Side width control","Stereo",parameters.data()};
static_assert(std::atomic<float>::is_always_lock_free&&std::atomic<uint64_t>::is_always_lock_free);
}
struct MlpInstance {
 std::array<std::atomic<float>,14> target{};
 std::array<float,7> widths{};
 std::array<float,6> splits{},filterState{};
 float rate=48000,bypass=0,leftPower=0,rightPower=0,crossPower=0;
 std::atomic<uint64_t> display{0},energyDisplay{0};
 uint32_t frames=0;
 MlpInstance(){for(size_t i=0;i<target.size();++i)target[i].store(float(parameters[i].default_value));}
};
namespace {
const MlpModuleDescriptor* MLP_CALL describe(){return &descriptor;}
MlpResult MLP_CALL create(MlpInstance** out){
 if(!out)return MLP_ERROR_ARGUMENT;
 *out=new(std::nothrow) MlpInstance;
 return *out?MLP_OK:MLP_ERROR_INTERNAL;
}
void MLP_CALL destroy(MlpInstance* p){delete p;}
void MLP_CALL reset(MlpInstance* p){
 if(!p)return;
 for(size_t i=0;i<7;++i)p->widths[i]=p->target[i].load(std::memory_order_relaxed)*.01f;
 for(size_t i=0;i<6;++i)p->splits[i]=p->target[7+i].load(std::memory_order_relaxed);
 p->splits=ml::orderedStereoSplits(p->splits,std::min(20000.f,p->rate*.45f));
 p->bypass=p->target[13].load(std::memory_order_relaxed);
 p->filterState={};p->leftPower=p->rightPower=p->crossPower=0;
 p->display.store(0,std::memory_order_relaxed);p->energyDisplay.store(0,std::memory_order_relaxed);
}
MlpResult MLP_CALL prepare(MlpInstance* p,double rate,uint32_t frames,uint32_t channels){
 if(!p||!std::isfinite(rate)||rate<8000||rate>768000||!frames||channels!=2)return MLP_ERROR_ARGUMENT;
 p->rate=float(rate);p->frames=frames;reset(p);return MLP_OK;
}
MlpResult MLP_CALL set(MlpInstance* p,uint32_t id,double value){
 if(!p||!id||id>parameters.size()||!std::isfinite(value))return MLP_ERROR_ARGUMENT;
 const auto& descriptor=parameters[id-1];
 p->target[id-1].store(float(std::clamp(id==14?std::round(value):value,descriptor.minimum,descriptor.maximum)),std::memory_order_relaxed);
 return MLP_OK;
}
MlpResult MLP_CALL get(const MlpInstance* p,uint32_t id,double* value){
 if(!p||!id||id>parameters.size()||!value)return MLP_ERROR_ARGUMENT;
 *value=p->target[id-1].load(std::memory_order_relaxed);return MLP_OK;
}
float smooth(float previous,float target,float amount) noexcept {
 const auto value=std::lerp(previous,target,amount);
 return value==previous||std::abs(target-value)<1e-6f*std::max(1.f,std::abs(target))?target:value;
}
MlpResult MLP_CALL process(MlpInstance* p,const MlpProcessContext* context){
 if(!p||!context||context->struct_size<sizeof(*context)||context->channels!=2||
    context->frames>p->frames||!context->audio||!context->audio[0]||!context->audio[1]||
    context->audio[0]==context->audio[1])return MLP_ERROR_ARGUMENT;
 const float widthStep=float(-std::expm1(-50./p->rate)),splitStep=float(-std::expm1(-33.3333/p->rate));
 std::array<float,7> widths{};
 std::array<float,6> splits{},coefficients{};
 for(size_t i=0;i<widths.size();++i)widths[i]=p->target[i].load(std::memory_order_relaxed)*.01f;
 for(size_t i=0;i<splits.size();++i)splits[i]=p->target[7+i].load(std::memory_order_relaxed);
 splits=ml::orderedStereoSplits(splits,std::min(20000.f,p->rate*.45f));
 for(size_t i=0;i<splits.size();++i)coefficients[i]=ml::CompressorCrossover::coefficient(p->splits[i],p->rate);
 const float bypass=p->target[13].load(std::memory_order_relaxed);
 float midPower=0,sidePower=0;
 const float meterStep=float(-std::expm1(-8./p->rate));
 for(uint32_t frame=0;frame<context->frames;++frame){
  const float left=std::clamp(ml::flushCompressorSample(context->audio[0][frame]),-64.f,64.f);
  const float right=std::clamp(ml::flushCompressorSample(context->audio[1][frame]),-64.f,64.f);
  const float side=(left-right)*.5f;
  float correction=0,lower=0;
  for(size_t i=0;i<widths.size();++i){
   p->widths[i]=smooth(p->widths[i],widths[i],widthStep);
   float upper=side;
   if(i<splits.size()){
    const float next=smooth(p->splits[i],splits[i],splitStep);
    if(next!=p->splits[i]){
     p->splits[i]=next;coefficients[i]=ml::CompressorCrossover::coefficient(next,p->rate);
    }
    auto& state=p->filterState[i];
    const float change=(side-state)*coefficients[i];upper=change+state;
    state=ml::flushCompressorSample(upper+change);
   }
   correction+=(upper-lower)*(p->widths[i]-1);lower=upper;
  }
  p->bypass=smooth(p->bypass,bypass,widthStep);
  correction*=1-p->bypass;
  const float outL=left+correction,outR=right-correction;
  p->leftPower=std::lerp(p->leftPower,outL*outL,meterStep);
  p->rightPower=std::lerp(p->rightPower,outR*outR,meterStep);
  p->crossPower=std::lerp(p->crossPower,outL*outR,meterStep);
  const float mid=(outL+outR)*.5f,wetSide=(outL-outR)*.5f;
  midPower+=mid*mid;sidePower+=wetSide*wetSide;
  context->audio[0][frame]=outL;context->audio[1][frame]=outR;
 }
 p->leftPower=ml::flushCompressorSample(p->leftPower);p->rightPower=ml::flushCompressorSample(p->rightPower);
 p->crossPower=ml::flushCompressorSample(p->crossPower);
 const float denominator=std::sqrt(p->leftPower*p->rightPower);
 const float correlation=denominator>1e-14f?std::clamp(p->crossPower/denominator,-1.f,1.f):0;
 p->display.store(std::bit_cast<uint32_t>(correlation),std::memory_order_relaxed);
 if(context->frames){
  const float divisor=float(context->frames);
  p->energyDisplay.store((uint64_t(std::bit_cast<uint32_t>(std::sqrt(midPower/divisor)))<<32)|
                          std::bit_cast<uint32_t>(std::sqrt(sidePower/divisor)),std::memory_order_relaxed);
 }
 return MLP_OK;
}
uint32_t MLP_CALL latency(const MlpInstance*){return 0;}
MlpResult MLP_CALL stateSize(const MlpInstance* p,uint32_t* size){
 if(!p||!size)return MLP_ERROR_ARGUMENT;
 *size=bytes;return MLP_OK;
}
MlpResult MLP_CALL save(const MlpInstance* p,void* data,uint32_t capacity,uint32_t* written){
 if(!p||!written)return MLP_ERROR_ARGUMENT;
 *written=bytes;
 if(!data||capacity<bytes)return MLP_ERROR_CAPACITY;
 auto* out=static_cast<uint8_t*>(data);
 auto write=[&](size_t pos,uint32_t value){for(unsigned i=0;i<4;++i)out[pos+i]=uint8_t(value>>(i*8));};
 write(0,0x31414c4d);
 for(size_t i=0;i<parameters.size();++i)write(4+4*i,std::bit_cast<uint32_t>(p->target[i].load(std::memory_order_relaxed)));
 return MLP_OK;
}
MlpResult MLP_CALL load(MlpInstance* p,const void* data,uint32_t size){
 if(!p||!data||size!=bytes)return MLP_ERROR_ARGUMENT;
 const auto* in=static_cast<const uint8_t*>(data);
 auto read=[&](size_t pos){uint32_t value=0;for(unsigned i=0;i<4;++i)value|=uint32_t(in[pos+i])<<(i*8);return value;};
 if(read(0)!=0x31414c4d)return MLP_ERROR_STATE;
 std::array<float,parameters.size()> values{};
 for(size_t i=0;i<values.size();++i){
  values[i]=std::bit_cast<float>(read(4+4*i));
  if(!std::isfinite(values[i])||values[i]<float(parameters[i].minimum)||values[i]>float(parameters[i].maximum))return MLP_ERROR_STATE;
 }
 for(uint32_t i=0;i<values.size();++i)set(p,i+1,values[i]);
 return MLP_OK;
}
}
extern "C" MLP_EXPORT uint64_t MLP_CALL masterlabpro_read_dynamics_v1(const MlpInstance* p,uint32_t field){
 if(!p)return 0;
 return (field==0?p->display:p->energyDisplay).load(std::memory_order_relaxed);
}
extern "C" MLP_EXPORT MlpResult MLP_CALL masterlabpro_get_module_api(uint32_t major,uint32_t,MlpModuleApi* api){
 if(!api||api->struct_size<sizeof(*api))return MLP_ERROR_ARGUMENT;
 if(major!=MLP_ABI_MAJOR)return MLP_ERROR_ABI;
 *api={sizeof(*api),MLP_ABI_MAJOR,MLP_ABI_MINOR,0,describe,create,destroy,prepare,reset,process,set,get,latency,stateSize,save,load};
 return MLP_OK;
}
