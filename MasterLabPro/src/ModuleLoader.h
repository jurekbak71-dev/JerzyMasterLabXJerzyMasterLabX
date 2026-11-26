#pragma once
#include "ModuleAbiValidation.h"
#include "../sdk/masterlabpro_input_compressor.h"
#include <filesystem>
#include <string>

namespace ml {
class ModuleLibrary {
public:
 ModuleLibrary()=default;~ModuleLibrary(){close();}
 ModuleLibrary(const ModuleLibrary&)=delete;ModuleLibrary& operator=(const ModuleLibrary&)=delete;
 bool open(const std::filesystem::path& file,const std::filesystem::path& allowedDirectory,std::string& error) noexcept;
 bool close() noexcept;
 MlpInstance* create(std::string& error) noexcept;
 void destroy(MlpInstance* instance) noexcept;
 const MlpModuleApi* api()const noexcept{return handle?&functions:nullptr;}
 unsigned liveInstances()const noexcept{return instances;}
 MlpReadGainReductionV1 gainReduction()const noexcept{return readReduction;}
 using ReadDynamics=uint64_t (MLP_CALL *)(const MlpInstance*,uint32_t);
 ReadDynamics dynamics()const noexcept{return readDynamics;}
private:
 MlpReadGainReductionV1 readReduction=nullptr;
 ReadDynamics readDynamics=nullptr;
 void* handle=nullptr;MlpModuleApi functions{};unsigned instances=0;
};
}
