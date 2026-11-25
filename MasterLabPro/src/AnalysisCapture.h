#pragma once
#include "SpectralAnalysis.h"
#include <cstdint>
namespace ml {
// Producer owns the ring. Consumer receives complete chronological windows.
class AnalysisCapture {
 AnalysisFrame ring{},frame{};
 std::size_t pos=0,filled=0,hop=0;
 std::uint64_t clock=0,epoch=0,source=0;
 double rate=44100;
public:
 void restart(double sr) noexcept {rate=sr;clock=0;resetWindow();++epoch;}
 void setSource(std::uint64_t id) noexcept {if(id!=source){source=id;resetWindow();++epoch;}}
 template<class Sink> void push(float bl,float br,float al,float ar,Sink&& sink) noexcept {
  ring.samples[0][0][pos]=bl;ring.samples[0][1][pos]=br;
  ring.samples[1][0][pos]=al;ring.samples[1][1][pos]=ar;
  ring.samples[2][0][pos]=al-bl;ring.samples[2][1][pos]=ar-br;
  pos=(pos+1)%fftSize;++clock;
  if(filled<fftSize){if(++filled<fftSize)return;}else if(++hop<fftHop)return;
  hop=0;
  for(std::size_t t=0;t<3;++t)for(std::size_t c=0;c<2;++c){
   const auto& src=ring.samples[t][c];auto& dst=frame.samples[t][c];
   auto tail=fftSize-pos;
   std::copy_n(src.begin()+pos,tail,dst.begin());
   std::copy_n(src.begin(),pos,dst.begin()+tail);
  }
  frame.sampleRate=rate;frame.sourceId=source;frame.endSample=clock;frame.epoch=epoch;
  sink(frame); // rejected frames leave a detectable timestamp gap
 }
private:
 void resetWindow() noexcept {pos=filled=hop=0;}
};
}
