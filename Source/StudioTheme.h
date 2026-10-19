#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

class StudioTheme final : public juce::LookAndFeel_V4 {
juce::Typeface::Ptr face=juce::Typeface::createSystemTypefaceFor(BinaryData::Manrope_ttf,BinaryData::Manrope_ttfSize);
public:
 juce::Font font(float size)const{return juce::Font(juce::FontOptions(face).withHeight(size*1.3f));}
 juce::Typeface::Ptr getTypefaceForFont(const juce::Font&)override{return face;}
 StudioTheme(){
  setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffedf4fa));
  setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101720));
  setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff19232e));
  setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff354555));
  setColour(juce::ComboBox::textColourId,juce::Colour(0xffe5edf5));
  setColour(juce::ToggleButton::textColourId,juce::Colour(0xffd8e3ed));
  setColour(juce::ToggleButton::tickColourId,juce::Colour(0xff82dfcd));
 }
 juce::Font getLabelFont(juce::Label& label) override {
  auto* slider=dynamic_cast<juce::Slider*>(label.getParentComponent());
  return font(slider?(slider->getSliderStyle()==juce::Slider::RotaryHorizontalVerticalDrag?21.f:14.f):12.f);
 }
 juce::Font getPopupMenuFont()override{return font(13.f);}

 juce::Font getTextButtonFont(juce::TextButton&,int) override {return font(12.f);}
 juce::Font getComboBoxFont(juce::ComboBox&) override{return font(13.f);}
 void drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox& box)override{
  g.setColour(juce::Colour(0xff24272f));g.fillRoundedRectangle(.5f,.5f,w-1.f,h-1.f,h*.48f);
  const float cx=w-17.f,cy=h*.5f;const bool lit=box.isPopupActive()||box.isMouseOver(true)||box.hasKeyboardFocus(true);
  g.setColour(juce::Colour(lit?0xff3c645e:0xff323841));g.fillEllipse(cx-8,cy-8,16,16);
  if(lit){g.setColour(juce::Colour(0xff82dfcd).withAlpha(.13f));g.fillEllipse(cx-6,cy-6,12,12);}
  g.setColour(juce::Colour(lit?0xffa2f3df:0xff849b98).withAlpha(box.isEnabled()?1.f:.35f));g.fillEllipse(cx-2,cy-2,4,4);
  if(box.hasKeyboardFocus(true)){g.setColour(juce::Colour(0xff73ddce));g.drawRoundedRectangle(.5f,.5f,w-1.f,h-1.f,h*.48f,1);}
 }
 void drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool hover,bool)override{
  const float y=b.getHeight()*.5f;g.setColour(juce::Colour(b.getToggleState()?0xff64cbbb:0xff424650));g.fillRoundedRectangle(0,y-7,25,14,7);
  g.setColour(juce::Colours::white.withAlpha(b.isEnabled()?1.f:.4f));g.fillEllipse(b.getToggleState()?13.f:3.f,y-4,8,8);g.setFont(font(12.f));g.setColour(juce::Colour(hover?0xffeff5f5:0xffbdc5d0));g.drawText(b.getButtonText(),32,0,b.getWidth()-32,b.getHeight(),juce::Justification::left);
 }
 void drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour&,bool hover,bool down) override {
  auto r=button.getLocalBounds().toFloat().reduced(.5f);const bool active=button.getToggleState(),primary=button.getComponentID()=="brain-apply"||button.getComponentID()=="brain-analyse";
  g.setColour(juce::Colour(active?0xff31504d:(primary?0xff315b55:(hover||down?0xff353b45:0xff242830))).withAlpha(button.isEnabled()?1.f:.35f));g.fillRoundedRectangle(r,r.getHeight()*.5f);
  if(button.hasKeyboardFocus(true)){g.setColour(juce::Colour(0xff73ddce));g.drawRoundedRectangle(r,r.getHeight()*.5f,1);}
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider&) override {
  const float radius=std::min(width,height)*.5f-10,cx=x+width*.5f,cy=y+height*.5f;
  auto point=[&](float angle,float r){return juce::Point<float>(cx+std::sin(angle)*r,cy-std::cos(angle)*r);};

  juce::Path track;track.addCentredArc(cx,cy,radius,radius,0,start,end,true);g.setColour(juce::Colour(0xff343a43));g.strokePath(track,juce::PathStrokeType(4));
  juce::Path active;active.addCentredArc(cx,cy,radius,radius,0,start,start+(end-start)*position,true);g.setColour(juce::Colour(0xff82dfcd));g.strokePath(active,juce::PathStrokeType(4,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  float inner=std::max(4.f,radius-8);g.setGradientFill(juce::ColourGradient(juce::Colour(0xff373e48),cx,cy-inner,juce::Colour(0xff171b23),cx,cy+inner,false));g.fillEllipse(cx-inner,cy-inner,inner*2,inner*2);
  g.setColour(juce::Colour(0xff4a535f));g.drawEllipse(cx-inner,cy-inner,inner*2,inner*2,.6f);
  auto p1=point(start+(end-start)*position,inner*.45f),p2=point(start+(end-start)*position,inner*.83f);g.setColour(juce::Colour(0xffe3f7f2));g.drawLine({p1,p2},2.5f);
 }
};
