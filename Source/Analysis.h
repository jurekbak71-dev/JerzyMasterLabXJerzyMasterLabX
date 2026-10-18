#pragma once
#include <cmath>
namespace masterlab {
// Hann-windowed, bin-centred sinusoid amplitude, averaged in energy over L/R.
// This is an amplitude spectrum, not a PSD, LUFS or True Peak measurement.
inline float stereoSpectrumAmplitude(float leftMagnitude,float rightMagnitude,int fftSize) noexcept {
    const double l=leftMagnitude,r=rightMagnitude;
    return static_cast<float>(std::sqrt((l*l+r*r)*.5)*4/(fftSize-1));
}
}
