#include "ModuleLoader.h"
#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace ml {
bool ModuleLibrary::open(const std::filesystem::path& file,const std::filesystem::path& allowed,std::string& error) noexcept {
 if(handle){error="library is already open";return false;}
 try{
  const auto root=std::filesystem::weakly_canonical(allowed);
  const auto path=std::filesystem::canonical(file);
  if(path.parent_path()!=root){error="module is outside the allowed directory";return false;}
#if defined(_WIN32)
  handle=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
  auto entry=handle?reinterpret_cast<MlpGetModuleApiFn>(GetProcAddress(static_cast<HMODULE>(handle),"masterlabpro_get_module_api")):nullptr;
#else
  handle=dlopen(path.c_str(),RTLD_NOW|RTLD_LOCAL);
  auto entry=handle?reinterpret_cast<MlpGetModuleApiFn>(dlsym(handle,"masterlabpro_get_module_api")):nullptr;
#endif
  if(!handle){error="system loader rejected module";return false;}
  if(!entry){error="required module export is missing";close();return false;}
  functions={};functions.struct_size=sizeof(functions);
  if(entry(MLP_ABI_MAJOR,MLP_ABI_MINOR,&functions)!=MLP_OK||!validModuleApi(functions,&error)){close();return false;}
#if defined(_WIN32)
  readReduction=reinterpret_cast<MlpReadGainReductionV1>(GetProcAddress(static_cast<HMODULE>(handle),"masterlabpro_read_gain_reduction_v1"));
#else
  readReduction=reinterpret_cast<MlpReadGainReductionV1>(dlsym(handle,"masterlabpro_read_gain_reduction_v1"));
#endif
  return true;
 }catch(...){error="module path validation failed";close();return false;}
}
bool ModuleLibrary::close() noexcept {
 if(!handle)return true;
 if(instances)return false;
#if defined(_WIN32)
 FreeLibrary(static_cast<HMODULE>(handle));
#else
 dlclose(handle);
#endif
 handle=nullptr;functions={};readReduction=nullptr;return true;
}
MlpInstance* ModuleLibrary::create(std::string& error) noexcept {MlpInstance* p=nullptr;if(!handle||functions.create(&p)!=MLP_OK||!p){error="module instance creation failed";return nullptr;}++instances;return p;}
void ModuleLibrary::destroy(MlpInstance* p) noexcept {if(handle&&p&&instances){functions.destroy(p);--instances;}}
}
