#include "RackController.h"
#include "RackTiles.h"
#include "RotaryParameterControl.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <iostream>

namespace {
struct ParameterListener final:juce::AudioProcessorParameter::Listener {
 int starts=0,ends=0,changes=0;
 void parameterValueChanged(int,float) override {++changes;}
 void parameterGestureChanged(int,bool starting) override {(starting?starts:ends)++;}
};

juce::MouseEvent mouse(juce::Component& component,juce::ModifierKeys modifiers){
 const auto now=juce::Time::getCurrentTime();
 return {juce::Desktop::getInstance().getMainMouseSource(),{10.f,10.f},modifiers,
         1.f,0.f,0.f,0.f,0.f,&component,&component,now,{10.f,10.f},now,1,false};
}
}

int main(){
 juce::ScopedJuceInitialiser_GUI gui;
 juce::AudioParameterFloat gain(juce::ParameterID{"gain_test",1},"Gain",
   juce::NormalisableRange<float>(-24.f,24.f,.01f),0.f);
 ParameterListener listener;gain.addListener(&listener);
 gain.setValueNotifyingHost(gain.convertTo0to1(12.f));
 listener.starts=listener.ends=listener.changes=0;
 ml::RotaryParameterControl control(gain,0.f,{}, {},juce::Colours::cyan);
 control.mouseDown(mouse(control,juce::ModifierKeys::rightButtonModifier));
 if(std::abs(gain.get())>.001f||listener.starts!=1||listener.ends!=1||listener.changes<1)return 1;
 gain.removeListener(&listener);

 ml::RackController controller;
 const auto first=controller.add(0),second=controller.add(1);
 if(!first||!second||first==second)return 2;
 ml::RackTiles tiles(controller.order());tiles.setSize(1000,200);
 ml::RackOrder::Id selected=0;int selectedSlot=-1;
 tiles.onSelect=[&](ml::RackOrder::Id id,int slot){selected=id;selectedSlot=slot;};
 auto* firstTile=tiles.getChildComponent(0);if(!firstTile)return 3;
 firstTile->mouseDown(mouse(*firstTile,juce::ModifierKeys::leftButtonModifier));
 if(selected!=first||selectedSlot!=0)return 4;
 if(!controller.move(0,9)||controller.order().slots()[9]!=first||controller.order().slots()[1]!=second)return 5;
 std::cout<<"PASS right-click complete reset gesture, duplicate instances, selection and reorder\n";
}
