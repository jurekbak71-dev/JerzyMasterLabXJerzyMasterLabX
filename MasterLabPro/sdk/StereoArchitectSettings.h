#pragma once
#include "masterlabpro_module_abi.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace ml {
inline constexpr uint64_t stereoTypeHi=UINT64_C(0x4d4c505354455245);
inline constexpr uint64_t stereoTypeLo=UINT64_C(0x473a093428d79451);
inline constexpr std::array<double,6> stereoSplits{60,250,500,2000,4000,8000};
inline constexpr std::array<const char*,7> stereoBands{"SUB","LOW","LOW MID","MID","HIGH MID","HIGH","AIR"};
inline std::array<float,6> orderedStereoSplits(std::array<float,6> requested,float upper) noexcept {
 float minimum=20.f;
 for(size_t i=0;i<requested.size();++i){
  const auto maximum=upper/std::pow(1.05f,float(requested.size()-i));
  requested[i]=std::clamp(requested[i],minimum,maximum);
  minimum=requested[i]*1.05f;
 }
 return requested;
}
}
