#include "masterlabpro_module_abi.h"
#if INVALID_MODULE_KIND == 1
extern "C" MLP_EXPORT int not_the_required_export(){return 0;}
#else
extern "C" MLP_EXPORT MlpResult MLP_CALL masterlabpro_get_module_api(uint32_t,uint32_t,MlpModuleApi* api){
 if(!api)return MLP_ERROR_ARGUMENT;
 api->abi_major=MLP_ABI_MAJOR+1;api->abi_minor=0;return MLP_OK;
}
#endif
