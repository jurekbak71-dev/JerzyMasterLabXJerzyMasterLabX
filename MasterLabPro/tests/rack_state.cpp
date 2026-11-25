#include "RackState.h"
#include <cassert>
int main(){ml::RackState a;a.analysisSource=2;a.deltaListen=true;ml::SavedModule m;m.instance=7;m.typeHi=8;m.typeLo=9;m.version=3;m.lane=4;m.bypass=true;m.parameters={{2,1.25},{7,-3}};m.opaque={1,2,3,4};a.modules.push_back(m);std::vector<uint8_t> data;assert(ml::RackStateCodec::encode(a,data));ml::RackState b;assert(ml::RackStateCodec::decode(data.data(),data.size(),b));assert(a==b);data.push_back(0);assert(!ml::RackStateCodec::decode(data.data(),data.size(),b));data.resize(8);assert(!ml::RackStateCodec::decode(data.data(),data.size(),b));}
