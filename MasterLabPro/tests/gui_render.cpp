#include "RackEditor.h"
#include "AssetLoader.h"
#include "RotaryParameterControl.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
// Smoke test of actual JUCE painting; strict mode uses a real gain parameter, no fake telemetry.
// Optional explicit asset root makes missing or invalid skins a test failure.
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 ml::RackOrder order;
 ml::FrequencyBandSelector::Skins skins;
 juce::Image panel,slot,cap,pointer;
 const bool strict=argc>2;
 if(strict){
  ml::AssetLoader assets(juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[2])));
  skins=assets.loadBands();panel=assets.loadPanel();slot=assets.loadSlot();cap=assets.loadKnobCap();pointer=assets.loadKnobPointer();
  if(!assets.missingFiles().isEmpty()){
   std::cerr<<assets.missingFiles().joinIntoString(", ").toStdString()<<"\n";return 4;
  }
  if(slot.getWidth()!=356||slot.getHeight()!=58)return 5;
  if(cap.getWidth()!=68||cap.getHeight()!=69||pointer.getWidth()!=68||pointer.getHeight()!=68)return 6;
 }
 juce::AudioParameterFloat gain(juce::ParameterID{"gain_test",1},"Gain",juce::NormalisableRange<float>(-24.f,24.f,.01f),0.f);
 ml::RackEditor editor(order,std::move(skins),{},panel);
 if(strict)editor.setRackSkin(slot);
 editor.setSize(1600,1000);
 if(strict){
  if(!order.insert(0,1))return 7;
  editor.refreshRack();editor.setSelection(1);editor.setBandFromController(7);editor.setBandSelectionEnabled(false);
  editor.setModuleEditor(std::make_unique<ml::RotaryParameterControl>(gain,0.f,cap,pointer,juce::Colour(0xff58bde8)));
 }
 auto img=editor.createComponentSnapshot(editor.getLocalBounds(),true,1.0f);
 if(!img.isValid()||img.getWidth()!=1600||img.getHeight()!=1000)return 1;
 if(argc>1){
  auto file=juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[1]));
  if(!file.deleteFile())return 2;
  juce::FileOutputStream stream(file);
  juce::PNGImageFormat png;
  if(!stream.openedOk()||!png.writeImageToStream(img,stream))return 3;
 }
 std::cout<<"PASS: offscreen rack render ("<<(strict?"approved assets":"fallback skin")<<", no audio)\n";
 return 0;
}
