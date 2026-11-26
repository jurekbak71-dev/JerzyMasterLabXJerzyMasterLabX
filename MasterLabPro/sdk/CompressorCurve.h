#pragma once
#include <algorithm>
#include <cmath>

namespace ml {
inline float compressorReduction(float level,float threshold,float ratio,float knee,float analog) noexcept {
 const float over=level-threshold;
 const float slope=1-1/ratio;
 knee+=analog*5;
 float excess=std::max(0.f,over);
 if(knee>0&&std::abs(over)<knee*.5f){
  const float offset=over+knee*.5f;
  excess=offset*offset/(2*knee);
 }
 const float progressive=analog*.23f*excess/(excess+9.f);
 return excess*std::lerp(slope,1.f,progressive*slope);
}
}
