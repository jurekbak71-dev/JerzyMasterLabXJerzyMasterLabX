#include "ModularGraph.h"
#include "ModuleLoader.h"
#include <array>
#include <atomic>
#include <cassert>
#include <filesystem>
#include <string>
#include <thread>
#ifndef TEST_MODULE_DIRECTORY
#error TEST_MODULE_DIRECTORY is required
#endif
#ifndef TEST_SHARED_PREFIX
#define TEST_SHARED_PREFIX "lib"
#endif
#ifndef TEST_SHARED_SUFFIX
#define TEST_SHARED_SUFFIX ".so"
#endif
static std::filesystem::path module(const char* n){return std::filesystem::path(TEST_MODULE_DIRECTORY)/(std::string(TEST_SHARED_PREFIX)+n+TEST_SHARED_SUFFIX);}
int main(){std::string error;ml::ModuleLibrary gain,delay;assert(gain.open(module("mlp_gain"),TEST_MODULE_DIRECTORY,error));assert(delay.open(module("mlp_delay"),TEST_MODULE_DIRECTORY,error));auto* g=gain.create(error);auto* d=delay.create(error);assert(g&&d);assert(gain.api()->prepare(g,48000,64,2)==MLP_OK);assert(delay.api()->prepare(d,48000,64,2)==MLP_OK);ml::ModularGraph graph;graph.prepare(64);
 ml::GraphSnapshot a;a.count=2;a.nodes[0]={1,gain.api(),g,false};a.nodes[1]={2,delay.api(),d,false};ml::GraphSnapshot b=a;std::swap(b.nodes[0],b.nodes[1]);
 unsigned accepted=0;for(uint64_t i=1;i<=20;++i){auto next=(i&1)?a:b;next.revision=i;accepted+=graph.submit(next)?1u:0u;}assert(accepted>0&&accepted<20);
 std::atomic<bool> stop{false},failed{false},started{false};std::thread audio([&]{std::array<float,64> l{},r{};while(!stop.load(std::memory_order_acquire)){l.fill(.1f);r.fill(-.1f);if(!graph.process(l.data(),r.data(),64))failed.store(true,std::memory_order_release);started.store(true,std::memory_order_release);}});
 for(int i=0;i<10000&&!started.load(std::memory_order_acquire);++i)std::this_thread::yield();
 assert(started.load(std::memory_order_acquire));
 for(uint32_t i=0;i<5000;++i){gain.api()->set_parameter(g,1,double(int(i%49)-24));delay.api()->set_parameter(d,1,double(i%257));ml::GraphSnapshot retired;while(graph.pollRetired(retired)){}if((i%16)==0){auto next=(i&16)?a:b;next.revision=100+i;graph.submit(next);}}
 stop.store(true,std::memory_order_release);audio.join();assert(!failed.load());assert(graph.diagnostics().blocks>0);assert(graph.diagnostics().queueOverloads>0);
 std::array<float,64> l{},r{};for(int i=0;i<32;++i){graph.process(l.data(),r.data(),64);ml::GraphSnapshot retired;while(graph.pollRetired(retired)){}}
 gain.destroy(g);delay.destroy(d);assert(gain.close()&&delay.close());
}
