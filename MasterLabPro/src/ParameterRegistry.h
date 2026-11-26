#pragma once
#include "../sdk/masterlabpro_module_abi.h"
#include <array>
#include <cstdint>

namespace ml {
class ParameterRegistry {
public:
 static constexpr uint32_t lanes=10,parametersPerLane=64;
 struct Entry {uint64_t instance=0,typeHi=0,typeLo=0;uint32_t localId=0,hostId=0,flags=0;double value=0,neutral=0;};
 bool attach(uint32_t lane,uint64_t instance,const MlpModuleDescriptor& d)noexcept{
  if(lane>=lanes||!instance||instances[lane]||d.parameter_count>parametersPerLane)return false;
  for(const auto id:instances)if(id==instance)return false;
  detachLane(lane);
  for(uint32_t i=0;i<d.parameter_count;++i){auto& e=entries[lane*parametersPerLane+i];const auto& p=d.parameters[i];e={instance,d.type_id_hi,d.type_id_lo,p.id,lane*parametersPerLane+i,p.flags,p.default_value,p.neutral_value};}
  instances[lane]=instance;return true;
 }
 bool detach(uint64_t instance)noexcept{for(uint32_t l=0;l<lanes;++l)if(instances[l]==instance){detachLane(l);return true;}return false;}
 Entry* byHost(uint32_t id)noexcept{return id<entries.size()&&entries[id].instance?&entries[id]:nullptr;}
 Entry* byLocal(uint64_t instance,uint32_t id)noexcept{for(auto& e:entries)if(e.instance==instance&&e.localId==id)return &e;return nullptr;}
 uint32_t laneOf(uint64_t instance)const noexcept{for(uint32_t l=0;l<lanes;++l)if(instances[l]==instance)return l;return lanes;}
private:
 void detachLane(uint32_t lane)noexcept{for(uint32_t i=0;i<parametersPerLane;++i)entries[lane*parametersPerLane+i]={};instances[lane]=0;}
 std::array<Entry,lanes*parametersPerLane> entries{};std::array<uint64_t,lanes> instances{};
};
}
