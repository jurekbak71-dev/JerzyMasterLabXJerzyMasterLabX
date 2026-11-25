#include "ModuleRackHost.h"
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <memory>
#ifndef TEST_MODULE_DIRECTORY
#error TEST_MODULE_DIRECTORY is required
#endif
int main(){auto host=std::make_unique<ml::ModuleRackHost>();assert(host->scan(TEST_MODULE_DIRECTORY)==4);host->prepare(48000,257);ml::RackOrder order;for(std::size_t i=0;i<4;++i){assert(order.insert(i,i+1));assert(host->add(i,i+1));const auto* d=host->descriptor(i+1);assert(d&&d->name);}assert(host->setParameter(2,1,6));assert(host->submit(order));std::array<float,257> l{},r{};for(int block=0;block<40;++block){for(std::size_t i=0;i<l.size();++i){l[i]=.1f*std::sin(float(i+block*l.size())*.1f);r[i]=l[i];}assert(host->process(l.data(),r.data(),l.size()));}assert(host->diagnostics().blocks==40);ml::ModuleRackMeters meters;assert(host->pollMeters(meters));ml::AnalysisFrame frame;while(host->pollAnalysis(frame)){}host->setAnalysis(3,true,false);for(int block=0;block<40;++block){l.fill(.2f);r=l;assert(host->process(l.data(),r.data(),l.size()));}bool moduleFrame=false;while(host->pollAnalysis(frame))moduleFrame|=frame.sourceId==3;assert(moduleFrame);auto state=host->saveState(order);std::vector<uint8_t> encoded;assert(ml::RackStateCodec::encode(state,encoded));ml::RackState decoded;assert(ml::RackStateCodec::decode(encoded.data(),encoded.size(),decoded));ml::RackOrder restored;assert(host->restoreStateWhenStopped(decoded,restored));double value=0;assert(host->getParameter(2,1,value)&&value==6);auto missing=decoded;missing.modules[0].typeHi^=1;assert(!host->restoreStateWhenStopped(missing,restored));assert(host->unavailable().size()==1);auto preserved=host->saveState(restored);assert(preserved.modules.size()==missing.modules.size());host->shutdownWhenStopped();}
