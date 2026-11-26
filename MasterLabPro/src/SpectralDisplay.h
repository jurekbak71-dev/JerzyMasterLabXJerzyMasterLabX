#pragma once
#include "AnalysisHistory.h"
#include "MeasurementViews.h"

namespace ml {
class SpectralDisplay {
public:
 void clear() noexcept {initialized=false;}
 std::array<std::vector<SpectrumBin>,3> traces(const AnalysisHistory& source,double dt) {
  std::array<std::vector<SpectrumBin>,3> result;
  const double rate=source.sampleRate();
  const auto limit=std::min(20000.,rate*.5);
  for(size_t channel=0;channel<result.size();++channel){
   auto& output=result[channel];output.reserve(points);
   for(size_t p=0;p<points;++p){
    const double hz=20.*std::pow(limit/20.,double(p)/double(points-1));
    const auto location=hz*fftSize/rate;
    const auto centre=std::clamp<size_t>(size_t(std::llround(location)),1,fftBins-1);
    const auto radius=std::max<size_t>(1,centre/35);
    const auto first=centre>radius?centre-radius:1;
    const auto last=std::min(fftBins-1,centre+radius);
    float power=0;
    for(auto k=first;k<=last;++k)power+=std::pow(10.f,source.spectra[channel][k]*.1f);
    const float target=10.f*std::log10(std::max(power/float(last-first+1),1e-12f));
    auto& value=levels[channel][p];
    const auto time=target>value?.065:.27;
    value=initialized?std::lerp(value,target,float(-std::expm1(-std::clamp(dt,0.,.5)/time))):target;
    output.push_back({float(hz),value});
   }
  }
  initialized=true;return result;
 }
private:
 static constexpr size_t points=256;
 std::array<std::array<float,points>,3> levels{};
 bool initialized=false;
};

// Time only contributes to the decaying magnitude. X is frequency in every row.
inline std::array<juce::Image,3> renderSpectralCurtain(const AnalysisHistory& history){
 constexpr int width=384,height=40;
 std::array<std::array<float,AnalysisHistory::height>,3> energy{};
 const double nyquist=std::min(20000.,history.sampleRate()*.5);
 for(int age=0;age<AnalysisHistory::width;++age){
  const auto& frame=history.column(AnalysisHistory::width-1-age);
  if(!frame.valid)continue;
  const float weight=std::exp(-float(age)/58.f)*(1.f-float(age)/AnalysisHistory::width);
  for(size_t trace=0;trace<energy.size();++trace)
   for(size_t frequency=0;frequency<energy[trace].size();++frequency){
    const float db=trace==2?frame.audioDelta[frequency]:frame.values[trace][frequency];
    const float magnitude=(db+95.f)/95.f;
    energy[trace][frequency]=std::max(energy[trace][frequency],std::clamp(magnitude,0.f,1.f)*weight);
   }
 }
 const juce::Colour cold(0xff0b102b),violet(0xff6636bc),hot(0xffdf4238),bright(0xffffdf72);
 std::array<juce::Image,3> images;
 for(size_t trace=0;trace<images.size();++trace){
  images[trace]=juce::Image(juce::Image::RGB,width,height,true);
  juce::Image::BitmapData pixels(images[trace],juce::Image::BitmapData::writeOnly);
  for(int x=0;x<width;++x){
   const auto hz=20.*std::pow(1000.,double(x)/(width-1));
   const float coord=float((1.-std::log(hz/20.)/std::log(nyquist/20.))*(AnalysisHistory::height-1));
   const auto index=juce::jlimit(0,AnalysisHistory::height-1,int(std::max(0.f,coord)));
   const auto next=std::min(index+1,AnalysisHistory::height-1);
   const float level=hz>nyquist?0.f:std::lerp(energy[trace][size_t(index)],energy[trace][size_t(next)],std::clamp(coord-float(index),0.f,1.f));
   const auto colour=level<.48f?cold.interpolatedWith(violet,level/.48f):level<.76f?violet.interpolatedWith(hot,(level-.48f)/.28f):hot.interpolatedWith(bright,(level-.76f)/.24f);
   for(int y=0;y<height;++y){
    const float distance=std::abs(2.f*float(y)/float(height-1)-1.f);
    const float spread=.12f+.75f*level;
    const float glow=level*std::exp(-2.f*distance*distance/(spread*spread));
    pixels.setPixelColour(x,y,cold.interpolatedWith(colour,std::clamp(glow*1.5f,0.f,1.f)));
   }
  }
 }
 return images;
}
}
