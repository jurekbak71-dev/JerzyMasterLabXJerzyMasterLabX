#include "RackEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
// Smoke test of actual JUCE painting; deliberately no fake signal or modules.
// Assets are not checked here; this is the fallback rendering path.
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 ml::RackOrder order;
 ml::RackEditor editor(order,{}, {});
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
 std::cout<<"PASS: offscreen rack render (fallback skin, no audio)\n";
 return 0;
}
