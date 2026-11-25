#pragma once
#include "SpectralAnalysis.h"
#include <cstdint>
namespace ml {
// Consumer-thread model. Columns use a fixed time grid, missing frames remain invalid.
class AnalysisHistory {
public:
 static constexpr int width=300,height=128;
 using Column=std::array<std::array<float,height>,3>;
 struct Cell {Column values{};bool valid=false;};
 std::array<Spectrum,3> spectra{};
 std::array<Cell,width> history{};
 bool accept(const AnalysisFrame& f){
  if(!std::isfinite(f.sampleRate)||f.sampleRate<8000||f.sampleRate>768000)return false;
  if(!ready||epoch!=f.epoch||source!=f.sourceId||rate!=f.sampleRate)clear();
  if(ready&&f.endSample<=last)return false;
  epoch=f.epoch;source=f.sourceId;rate=f.sampleRate;last=f.endSample;ready=true;
  for(int t=0;t<3;++t)spectra[t]=fft.stereoDb(f.samples[t]);
  const auto tick=static_cast<std::uint64_t>(double(f.endSample)*30./rate);
  if(!hasTick){lastTick=tick;hasTick=true;head=width-1;}
  else if(tick>lastTick){
   auto gap=std::min<std::uint64_t>(tick-lastTick,width);
   for(std::uint64_t n=0;n<gap;++n){head=(head+1)%width;history[head].valid=false;}
   lastTick=tick;
  }
  auto& col=history[head];col.valid=true;
  const double hi=std::min(20000.,rate*.5);
  for(int y=0;y<height;++y){
   const double hz=20.*std::pow(hi/20.,double(height-1-y)/(height-1));
   const auto k=std::clamp<std::size_t>(std::size_t(std::llround(hz*fftSize/rate)),1,fftBins-1);
   col.values[0][y]=spectra[0][k];col.values[1][y]=spectra[1][k];
   col.values[2][y]=std::max(spectra[0][k],spectra[1][k]) < -100.f ? 0.f : std::clamp(spectra[1][k]-spectra[0][k],-24.f,24.f);
  }
  return true;
 }
 const Cell& column(int x)const noexcept{return history[(head+1+x)%width];}
 double sampleRate()const noexcept{return rate;}
 void clear()noexcept{for(auto& c:history)c.valid=false;ready=hasTick=false;head=width-1;last=lastTick=0;}
private:
 SpectralAnalysis fft;bool ready=false,hasTick=false;int head=width-1;
 std::uint64_t epoch=0,source=0,last=0,lastTick=0;double rate=44100;
};
}
