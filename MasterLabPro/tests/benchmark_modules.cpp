#include "ModularGraph.h"
#include "ModuleLoader.h"
#include "ModuleRackHost.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#endif
#ifndef TEST_MODULE_DIRECTORY
#error TEST_MODULE_DIRECTORY is required
#endif
#ifndef TEST_SHARED_PREFIX
#define TEST_SHARED_PREFIX "lib"
#endif
#ifndef TEST_SHARED_SUFFIX
#define TEST_SHARED_SUFFIX ".so"
#endif
namespace {
using Clock=std::chrono::steady_clock;
struct Stats{double median=0,p95=0,p99=0,maximum=0;};
Stats stats(std::vector<double> values){if(values.empty())return {};std::sort(values.begin(),values.end());return {values[values.size()/2],values[values.size()*95/100],values[values.size()*99/100],values.back()};}
Stats run(ml::ModularGraph& graph){std::array<float,256> l{},r{};l.fill(.1f);r=l;std::vector<double> values;values.reserve(2000);for(int i=0;i<2000;++i){auto start=Clock::now();if(!graph.process(l.data(),r.data(),256))return {};values.push_back(std::chrono::duration<double,std::nano>(Clock::now()-start).count());}return stats(std::move(values));}
Stats run(ml::ModuleRackHost& host){std::array<float,256> l{},r{};l.fill(.1f);r=l;std::vector<double> values;values.reserve(2000);for(int i=0;i<2000;++i){auto start=Clock::now();if(!host.process(l.data(),r.data(),256))return {};values.push_back(std::chrono::duration<double,std::nano>(Clock::now()-start).count());}return stats(std::move(values));}
std::filesystem::path module(const char* n){return std::filesystem::path(TEST_MODULE_DIRECTORY)/(std::string(TEST_SHARED_PREFIX)+n+TEST_SHARED_SUFFIX);}
void print(const char* name,const Stats& s){std::cout<<'"'<<name<<"\":{\"median_ns\":"<<s.median<<",\"p95_ns\":"<<s.p95<<",\"p99_ns\":"<<s.p99<<",\"max_ns\":"<<s.maximum<<'}';}
std::uint64_t residentBytes(){
#if defined(_WIN32)
 PROCESS_MEMORY_COUNTERS info{};info.cb=sizeof(info);return GetProcessMemoryInfo(GetCurrentProcess(),&info,sizeof(info))?info.WorkingSetSize:0;
#elif defined(__linux__)
 std::ifstream stat("/proc/self/statm");std::uint64_t total=0,resident=0;stat>>total>>resident;return resident*static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
#else
 return 0;
#endif
}
}
int main(){std::string error;ml::ModuleLibrary unity,delay;if(!unity.open(module("mlp_unity"),TEST_MODULE_DIRECTORY,error)||!delay.open(module("mlp_delay"),TEST_MODULE_DIRECTORY,error))return 1;std::array<MlpInstance*,10> instances{};for(auto& p:instances){p=unity.create(error);if(!p||unity.api()->prepare(p,48000,256,2)!=MLP_OK)return 2;}auto* delayed=delay.create(error);if(!delayed||delay.api()->prepare(delayed,48000,256,2)!=MLP_OK||delay.api()->set_parameter(delayed,1,128)!=MLP_OK)return 3;
 ml::ModularGraph graph;graph.prepare(256);const auto empty=run(graph);ml::GraphSnapshot one;one.revision=1;one.count=1;one.nodes[0]={1,unity.api(),instances[0],false};graph.submit(one);run(graph);const auto single=run(graph);ml::GraphSnapshot ten;ten.revision=2;ten.count=10;for(uint32_t i=0;i<10;++i)ten.nodes[i]={i+1,unity.api(),instances[i],false};graph.submit(ten);run(graph);const auto full=run(graph);ml::GraphSnapshot pdc;pdc.revision=3;pdc.count=1;pdc.nodes[0]={11,delay.api(),delayed,false};graph.submit(pdc);run(graph);const auto delayedStats=run(graph);const auto pdcSamples=graph.diagnostics().latency;
 std::vector<double> switching;switching.reserve(200);std::array<float,256> switchL{},switchR{};for(uint64_t i=0;i<200;++i){auto next=(i&1)?ten:one;next.revision=100+i;const auto start=Clock::now();if(!graph.submit(next))return 7;if(!graph.process(switchL.data(),switchR.data(),256)||!graph.process(switchL.data(),switchR.data(),256))return 8;switching.push_back(std::chrono::duration<double,std::nano>(Clock::now()-start).count());ml::GraphSnapshot retired;while(graph.pollRetired(retired)){}}
 auto host=std::make_unique<ml::ModuleRackHost>();if(host->scan(TEST_MODULE_DIRECTORY)!=4)return 4;host->prepare(48000,256);ml::RackOrder order;for(uint64_t i=1;i<=10;++i){if(!order.insert(i-1,i)||!host->add(std::size_t((i-1)%4),i))return 5;}if(!host->submit(order))return 6;run(*host);const auto metered=run(*host);
 std::cout<<'{';print("empty_graph_256",empty);std::cout<<',';print("one_unity_256",single);std::cout<<',';print("ten_unity_256",full);std::cout<<',';print("delay_pdc_256",delayedStats);std::cout<<',';print("graph_switch_two_blocks",stats(std::move(switching)));std::cout<<',';print("ten_mixed_with_metering_analysis_capture_256",metered);std::cout<<",\"resident_process_bytes\":"<<residentBytes()<<",\"reported_pdc_samples\":"<<pdcSamples<<",\"memory_note\":\"process RSS, not per-module RAM\"}\n";
 host->shutdownWhenStopped();graph.resetWhenStopped();for(auto* p:instances)unity.destroy(p);delay.destroy(delayed);return 0;}
