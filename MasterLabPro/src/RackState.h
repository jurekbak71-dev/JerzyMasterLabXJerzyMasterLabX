#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace ml {
struct SavedParameter{uint32_t id=0;double value=0;bool operator==(const SavedParameter&)const=default;};
struct SavedModule{
 uint64_t instance=0,typeHi=0,typeLo=0;uint32_t version=0,lane=0,slot=10;bool bypass=false;
 std::vector<SavedParameter> parameters;std::vector<uint8_t> opaque;
 bool operator==(const SavedModule&)const=default;
};
struct RackState{uint32_t format=2;std::vector<SavedModule> modules;uint32_t analysisSource=0;bool deltaListen=false;bool operator==(const RackState&)const=default;};
class RackStateCodec{
public:
 static constexpr uint32_t maximumBytes=4u*1024u*1024u,maximumOpaque=1024u*1024u;
 static bool encode(const RackState&,std::vector<uint8_t>&)noexcept;
 static bool decode(const uint8_t*,std::size_t,RackState&)noexcept;
};
}
