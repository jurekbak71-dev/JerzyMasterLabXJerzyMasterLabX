#include "ModuleLoader.h"
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>
#ifndef TEST_MODULE_DIRECTORY
#error TEST_MODULE_DIRECTORY is required
#endif
#ifndef TEST_SHARED_PREFIX
#define TEST_SHARED_PREFIX "lib"
#endif
#ifndef TEST_SHARED_SUFFIX
#define TEST_SHARED_SUFFIX ".so"
#endif
static std::filesystem::path module(const char* name){return std::filesystem::path(TEST_MODULE_DIRECTORY)/(std::string(TEST_SHARED_PREFIX)+name+TEST_SHARED_SUFFIX);}
int main(){const double rates[]{44100,48000,88200,96000,176400,192000};const uint32_t blocks[]{1,17,32,64,127,128,256,511,512,1024,2048};const char* names[]{"mlp_unity","mlp_gain","mlp_saturation","mlp_delay"};std::string error;
 for(const char* name:names){ml::ModuleLibrary library;assert(library.open(module(name),TEST_MODULE_DIRECTORY,error));auto* instance=library.create(error);assert(instance);const auto* api=library.api();
  for(double rate:rates)for(uint32_t block:blocks){assert(api->prepare(instance,rate,block,2)==MLP_OK);std::vector<float> l(block,.125f),r(block,-.125f);float* channels[]{l.data(),r.data()};MlpProcessContext c{sizeof(c),block,2,0,channels};assert(api->process(instance,&c)==MLP_OK);for(float x:l)assert(std::isfinite(x));for(float x:r)assert(std::isfinite(x));}
  uint32_t bytes=0,written=0;assert(api->state_size(instance,&bytes)==MLP_OK);std::vector<unsigned char> state(bytes);assert(api->save_state(instance,state.data(),bytes,&written)==MLP_OK&&written==bytes);assert(api->load_state(instance,state.data(),bytes)==MLP_OK);
  library.destroy(instance);assert(library.close());
 }
 ml::ModuleLibrary missing,incompatible;assert(!missing.open(module("mlp_missing_export"),TEST_MODULE_DIRECTORY,error));assert(!incompatible.open(module("mlp_incompatible"),TEST_MODULE_DIRECTORY,error));
}
