#pragma once
#include "ModuleAbiValidation.h"
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
private:
 void* handle=nullptr;MlpModuleApi functions{};unsigned instances=0;
};
}
