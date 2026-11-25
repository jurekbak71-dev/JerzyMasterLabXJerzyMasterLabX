#include "RackEditor.h"
#include "AssetLoader.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
// Smoke test of actual JUCE painting; deliberately no fake signal or modules.
// Optional explicit asset root makes missing or invalid skins a test failure.
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 ml::RackOrder order;
 ml::FrequencyBandSelector::Skins skins;
 juce::Image panel,slot;
 const bool strict=argc>2;
 if(strict){
  ml::AssetLoader assets(juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[2])));
  skins=assets.loadBands();panel=assets.loadPanel();slot=assets.loadSlot();
  if(!assets.missingFiles().isEmpty()){
   std::cerr<<assets.missingFiles().joinIntoString(", ").toStdString()<<"\n";return 4;
  }
  if(slot.getWidth()!=356||slot.getHeight()!=58)return 5;
 }
 ml::RackEditor editor(order,std::move(skins),{},panel);
 if(strict)editor.setRackSkin(slot);
 editor.setSize(1600,1000);
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
