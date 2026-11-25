#pragma once
#include "../sdk/masterlabpro_module_abi.h"
#include <cstring>
#include <string>

namespace ml {
inline bool validModuleApi(const MlpModuleApi& api, std::string* reason=nullptr) {
 auto fail=[&](const char* text){if(reason)*reason=text;return false;};
 if(api.struct_size<sizeof(MlpModuleApi))return fail("module API structure is truncated");
 if(api.abi_major!=MLP_ABI_MAJOR||api.abi_minor>MLP_ABI_MINOR)return fail("incompatible ABI version");
 if(!api.get_descriptor||!api.create||!api.destroy||!api.prepare||!api.reset||!api.process||
    !api.set_parameter||!api.get_parameter||!api.get_latency_samples||!api.state_size||
    !api.save_state||!api.load_state)return fail("required function is missing");
 const auto* d=api.get_descriptor();
 if(!d||d->struct_size<sizeof(MlpModuleDescriptor))return fail("descriptor is truncated");
 if(d->abi_major!=MLP_ABI_MAJOR||d->abi_minor>MLP_ABI_MINOR)return fail("descriptor ABI is incompatible");
 if((d->type_id_hi|d->type_id_lo)==0)return fail("module type identifier is zero");
 if(!d->name||!d->description||!d->category||std::strlen(d->name)==0)return fail("descriptor strings are invalid");
 if(d->minimum_channels==0||d->minimum_channels>d->maximum_channels||d->maximum_channels>2)return fail("channel declaration is invalid");
 if(d->parameter_count>MLP_MAX_PARAMETERS||(d->parameter_count&&!d->parameters))return fail("parameter table is invalid");
 for(uint32_t i=0;i<d->parameter_count;++i){const auto& p=d->parameters[i];
  if(p.struct_size<sizeof(MlpParameterDescriptor)||!p.name||p.minimum>p.maximum||
     p.default_value<p.minimum||p.default_value>p.maximum||p.neutral_value<p.minimum||p.neutral_value>p.maximum)
   return fail("parameter descriptor is invalid");
  for(uint32_t j=0;j<i;++j)if(d->parameters[j].id==p.id)return fail("parameter identifier is duplicated");
 }
 return true;
}
}
