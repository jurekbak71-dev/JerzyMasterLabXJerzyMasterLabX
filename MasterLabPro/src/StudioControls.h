#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace ml {
// Accents sampled from approved active band crops. FULL uses their neutral metal.
inline juce::Colour bandAccent(int band) {
 constexpr uint32_t colours[]{0xffa050d0,0xff2088d8,0xff20b4d0,0xff48b853,0xffc69821,0xffdf7122,0xffd34b5a,0xffbec8d0};
 return juce::Colour(colours[juce::jlimit(0,7,band)]);
}
class StudioButton : public juce::TextButton {
public:
 StudioButton()=default;
 explicit StudioButton(const juce::String& text):juce::TextButton(text){}
 void setAccent(juce::Colour value){accent=value;repaint();}
 void paintButton(juce::Graphics& g,bool hover,bool pressed)override {
  const auto face=getLocalBounds().toFloat().reduced(1.f);
  if(face.getWidth()<12||face.getHeight()<8)return;
  const bool active=getToggleState(),enabled=isEnabled();
  const auto top=juce::Colour(pressed?0xff18232a:hover?0xff50636e:0xff394a55);
  const auto bottom=juce::Colour(pressed?0xff0d171d:0xff10181d);
  g.setGradientFill({top,face.getTopLeft(),bottom,face.getBottomLeft(),false});
  g.fillRoundedRectangle(face,3);
  g.setColour(active?accent.withAlpha(.85f):juce::Colour(0xff86949d).withAlpha(hover?.8f:.4f));
  g.drawRoundedRectangle(face,3,1);
  auto label=getLocalBounds().reduced(6,1);
  if(active){
   const auto led=label.removeFromLeft(11).toFloat().withSizeKeepingCentre(5,5);
   g.setColour(accent.withAlpha(.17f));g.fillEllipse(led.expanded(3));
   g.setColour(accent.brighter(.25f));g.fillEllipse(led);
  }
  g.setColour(juce::Colours::whitesmoke.withAlpha(enabled?1.f:.40f));
  g.setFont(juce::FontOptions(juce::jlimit(9.f,13.f,face.getHeight()*.58f),juce::Font::bold));
  g.drawFittedText(getButtonText(),label,juce::Justification::centred,1,.66f);
 }
private:
 juce::Colour accent{0xff65c7e9};
};
}
