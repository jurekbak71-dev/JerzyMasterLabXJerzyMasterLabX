#pragma once
#include "FrequencyBandSelector.h"
#include <algorithm>
#include <cstdint>
namespace ml {
// Asset root points to the assets directory from the G3.04 ZIP.
// Missing files are reported, not treated as successful loads.
struct AssetLoader {
 explicit AssetLoader(juce::File root):root(std::move(root)){}
 juce::Image load(const juce::String& relative){
  auto file=root.getChildFile(relative);
  auto img=juce::ImageFileFormat::loadFrom(file);
  if(!img.isValid())missing.add(relative);
  return img;
 }
 FrequencyBandSelector::Skins loadBands(){
  FrequencyBandSelector::Skins result;
  const char* names[]{"sub","low","low_mid","mid","high_mid","high","air","full"};
  const char* states[]{"normal","hover","active","disabled"};
  for(int i=0;i<8;++i)for(int j=0;j<4;++j)
   result[i][j]=load(juce::String("bands/")+names[i]+"_"+states[j]+".png");
  return result;
 }
 juce::Image loadSlot(){return load("approved/04_rack/slot_empty.png");}
 juce::Image loadPanel(){return load("approved/07_panels/large_frame.png");}
 juce::Image loadKnobCap(){return removeBlackMatte(load("approved/01_controls/cap_silver.png"));}
 juce::Image loadKnobPointer(){return removeBlackMatte(load("approved/01_controls/pointer_silver.png"));}
 const juce::StringArray& missingFiles() const noexcept{return missing;}
private:
 static juce::Image removeBlackMatte(const juce::Image& source){
  if(!source.isValid())return {};
  juce::Image image(juce::Image::ARGB,source.getWidth(),source.getHeight(),true);
  for(int y=0;y<source.getHeight();++y)for(int x=0;x<source.getWidth();++x){
   auto pixel=source.getPixelAt(x,y);const auto brightness=std::max(pixel.getRed(),std::max(pixel.getGreen(),pixel.getBlue()));
   const auto alpha=juce::jlimit(0,255,(brightness-12)*255/34);
   image.setPixelAt(x,y,pixel.withAlpha(float(alpha)/255.f));
  }
  return image;
 }
 juce::File root;juce::StringArray missing;
};
}
