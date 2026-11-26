#pragma once
#include "AnalysisCapture.h"
#include "MeasurementModel.h"
#include "ModularGraph.h"
#include "ModuleLoader.h"
#include "RackOrder.h"
#include "RackState.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace ml {
struct ModuleRackMeters{StereoLevels input,output,delta;};
class ModuleRackHost {
public:
 struct Type {std::unique_ptr<ModuleLibrary> library;const MlpModuleDescriptor* descriptor=nullptr;std::filesystem::path file;};
 struct Instance {uint64_t id=0;Type* type=nullptr;MlpInstance* handle=nullptr;bool bypass=false;bool removed=false;uint32_t lane=10;uint64_t lastPublished=0;};
 ~ModuleRackHost(){shutdownWhenStopped();}
 ModuleRackHost(const ModuleRackHost&)=delete;ModuleRackHost& operator=(const ModuleRackHost&)=delete;ModuleRackHost()=default;
 std::size_t scan(const std::filesystem::path& directory){
  shutdownWhenStopped();errors.clear();moduleDirectory=directory;
  try{if(!std::filesystem::is_directory(directory)){errors.push_back("module directory is missing");return 0;}std::vector<std::filesystem::path> files;
   for(const auto& entry:std::filesystem::directory_iterator(directory))if(entry.is_regular_file()&&entry.path().extension()==sharedExtension())files.push_back(entry.path());
   std::sort(files.begin(),files.end());
   for(const auto& file:files){auto library=std::make_unique<ModuleLibrary>();std::string error;if(!library->open(file,directory,error)){errors.push_back(file.filename().string()+": "+error);continue;}const auto* descriptor=library->api()->get_descriptor();bool duplicate=false;for(const auto& type:types)duplicate|=type.descriptor->type_id_hi==descriptor->type_id_hi&&type.descriptor->type_id_lo==descriptor->type_id_lo;if(duplicate){errors.push_back(file.filename().string()+": duplicate type identifier");continue;}types.push_back({std::move(library),descriptor,file});}
  }catch(const std::exception& e){errors.push_back(e.what());}return types.size();
 }
 static std::filesystem::path::string_type sharedExtension(){
#if defined(_WIN32)
  return L".dll";
#elif defined(__APPLE__)
  return ".dylib";
#else
  return ".so";
#endif
 }
 const std::vector<Type>& availableTypes()const noexcept{return types;}
 const std::vector<std::string>& loadErrors()const noexcept{return errors;}
 bool add(std::size_t typeIndex,uint64_t id,uint32_t lane=10){collectRetired();if(typeIndex>=types.size()||!id||find(id))return false;std::array<bool,10> used{};for(const auto& item:instances)if(item.id&&!item.removed&&item.lane<10)used[item.lane]=true;for(const auto& missing:unavailableStates)if(missing.lane<10)used[missing.lane]=true;if(lane==10){for(uint32_t i=0;i<10;++i)if(!used[i]){lane=i;break;}}if(lane>=10||used[lane])return false;for(auto& item:instances)if(!item.id){std::string error;auto* handle=types[typeIndex].library->create(error);if(!handle){errors.push_back(error);return false;}if(types[typeIndex].library->api()->prepare(handle,sampleRate,maximumFrames,2)!=MLP_OK){types[typeIndex].library->destroy(handle);errors.push_back("module prepare failed");return false;}item={id,&types[typeIndex],handle,false,false,lane,0};return true;}return false;}
 bool remove(uint64_t id)noexcept{auto* item=find(id);if(!item)return false;item->removed=true;return true;}
 bool setBypass(uint64_t id,bool bypass)noexcept{auto* item=find(id);if(!item||item->removed)return false;item->bypass=bypass;return true;}
 bool setParameter(uint64_t id,uint32_t parameter,double value)noexcept{auto* item=find(id);return item&&!item->removed&&item->type->library->api()->set_parameter(item->handle,parameter,value)==MLP_OK;}
 bool getParameter(uint64_t id,uint32_t parameter,double& value)const noexcept{const auto* item=find(id);return item&&!item->removed&&item->type->library->api()->get_parameter(item->handle,parameter,&value)==MLP_OK;}
 const MlpModuleDescriptor* descriptor(uint64_t id)const noexcept{const auto* item=find(id);return item&&!item->removed?item->type->descriptor:nullptr;}
 void prepare(double rate,uint32_t maximumBlock){sampleRate=std::clamp(std::isfinite(rate)?rate:44100.,8000.,768000.);maximumFrames=std::clamp<uint32_t>(maximumBlock,1,2048);graph.prepare(maximumFrames);capture.restart(sampleRate);for(auto& item:instances)if(item.id&&!item.removed)item.type->library->api()->prepare(item.handle,sampleRate,maximumFrames,2);}
 bool submit(const RackOrder& order){GraphSnapshot snapshot;snapshot.revision=++revision;for(auto id:order.slots())if(id)if(auto* item=find(id);item&&!item->removed){snapshot.nodes[snapshot.count++]={id,item->type->library->api(),item->handle,item->bypass};}if(!graph.submit(snapshot))return false;for(uint32_t i=0;i<snapshot.count;++i)if(auto* item=find(snapshot.nodes[i].instanceId))item->lastPublished=snapshot.revision;collectRetired();return true;}
 void setAnalysis(uint64_t id,bool module,bool listen)noexcept{analysisId.store(id,std::memory_order_release);moduleAnalysis.store(module,std::memory_order_release);listenDelta.store(listen,std::memory_order_release);}
 bool process(float* left,float* right,std::size_t frames)noexcept{
  if(!left)return false;
  std::size_t offset=0;while(offset<frames){const auto count=uint32_t(std::min<std::size_t>(maximumFrames,frames-offset));if(!processChunk(left+offset,right?right+offset:nullptr,count))return false;offset+=count;}return true;
 }
 bool pollMeters(ModuleRackMeters& value)noexcept{return meters.pop(value);}
 bool pollAnalysis(AnalysisFrame& value)noexcept{return analysisFrames.pop(value);}
 bool pollRetired(GraphSnapshot& value)noexcept{if(!graph.pollRetired(value))return false;retiredThrough=std::max(retiredThrough,value.revision);reclaimRemoved();return true;}
 void collectRetired()noexcept{GraphSnapshot value;while(pollRetired(value)){}reclaimRemoved();}
 void resetGraphWhenStopped()noexcept{graph.resetWhenStopped();for(auto& item:instances)if(item.id){item.lastPublished=0;if(item.removed){item.type->library->destroy(item.handle);item={};}}retiredThrough=0;}
 GraphDiagnostics diagnostics()const noexcept{return graph.diagnostics();}
 RackState saveState(const RackOrder& order)const{
  RackState state;for(std::size_t lane=0;lane<order.slots().size();++lane)if(const auto id=order.slots()[lane])if(const auto* item=find(id);item&&!item->removed){SavedModule saved;saved.instance=id;saved.typeHi=item->type->descriptor->type_id_hi;saved.typeLo=item->type->descriptor->type_id_lo;saved.version=item->type->descriptor->module_version;saved.lane=item->lane;saved.slot=uint32_t(lane);saved.bypass=item->bypass;const auto* api=item->type->library->api();for(uint32_t p=0;p<item->type->descriptor->parameter_count;++p){double value=0;if(api->get_parameter(item->handle,item->type->descriptor->parameters[p].id,&value)==MLP_OK)saved.parameters.push_back({item->type->descriptor->parameters[p].id,value});}uint32_t bytes=0,written=0;if(api->state_size(item->handle,&bytes)==MLP_OK&&bytes<=RackStateCodec::maximumOpaque){saved.opaque.resize(bytes);if(api->save_state(item->handle,saved.opaque.data(),bytes,&written)!=MLP_OK||written!=bytes)saved.opaque.clear();}state.modules.push_back(std::move(saved));}for(const auto& missing:unavailableStates)state.modules.push_back(missing);return state;
 }
 bool restoreStateWhenStopped(const RackState& state,RackOrder& order){
  clearInstancesWhenStopped();order=RackOrder{};unavailableStates.clear();bool complete=true;
  for(const auto& saved:state.modules){std::size_t typeIndex=types.size();for(std::size_t i=0;i<types.size();++i)if(types[i].descriptor->type_id_hi==saved.typeHi&&types[i].descriptor->type_id_lo==saved.typeLo){typeIndex=i;break;}if(typeIndex==types.size()||saved.slot>=10||order.slots()[saved.slot]||!add(typeIndex,saved.instance,saved.lane)){unavailableStates.push_back(saved);complete=false;continue;}if(!order.insert(saved.slot,saved.instance)){remove(saved.instance);unavailableStates.push_back(saved);complete=false;continue;}auto* item=find(saved.instance);item->bypass=saved.bypass;const auto* api=item->type->library->api();for(const auto& p:saved.parameters)api->set_parameter(item->handle,p.id,p.value);if(!saved.opaque.empty()&&api->load_state(item->handle,saved.opaque.data(),uint32_t(saved.opaque.size()))!=MLP_OK)complete=false;
  }return complete;
 }
 const std::vector<SavedModule>& unavailable()const noexcept{return unavailableStates;}
 void clearInstancesWhenStopped()noexcept{resetGraphWhenStopped();for(auto& item:instances)if(item.id&&item.type&&item.handle){item.type->library->destroy(item.handle);item={};}}
 void shutdownWhenStopped()noexcept{clearInstancesWhenStopped();types.clear();}
private:
 void reclaimRemoved()noexcept{for(auto& item:instances)if(item.id&&item.removed&&item.lastPublished<=retiredThrough){item.type->library->destroy(item.handle);item={};}}
 bool processChunk(float* left,float* right,uint32_t frames)noexcept{
  for(uint32_t i=0;i<frames;++i){raw[0][i]=std::isfinite(left[i])?left[i]:0.f;raw[1][i]=right&&std::isfinite(right[i])?right[i]:raw[0][i];}
  const bool module=moduleAnalysis.load(std::memory_order_acquire);const uint64_t selected=analysisId.load(std::memory_order_acquire);GraphTap tap;tap.instanceId=selected;tap.alignedBeforeLeft=aligned[0].data();tap.alignedBeforeRight=aligned[1].data();tap.afterLeft=tapped[0].data();tap.afterRight=tapped[1].data();
  if(!graph.process(left,right,frames,delta[0].data(),delta[1].data(),aligned[0].data(),aligned[1].data(),module&&selected?&tap:nullptr))return false;
  ModuleRackMeters value;value.input=measure(raw[0].data(),raw[1].data(),frames);value.output=measure(left,right?right:left,frames);value.delta=measure(delta[0].data(),delta[1].data(),frames);meters.push(value);
  const bool useModule=module&&selected&&tap.found;capture.setSource(useModule?selected:0);for(uint32_t i=0;i<frames;++i){const float bl=aligned[0][i],br=aligned[1][i];const float al=useModule?tapped[0][i]:left[i],ar=useModule?tapped[1][i]:(right?right[i]:left[i]);capture.push(bl,br,al,ar,[this](const AnalysisFrame& frame){analysisFrames.push(frame);});}
  const bool listen=listenDelta.load(std::memory_order_acquire)&&(!module||useModule);const float target=listen?1.f:0.f,step=1.f/float(std::max(1u,uint32_t(sampleRate*.02)));for(uint32_t i=0;i<frames;++i){monitorMix+=std::clamp(target-monitorMix,-step,step);const float dl=(useModule?tapped[0][i]-aligned[0][i]:delta[0][i]);const float dr=(useModule?tapped[1][i]-aligned[1][i]:delta[1][i]);left[i]+=monitorMix*(dl-left[i]);if(right)right[i]+=monitorMix*(dr-right[i]);}
  return true;
 }
 Instance* find(uint64_t id)noexcept{for(auto& item:instances)if(item.id==id)return &item;return nullptr;}const Instance* find(uint64_t id)const noexcept{for(const auto& item:instances)if(item.id==id)return &item;return nullptr;}
 std::filesystem::path moduleDirectory;std::vector<Type> types;std::vector<std::string> errors;std::vector<SavedModule> unavailableStates;std::array<Instance,32> instances{};ModularGraph graph;double sampleRate=44100;uint32_t maximumFrames=512;uint64_t revision=0,retiredThrough=0;
 std::array<std::array<float,2048>,2> raw{},aligned{},tapped{},delta{};AnalysisCapture capture;GraphSpsc<ModuleRackMeters,8> meters;GraphSpsc<AnalysisFrame,5> analysisFrames;std::atomic<uint64_t> analysisId{0};std::atomic<bool> moduleAnalysis{false},listenDelta{false};float monitorMix=0;
};
}
