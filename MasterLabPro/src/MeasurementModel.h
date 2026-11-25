#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace ml {
inline float amplitudeDb(float a) noexcept {return std::isfinite(a)&&a>0?std::max(-120.f,20.f*std::log10(a)):-120.f;}
struct StereoLevels {std::array<float,2> peak{-120,-120},rms{-120,-120};std::array<bool,2> clip{};};
inline StereoLevels measure(const float* left,const float* right,std::size_t n) noexcept {
 StereoLevels out; const float* channels[]{left,right};
 for(int c=0;c<2;++c){if(!channels[c]||!n)continue; double sum=0;float p=0; for(std::size_t i=0;i<n;++i){float x=channels[c][i];if(!std::isfinite(x))continue;p=std::max(p,std::abs(x));sum+=double(x)*x;}out.peak[c]=amplitudeDb(p);out.rms[c]=amplitudeDb(float(std::sqrt(sum/n)));out.clip[c]=p>=1.f;}return out;
}
inline float frequencyX(float hz,float nyquist=20000.f) noexcept {const float hi=std::clamp(nyquist,21.f,20000.f);return std::log(std::clamp(hz,20.f,hi)/20.f)/std::log(hi/20.f);}
inline float dbY(float db,float floor=-90.f,float ceiling=12.f) noexcept {return 1.f-std::clamp((db-floor)/(ceiling-floor),0.f,1.f);}
inline float levelDifference(float before,float after) noexcept {return std::clamp(after-before,-24.f,24.f);}
}
