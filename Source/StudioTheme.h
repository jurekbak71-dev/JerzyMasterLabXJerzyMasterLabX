#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class StudioTheme final : public juce::LookAndFeel_V4 {
public:
 StudioTheme(){
  setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffedf4fa));
  setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101720));
  setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff19232e));
  setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff354555));
  setColour(juce::ComboBox::textColourId,juce::Colour(0xffe5edf5));
  setColour(juce::ToggleButton::textColourId,juce::Colour(0xffd8e3ed));
  setColour(juce::ToggleButton::tickColourId,juce::Colour(0xff58dac5));
 }
 juce::Font getLabelFont(juce::Label& label) override {return juce::Font(juce::FontOptions(label.getParentComponent()&&dynamic_cast<juce::Slider*>(label.getParentComponent())?17.f:12.f));}
 juce::Font getTextButtonFont(juce::TextButton&,int) override {return juce::Font(juce::FontOptions(12.f,juce::Font::bold));}
 void drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool hover,bool down) override {
  auto r=button.getLocalBounds().toFloat().reduced(.5f);bool active=button.getToggleState();
  g.setColour(juce::Colour(active?0xff23423f:(hover||down?0xff283543:0xff17212d)));g.fillRoundedRectangle(r,5);
  g.setColour(juce::Colour(active?0xff58dac5:0xff364553));g.drawRoundedRectangle(r,5,1);
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider&) override {
  const float radius=std::min(width,height)*.5f-10,cx=x+width*.5f,cy=y+height*.5f;
  auto point=[&](float angle,float r){return juce::Point<float>(cx+std::sin(angle)*r,cy-std::cos(angle)*r);};
  for(int i=0;i<=10;++i){float a=start+(end-start)*i/10.f;auto a1=point(a,radius+5),a2=point(a,radius+8);g.setColour(juce::Colour(0xff566779));g.drawLine({a1,a2},1);}
  juce::Path track;track.addCentredArc(cx,cy,radius,radius,0,start,end,true);g.setColour(juce::Colour(0xff2b3946));g.strokePath(track,juce::PathStrokeType(4));
  juce::Path active;active.addCentredArc(cx,cy,radius,radius,0,start,start+(end-start)*position,true);g.setColour(juce::Colour(0xff58dac5));g.strokePath(active,juce::PathStrokeType(4,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  float inner=std::max(4.f,radius-8);g.setGradientFill(juce::ColourGradient(juce::Colour(0xff354353),cx,cy-inner,juce::Colour(0xff141d28),cx,cy+inner,false));g.fillEllipse(cx-inner,cy-inner,inner*2,inner*2);
  g.setColour(juce::Colour(0xff465667));g.drawEllipse(cx-inner,cy-inner,inner*2,inner*2,1);
  auto p1=point(start+(end-start)*position,inner*.45f),p2=point(start+(end-start)*position,inner*.83f);g.setColour(juce::Colour(0xffe3f7f2));g.drawLine({p1,p2},2.5f);
 }
};
