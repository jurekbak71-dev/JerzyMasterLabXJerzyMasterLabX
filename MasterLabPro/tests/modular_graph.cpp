#include "ModularGraph.h"
#include "ModuleLoader.h"
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
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
int main(){ml::StereoDelayAligner aligner;assert(aligner.setDelay(3));float al=0,ar=0;for(int i=0;i<8;++i)aligner.process(1,2,al,ar);assert(al==1&&ar==2);aligner.reset();for(int i=0;i<3;++i){aligner.process(3,4,al,ar);assert(al==0&&ar==0);}aligner.process(3,4,al,ar);assert(al==3&&ar==4);std::string error;const std::filesystem::path dir=TEST_MODULE_DIRECTORY;ml::ModuleLibrary gain,sat,delay;assert(gain.open(module("mlp_gain"),dir,error));assert(sat.open(module("mlp_saturation"),dir,error));assert(delay.open(module("mlp_delay"),dir,error));auto* g=gain.create(error);auto* s=sat.create(error);auto* d=delay.create(error);assert(g&&s&&d);for(auto pair:{std::pair{&gain,g},std::pair{&sat,s},std::pair{&delay,d}})assert(pair.first->api()->prepare(pair.second,48000,256,2)==MLP_OK);assert(gain.api()->set_parameter(g,1,12)==MLP_OK);assert(sat.api()->set_parameter(s,1,12)==MLP_OK);assert(delay.api()->set_parameter(d,1,3)==MLP_OK);
 ml::ModularGraph graph;graph.prepare(256);ml::GraphSnapshot a;a.revision=1;a.count=2;a.nodes[0]={1,gain.api(),g,false};a.nodes[1]={2,sat.api(),s,false};assert(graph.submit(a));std::array<float,256> l{},r{},dl{},dr{};l.fill(.4f);r=l;assert(graph.process(l.data(),r.data(),256));l.fill(.4f);r=l;assert(graph.process(l.data(),r.data(),256));assert(graph.revision()==1);const float gainThenSat=l.back();
 ml::GraphSnapshot b=a;b.revision=2;std::swap(b.nodes[0],b.nodes[1]);assert(graph.submit(b));l.fill(.4f);r=l;assert(graph.process(l.data(),r.data(),256));l.fill(.4f);r=l;assert(graph.process(l.data(),r.data(),256));assert(graph.revision()==2);const float satThenGain=l.back();assert(std::abs(gainThenSat-satThenGain)>.05f);
 ml::GraphSnapshot c;c.revision=3;c.count=1;c.nodes[0]={3,delay.api(),d,false};assert(graph.submit(c));l.fill(0);r.fill(0);assert(graph.process(l.data(),r.data(),256,dl.data(),dr.data()));l.fill(0);r.fill(0);assert(graph.process(l.data(),r.data(),256,dl.data(),dr.data()));assert(graph.diagnostics().latency==3);
 assert(delay.api()->set_parameter(d,1,7)==MLP_OK);l.fill(0);r.fill(0);assert(graph.process(l.data(),r.data(),256,dl.data(),dr.data()));assert(graph.process(l.data(),r.data(),256,dl.data(),dr.data()));assert(graph.diagnostics().latency==7&&graph.diagnostics().latencyChanges>=1);
 // Once the transition is complete, a pure delay has an aligned audio delta near zero.
 delay.api()->reset(d);l.fill(0);r.fill(0);l[0]=r[0]=1;assert(graph.process(l.data(),r.data(),256,dl.data(),dr.data()));for(float x:dl)assert(std::abs(x)<1e-6f);
 ml::GraphSnapshot retired;assert(graph.pollRetired(retired));std::cout<<"{\"initial_latency_samples\":3,\"changed_latency_samples\":7,\"aligned_delta_tolerance\":1e-6,\"latency_changes\":"<<graph.diagnostics().latencyChanges<<"}\n";gain.destroy(g);sat.destroy(s);delay.destroy(d);}
