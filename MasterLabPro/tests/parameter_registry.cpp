#include "ParameterRegistry.h"
#include <cassert>
int main(){MlpParameterDescriptor p[]{
 {sizeof(MlpParameterDescriptor),7,MLP_PARAMETER_AUTOMATABLE,0,"Gain","dB",-24,24,0,0,.01,5},
 {sizeof(MlpParameterDescriptor),9,0,0,"Mode","",0,2,0,0,1,0}};
 MlpModuleDescriptor d{sizeof(d),1,0,1,2,3,0,1,2,2,0,"x","x","x",p};ml::ParameterRegistry r;
 assert(r.attach(3,42,d));assert(r.byLocal(42,7)->hostId==3*64);const auto id=r.byLocal(42,9)->hostId;
 // Rack reorder does not touch the instance lane or host parameter identifier.
 assert(r.laneOf(42)==3&&r.byHost(id)->instance==42);assert(!r.attach(4,42,d));assert(!r.attach(3,43,d));assert(r.detach(42));assert(!r.byHost(id));}
