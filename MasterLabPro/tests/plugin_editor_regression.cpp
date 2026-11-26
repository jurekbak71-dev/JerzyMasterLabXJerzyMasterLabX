#include <juce_audio_processors/juce_audio_processors.h>
#include "RackTiles.h"
#include "RackState.h"
#include "RotaryParameterControl.h"
#include <memory>
#include <vector>
#include <iostream>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
namespace {
template<class T> T* find(juce::Component& c){
 if(auto* result=dynamic_cast<T*>(&c))return result;
 for(int i=0;i<c.getNumChildComponents();++i)if(auto* result=find<T>(*c.getChildComponent(i)))return result;
 return nullptr;
}
juce::Component* named(juce::Component& c,const juce::String& name){
 if(c.getName()==name)return &c;
 for(int i=0;i<c.getNumChildComponents();++i)if(auto* result=named(*c.getChildComponent(i),name))return result;
 return nullptr;
}
juce::MouseEvent mouse(juce::Component& c,juce::ModifierKeys mods){
 const auto now=juce::Time::getCurrentTime();
 return {juce::Desktop::getInstance().getMainMouseSource(),{10.f,10.f},mods,1.f,0.f,0.f,0.f,0.f,&c,&c,now,{10.f,10.f},now,1,false};
}
}
int main(){
 juce::ScopedJuceInitialiser_GUI gui;
 std::unique_ptr<juce::AudioProcessor> p(createPluginFilter());p->prepareToPlay(48000,256);
 ml::RackState state;ml::SavedModule m;m.instance=1;m.typeHi=0x4d4c505445434800ull;m.typeLo=0x6761696e00000002ull;m.version=1;m.lane=0;m.slot=0;m.parameters.push_back({1,6});state.modules.push_back(m);
 std::vector<uint8_t> bytes;if(!ml::RackStateCodec::encode(state,bytes))return 1;p->setStateInformation(bytes.data(),int(bytes.size()));
 std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());if(!editor)return 2;
 auto* tiles=find<ml::RackTiles>(*editor);if(!tiles||tiles->getNumChildComponents()!=10)return 3;
 for(int i=0;i<100;++i){
  auto* tile=tiles->getChildComponent(0);tile->mouseDown(mouse(*tile,juce::ModifierKeys::leftButtonModifier));
  auto* panel=named(*editor,"MasterLabPro Module Panel");if(!panel)return 4;
  auto* knob=find<ml::RotaryParameterControl>(*panel);if(!knob)return 5;if(knob->getName()!="Gain"||!knob->valueText().endsWith(" dB")||knob->dialDiameter()<65.f)return 7;
  knob->mouseDown(mouse(*knob,juce::ModifierKeys::rightButtonModifier));if(knob->valueText()!="0.00 dB")return 8;
 }
 // Empty tile must clear selection and open a menu without dereferencing ID zero.
 auto* empty=tiles->getChildComponent(1);empty->mouseDown(mouse(*empty,juce::ModifierKeys::leftButtonModifier));
 if(named(*editor,"MasterLabPro Module Panel"))return 6;
 juce::PopupMenu::dismissAllActiveMenus();editor.reset();p->releaseResources();
 std::cout<<"PASS actual plugin editor: 100 selections, knob resets, empty tile\n";
}
