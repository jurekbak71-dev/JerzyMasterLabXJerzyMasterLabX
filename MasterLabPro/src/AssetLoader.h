#pragma once
#include "FrequencyBandSelector.h"
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
 juce::Image loadKnobCap(){return load("knob_cap.png");}
 juce::Image loadKnobPointer(){return load("knob_pointer.png");}
 const juce::StringArray& missingFiles() const noexcept{return missing;}
private:juce::File root;juce::StringArray missing;
};
}
