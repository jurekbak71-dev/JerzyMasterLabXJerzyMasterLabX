#include "masterlabpro_module_abi.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <new>

#ifndef MLP_TECH_KIND
#error MLP_TECH_KIND must be defined
#endif

namespace {
constexpr uint32_t kKind=MLP_TECH_KIND;
constexpr uint32_t kStateBytes=16;
constexpr uint32_t kMaximumDelay=16384;

#if MLP_TECH_KIND == 1
constexpr const char* kName="Unity"; constexpr uint64_t kTypeLo=0x756e697479000001ull; constexpr uint64_t kCaps=MLP_CAP_STEREO|MLP_CAP_STATE;
constexpr uint32_t kParameterCount=0;
#elif MLP_TECH_KIND == 2
constexpr const char* kName="Gain"; constexpr uint64_t kTypeLo=0x6761696e00000002ull; constexpr uint64_t kCaps=MLP_CAP_STEREO|MLP_CAP_STATE;
constexpr uint32_t kParameterCount=1;
#elif MLP_TECH_KIND == 3
constexpr const char* kName="Saturation Test"; constexpr uint64_t kTypeLo=0x7361747572610003ull; constexpr uint64_t kCaps=MLP_CAP_STEREO|MLP_CAP_STATE|MLP_CAP_NONLINEAR;
constexpr uint32_t kParameterCount=1;
#elif MLP_TECH_KIND == 4
constexpr const char* kName="Delay / Latency Test"; constexpr uint64_t kTypeLo=0x64656c6179000004ull; constexpr uint64_t kCaps=MLP_CAP_STEREO|MLP_CAP_STATE|MLP_CAP_VARIABLE_LATENCY;
constexpr uint32_t kParameterCount=1;
#else
#error unsupported MLP_TECH_KIND
#endif

constexpr MlpParameterDescriptor kParameter=[]{
 MlpParameterDescriptor p{sizeof(MlpParameterDescriptor),1,MLP_PARAMETER_AUTOMATABLE,0,"","",0,1,0,0,0,5};
#if MLP_TECH_KIND == 2
 p.name="Gain";p.unit="dB";p.minimum=-24;p.maximum=24;p.default_value=p.neutral_value=0;p.step=.01;
#elif MLP_TECH_KIND == 3
 p.name="Drive";p.unit="dB";p.minimum=0;p.maximum=24;p.default_value=p.neutral_value=0;p.step=.01;
#elif MLP_TECH_KIND == 4
 p.name="Latency";p.unit="samples";p.minimum=0;p.maximum=kMaximumDelay;p.default_value=p.neutral_value=0;p.step=1;p.flags|=MLP_PARAMETER_INTEGER|MLP_PARAMETER_AFFECTS_LATENCY;
#endif
 return p;
}();
constexpr MlpModuleDescriptor kDescriptor{sizeof(MlpModuleDescriptor),MLP_ABI_MAJOR,MLP_ABI_MINOR,1,
 0x4d4c505445434800ull,kTypeLo,kCaps,1,2,kParameterCount,0,kName,
 "MasterLabPro SDK validation processor; not a production mastering effect","Technical",
 kParameterCount?&kParameter:nullptr};

void put32(unsigned char* p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=static_cast<unsigned char>(v>>(i*8));}
uint32_t get32(const unsigned char* p){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(p[i])<<(i*8);return v;}
void put64(unsigned char* p,uint64_t v){for(unsigned i=0;i<8;++i)p[i]=static_cast<unsigned char>(v>>(i*8));}
uint64_t get64(const unsigned char* p){uint64_t v=0;for(unsigned i=0;i<8;++i)v|=uint64_t(p[i])<<(i*8);return v;}
}

struct MlpInstance {
 std::atomic<double> value{0};
 std::atomic<uint32_t> latency{0};
 uint32_t position=0,channels=0;
 std::array<std::array<float,kMaximumDelay+1>,2> delay{};
};

