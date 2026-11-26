#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

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
 void setIndicator(juce::Colour value){indicator=value;alwaysLit=true;repaint();}
 void loadSkins(const juce::File& root){
  const char* states[]{"button_off.png","button_on.png","button_hover.png","button_pressed.png"};
  for(size_t i=0;i<skins.size();++i)skins[i]=juce::ImageFileFormat::loadFrom(root.getChildFile("approved/01_controls").getChildFile(states[i]));
  repaint();
 }
 void paintButton(juce::Graphics& g,bool hover,bool pressed)override {
  const auto face=getLocalBounds().toFloat().reduced(1.f);
  if(face.getWidth()<12||face.getHeight()<8)return;
  const bool active=getToggleState(),enabled=isEnabled();
  const auto top=juce::Colour(pressed?0xff18232a:hover?0xff50636e:0xff394a55);
  const auto bottom=juce::Colour(pressed?0xff0d171d:0xff10181d);
  const auto& skin=skins[pressed?3:active?1:hover?2:0];
  if(skin.isValid()){
   const float edge=std::min(face.getWidth()*.24f,face.getHeight()*.12f);
   auto middle=face;auto left=middle.removeFromLeft(edge),right=middle.removeFromRight(edge);
   const int sw=skin.getWidth(),sh=skin.getHeight(),cut=std::max(1,sw/18);
   g.drawImage(skin.getClippedImage({0,0,cut,sh}),left);
   // The supplied state samples contain baked demo labels and a lamp.
   // Only use the outer bevel from the approved artwork; render a clean face.
   const auto faceTop=juce::Colour(pressed?0xff151b20:0xff353a3e);
   g.setGradientFill({faceTop,middle.getTopLeft(),juce::Colour(0xff080b0e),middle.getBottomLeft(),false});
   g.fillRect(middle);
   g.setColour(juce::Colour(0xff6e7578));g.drawHorizontalLine(int(middle.getY()+1),middle.getX(),middle.getRight());
   g.setColour(juce::Colour(0xff050708));g.drawHorizontalLine(int(middle.getBottom()-2),middle.getX(),middle.getRight());
   g.drawImage(skin.getClippedImage({sw-cut,0,cut,sh}),right);
  }else{
  g.setGradientFill({top,face.getTopLeft(),bottom,face.getBottomLeft(),false});
  g.fillRoundedRectangle(face,3);
  g.setColour(active?accent.withAlpha(.85f):juce::Colour(0xff86949d).withAlpha(hover?.8f:.4f));
  g.drawRoundedRectangle(face,3,1);
  }
  auto label=getLocalBounds().reduced(6,1);
  const auto led=label.removeFromLeft(15).toFloat().withSizeKeepingCentre(7,7);
  const auto colour=alwaysLit?indicator:active?accent:juce::Colour(0xff30383c);
  g.setColour(juce::Colour(0xff080b0d));g.fillEllipse(led.expanded(2));
  if(alwaysLit||active){g.setColour(colour.withAlpha(.23f));g.fillEllipse(led.expanded(4));}
  g.setColour(colour);g.fillEllipse(led);
  g.setColour(colour.brighter(.4f));g.fillEllipse(led.withSizeKeepingCentre(3,3));
  g.setColour(juce::Colours::whitesmoke.withAlpha(enabled?1.f:.40f));
  g.setFont(juce::FontOptions(juce::jlimit(9.f,12.f,face.getHeight()*.46f),juce::Font::bold));
  g.drawFittedText(getButtonText(),label,juce::Justification::centred,1,.66f);
 }
private:
 std::array<juce::Image,4> skins{};
 juce::Colour accent{0xff65c7e9},indicator{};
 bool alwaysLit=false;
};
}
