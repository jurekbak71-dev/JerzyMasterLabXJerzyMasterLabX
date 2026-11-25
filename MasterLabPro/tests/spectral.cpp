#include "SpectralAnalysis.h"
#include <iostream>
int main(){
 ml::SpectralAnalysis fft;ml::AnalysisFrame frame;
 constexpr std::size_t bin=128;
 for(std::size_t i=0;i<ml::fftSize;++i){
  float x=float(.5*std::sin(2*std::numbers::pi*bin*i/ml::fftSize));
  frame.samples[0][0][i]=x;frame.samples[0][1][i]=-x;
  for(int c=0;c<2;++c){frame.samples[1][c][i]=2*frame.samples[0][c][i];frame.samples[2][c][i]=frame.samples[1][c][i]-frame.samples[0][c][i];}
 }
 auto a=fft.stereoDb(frame.samples[0]),b=fft.stereoDb(frame.samples[1]),d=fft.stereoDb(frame.samples[2]);
 if(std::abs(a[bin]+6.0206f)>.001f||std::abs(b[bin])>.001f||std::abs(d[bin]-a[bin])>.001f)return 1;
 if(std::size_t(std::max_element(a.begin(),a.end())-a.begin())!=bin)return 2;
 if(std::abs(a[bin+1]+12.0412f)>.001f)return 3;
 frame.samples[0]={};a=fft.stereoDb(frame.samples[0]);for(float x:a)if(x!=-120.f)return 4;
 std::cout<<"PASS FFT frequency, amplitude, stereo anti-phase, Hann and silence\n";
}
