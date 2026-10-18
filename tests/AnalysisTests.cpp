#include <juce_dsp/juce_dsp.h>
#include "Analysis.h"
#include <iostream>
int main(){
 for(int order:{12,13,14,15}){
  const int size=1<<order,bin=171;juce::dsp::FFT fft(order);
  std::vector<float> left(size*2),right(size*2);
  for(int i=0;i<size;++i){float sample=.5f*std::sin(juce::MathConstants<float>::twoPi*bin*i/size)*(.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/(size-1)));left[i]=sample;right[i]=-sample;}
  fft.performFrequencyOnlyForwardTransform(left.data());fft.performFrequencyOnlyForwardTransform(right.data());
  float amplitude=masterlab::stereoSpectrumAmplitude(left[bin],right[bin],size);
  if(std::abs(amplitude-.5f)>.001f){std::cerr<<"anti-phase spectrum cancelled or amplitude calibration failed";return 1;}
  float mono=masterlab::stereoSpectrumAmplitude(left[bin],0,size);
  if(std::abs(mono-.5f/std::sqrt(2.f))>.001f){std::cerr<<"single-channel energy calibration failed";return 1;}
 }
 if(masterlab::stereoSpectrumAmplitude(0,0,8192)!=0)return 1;
 std::cout<<"Passed: stereo-energy FFT preserves anti-phase audio, coherent amplitude and single-channel calibration at all four FFT sizes\n";
}
