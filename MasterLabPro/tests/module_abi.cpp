#include "ModuleAbiValidation.h"
#include <cassert>

namespace {
MlpModuleDescriptor descriptor{sizeof(MlpModuleDescriptor),MLP_ABI_MAJOR,MLP_ABI_MINOR,1,1,2,
 MLP_CAP_STEREO,1,2,0,0,"Test","Test module","Test",nullptr};
const MlpModuleDescriptor* getDescriptor(){return &descriptor;}
MlpResult create(MlpInstance**){return MLP_OK;} void destroy(MlpInstance*){}
MlpResult prepare(MlpInstance*,double,uint32_t,uint32_t){return MLP_OK;} void reset(MlpInstance*){}
MlpResult process(MlpInstance*,const MlpProcessContext*){return MLP_OK;}
MlpResult set(MlpInstance*,uint32_t,double){return MLP_OK;} MlpResult get(const MlpInstance*,uint32_t,double*){return MLP_OK;}
uint32_t latency(const MlpInstance*){return 0;} MlpResult size(const MlpInstance*,uint32_t*){return MLP_OK;}
MlpResult save(const MlpInstance*,void*,uint32_t,uint32_t*){return MLP_OK;} MlpResult load(MlpInstance*,const void*,uint32_t){return MLP_OK;}
MlpModuleApi valid(){return {sizeof(MlpModuleApi),MLP_ABI_MAJOR,MLP_ABI_MINOR,0,getDescriptor,create,destroy,prepare,reset,process,set,get,latency,size,save,load};}
}
int main(){
 auto api=valid();assert(ml::validModuleApi(api));
 api.abi_major++;assert(!ml::validModuleApi(api));api=valid();api.process=nullptr;assert(!ml::validModuleApi(api));
 api=valid();descriptor.type_id_hi=descriptor.type_id_lo=0;assert(!ml::validModuleApi(api));
 descriptor.type_id_hi=1;descriptor.type_id_lo=2;descriptor.maximum_channels=0;assert(!ml::validModuleApi(api));
 descriptor.maximum_channels=2;assert(ml::validModuleApi(api));
}
