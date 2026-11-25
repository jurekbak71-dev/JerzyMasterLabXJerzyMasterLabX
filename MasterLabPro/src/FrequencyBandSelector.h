#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
namespace ml {
class FrequencyBandSelector final: public juce::Component {
public:
 using Skins=std::array<std::array<juce::Image,4>,8>; // normal, hover, active, disabled
 explicit FrequencyBandSelector(Skins s):skins(std::move(s)) {for(int i=0;i<8;++i){buttons[i].setButtonText(names[i]);buttons[i].setTooltip(names[i]);buttons[i].onClick=[this,i]{setSelected(i);if(onSelection)onSelection(i);};addAndMakeVisible(buttons[i]);}initialiseSkins();setSelected(7);}
 void setSelected(int n){if(n<0||n>=8)return;selected=n;for(int i=0;i<8;++i)buttons[i].setToggleState(i==n,juce::dontSendNotification);}
 int getSelected()const{return selected;}
 std::function<void(int)> onSelection;
 void resized() override {for(int i=0;i<8;++i){int x=i*getWidth()/8;buttons[i].setBounds(x,0,(i+1)*getWidth()/8-x,getHeight());}}
private:
 struct BandButton:juce::Button {BandButton():Button("Band"){} const std::array<juce::Image,4>* images=nullptr;void paintButton(juce::Graphics& g,bool hover,bool down) override {int s=!isEnabled()?3:getToggleState()||down?2:hover?1:0;if(images&&(*images)[s].isValid())g.drawImage((*images)[s],getLocalBounds().toFloat().reduced(2));else{g.fillAll(juce::Colour(0xff20252b));g.setColour(juce::Colours::white);g.drawText(getButtonText(),getLocalBounds(),juce::Justification::centred);}}};
 Skins skins;std::array<BandButton,8> buttons;int selected=7;
 inline static const char* names[]{"SUB","LOW","LOW MID","MID","HIGH MID","HIGH","AIR","FULL"};
public:
 void initialiseSkins(){for(int i=0;i<8;++i)buttons[i].images=&skins[i];repaint();}
};}
