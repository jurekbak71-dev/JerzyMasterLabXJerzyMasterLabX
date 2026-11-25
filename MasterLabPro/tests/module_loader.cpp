#include "ModuleLoader.h"
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <string>

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
int main(){
 const std::filesystem::path directory=TEST_MODULE_DIRECTORY;std::string error;
 ml::ModuleLibrary gain;assert(gain.open(module("mlp_gain"),directory,error));
 auto* a=gain.create(error);auto* b=gain.create(error);assert(a&&b&&gain.liveInstances()==2);assert(!gain.close());
 assert(gain.api()->prepare(a,48000,64,2)==MLP_OK);assert(gain.api()->prepare(b,48000,64,2)==MLP_OK);
 assert(gain.api()->set_parameter(a,1,6)==MLP_OK);double av=0,bv=1;assert(gain.api()->get_parameter(a,1,&av)==MLP_OK);assert(gain.api()->get_parameter(b,1,&bv)==MLP_OK);assert(av==6&&bv==0);
 std::array<float,4> l{.1f,.2f,.3f,.4f},r=l;float* channels[]{l.data(),r.data()};MlpProcessContext c{sizeof(c),4,2,0,channels};assert(gain.api()->process(a,&c)==MLP_OK);assert(l[0]>.19f);
 gain.destroy(a);gain.destroy(b);assert(gain.close());
 ml::ModuleLibrary saturation;assert(saturation.open(module("mlp_saturation"),directory,error));auto* s=saturation.create(error);assert(s);assert(saturation.api()->prepare(s,48000,64,2)==MLP_OK);assert(saturation.api()->set_parameter(s,1,12)==MLP_OK);
 l={.1f,.2f,.3f,.4f};r=l;assert(saturation.api()->process(s,&c)==MLP_OK);assert(std::abs(l[1]/.2f-l[3]/.4f)>.01f);saturation.destroy(s);assert(saturation.close());
 ml::ModuleLibrary delay;assert(delay.open(module("mlp_delay"),directory,error));auto* d=delay.create(error);assert(d);assert(delay.api()->prepare(d,48000,64,2)==MLP_OK);assert(delay.api()->set_parameter(d,1,3)==MLP_OK);assert(delay.api()->get_latency_samples(d)==3);
 l={1,0,0,0};r=l;assert(delay.api()->process(d,&c)==MLP_OK);assert(l[0]==0&&l[1]==0&&l[2]==0&&l[3]==1);delay.destroy(d);assert(delay.close());
}