namespace {
const MlpModuleDescriptor* MLP_CALL descriptor(){return &kDescriptor;}
MlpResult MLP_CALL create(MlpInstance** out){if(!out)return MLP_ERROR_ARGUMENT;*out=new(std::nothrow) MlpInstance;return *out?MLP_OK:MLP_ERROR_INTERNAL;}
void MLP_CALL destroy(MlpInstance* p){delete p;}
MlpResult MLP_CALL prepare(MlpInstance* p,double rate,uint32_t frames,uint32_t channels){
 if(!p||!std::isfinite(rate)||rate<8000||rate>768000||frames==0||channels<1||channels>2)return MLP_ERROR_ARGUMENT;
 p->channels=channels;p->position=0;for(auto& c:p->delay)c.fill(0);return MLP_OK;
}
void MLP_CALL reset(MlpInstance* p){if(p){p->position=0;for(auto& c:p->delay)c.fill(0);}}
MlpResult MLP_CALL process(MlpInstance* p,const MlpProcessContext* c){
 if(!p||!c||c->struct_size<sizeof(MlpProcessContext)||!c->audio||c->channels<1||c->channels>p->channels)return MLP_ERROR_ARGUMENT;
#if MLP_TECH_KIND == 2 || MLP_TECH_KIND == 3
 const double parameter=p->value.load(std::memory_order_relaxed);
#endif
#if MLP_TECH_KIND == 4
 const uint32_t blockLatency=p->latency.load(std::memory_order_acquire);
#endif
 for(uint32_t i=0;i<c->frames;++i){
  for(uint32_t ch=0;ch<c->channels;++ch){float* const a=c->audio[ch];if(!a)continue;float x=std::isfinite(a[i])?a[i]:0.f;
#if MLP_TECH_KIND == 2
   x*=static_cast<float>(std::pow(10.,parameter/20.));
#elif MLP_TECH_KIND == 3
   {const float drive=static_cast<float>(std::pow(10.,parameter/20.));const float norm=std::tanh(drive);x=norm>0?std::tanh(x*drive)/norm:x;}
#elif MLP_TECH_KIND == 4
   {auto& line=p->delay[ch];const uint32_t read=(p->position+kMaximumDelay+1-blockLatency)%(kMaximumDelay+1);const float y=line[read];line[p->position]=x;x=blockLatency?y:x;}
#endif
   a[i]=std::isfinite(x)?x:0.f;
  }
#if MLP_TECH_KIND == 4
  p->position=(p->position+1)%(kMaximumDelay+1);
#endif
 }
 return MLP_OK;
}
MlpResult MLP_CALL setParameter(MlpInstance* p,uint32_t id,double value){
 if(!p||id!=1||!std::isfinite(value)||kParameterCount==0)return MLP_ERROR_ARGUMENT;
 value=std::clamp(value,kParameter.minimum,kParameter.maximum);
#if MLP_TECH_KIND == 4
 p->latency.store(static_cast<uint32_t>(std::llround(value)),std::memory_order_release);
 p->value.store(double(p->latency.load(std::memory_order_relaxed)),std::memory_order_relaxed);
#else
 p->value.store(value,std::memory_order_relaxed);
#endif
 return MLP_OK;
}
MlpResult MLP_CALL getParameter(const MlpInstance* p,uint32_t id,double* value){if(!p||!value||id!=1||kParameterCount==0)return MLP_ERROR_ARGUMENT;*value=p->value.load(std::memory_order_relaxed);return MLP_OK;}
uint32_t MLP_CALL latency(const MlpInstance* p){return p?p->latency.load(std::memory_order_acquire):0;}
MlpResult MLP_CALL stateSize(const MlpInstance* p,uint32_t* bytes){if(!p||!bytes)return MLP_ERROR_ARGUMENT;*bytes=kStateBytes;return MLP_OK;}
MlpResult MLP_CALL save(const MlpInstance* p,void* destination,uint32_t capacity,uint32_t* written){
 if(!p||!written)return MLP_ERROR_ARGUMENT;
 *written=kStateBytes;
 if(!destination||capacity<kStateBytes)return MLP_ERROR_CAPACITY;
 auto* b=static_cast<unsigned char*>(destination);put32(b,0x31504c4d);put32(b+4,kKind);put64(b+8,std::bit_cast<uint64_t>(p->value.load(std::memory_order_relaxed)));return MLP_OK;
}
MlpResult MLP_CALL load(MlpInstance* p,const void* source,uint32_t bytes){
 if(!p||!source||bytes!=kStateBytes)return MLP_ERROR_ARGUMENT;
 const auto* b=static_cast<const unsigned char*>(source);
 if(get32(b)!=0x31504c4d||get32(b+4)!=kKind)return MLP_ERROR_STATE;
 const double v=std::bit_cast<double>(get64(b+8));
 if(kParameterCount==0){if(v!=0)return MLP_ERROR_STATE;return MLP_OK;}return setParameter(p,1,v);
}
}

extern "C" MLP_EXPORT MlpResult MLP_CALL masterlabpro_get_module_api(uint32_t major,uint32_t minor,MlpModuleApi* api){
 if(!api||api->struct_size<sizeof(MlpModuleApi))return MLP_ERROR_ARGUMENT;
 if(major!=MLP_ABI_MAJOR)return MLP_ERROR_ABI;
 (void)minor;
 *api={sizeof(MlpModuleApi),MLP_ABI_MAJOR,MLP_ABI_MINOR,0,descriptor,create,destroy,prepare,reset,process,setParameter,getParameter,latency,stateSize,save,load};
 return MLP_OK;
}
