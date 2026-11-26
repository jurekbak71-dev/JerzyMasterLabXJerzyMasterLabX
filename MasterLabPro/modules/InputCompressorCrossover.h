#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace ml {
inline float flushCompressorSample(float x) noexcept {
 return std::isfinite(x)&&std::abs(x)>=1e-25f ? x : 0.f;
}
class CompressorCrossover {
public:
 static float coefficient(float hz,float rate) noexcept {
  const float g=std::tan(std::numbers::pi_v<float>*std::min(hz,rate*.45f)/rate);
  return g/(1+g);
 }
 std::array<float,3> process(float input,float lowCoefficient,float highCoefficient) noexcept {
  const float low=lowpass(input,lowState,lowCoefficient);
  const float upper=lowpass(input,upperState,highCoefficient);
  return {low,upper-low,input-upper};
 }
private:
 static float lowpass(float input,float& state,float coefficient) noexcept {
  const float change=(input-state)*coefficient;
  const float output=change+state;
  state=flushCompressorSample(output+change);
  return output;
 }
 float lowState=0,upperState=0;
};
}
