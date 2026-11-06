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
  // Recessed hardware selector with a clear machined edge.
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff344350),0.f,0.f,juce::Colour(0xff101820),0.f,static_cast<float>(h),false));
  g.fillRoundedRectangle(.5f,.5f,w-1.f,h-1.f,5.f);
  g.setColour(juce::Colour(0xff63788a));g.drawRoundedRectangle(.5f,.5f,w-1.f,h-1.f,5.f,.8f);
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
  const auto upper=juce::Colour(active?0xff35695f:(primary?0xff356a63:(hover||down?0xff3b5363:0xff344453)));
  const auto lower=juce::Colour(active?0xff183d37:(primary?0xff21443e:0xff131b25));
  g.setGradientFill(juce::ColourGradient(upper.withAlpha(button.isEnabled()?1.f:.35f),r.getX(),r.getY(),lower,r.getX(),r.getBottom(),false));
  g.fillRoundedRectangle(r,5.f);
  g.setColour(juce::Colour(active?0xffa2d8c8:0xff526b7b));g.drawRoundedRectangle(r,5.f,1.f);
  if(button.hasKeyboardFocus(true)){g.setColour(juce::Colour(0xff73ddce));g.drawRoundedRectangle(r.reduced(2.f),4.f,1.3f);}
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,
                       float position,float start,float end,juce::Slider& slider) override {
  // Pure vector-rendered metal knob: bevelled chassis, radial scale, moving
  // index pointer. Parameter automation remains owned by JUCE/APVTS.
  const float size=static_cast<float>(std::min(width,height));
  if(size<24.f)return;
  const float cx=x+width*.5f,cy=y+height*.5f-2.f;
  const float ring=std::max(11.f,size*.5f-13.f);
  const float body=ring*.74f;
  const float angle=start+(end-start)*juce::jlimit(0.f,1.f,position);
  const auto accent=slider.findColour(juce::Slider::rotarySliderFillColourId);
  const auto metal=juce::Colour(0xff606b77);
  auto polar=[&](float theta,float radius){
   return juce::Point<float>(cx+std::sin(theta)*radius,cy-std::cos(theta)*radius);
  };

  // Deep, softly offset shadow (no bitmap allocations or DSP work).
  g.setColour(juce::Colours::black.withAlpha(.35f));
  g.fillEllipse(cx-body-3.f,cy-body+5.f,2.f*body+6.f,2.f*body+6.f);
  g.setColour(juce::Colour(0xff0a1018));
  g.fillEllipse(cx-ring-3.f,cy-ring-3.f,2.f*ring+6.f,2.f*ring+6.f);

  // Brushed circular bezel and restrained scale marks.
  g.setColour(juce::Colour(0xff52606c));
  g.drawEllipse(cx-ring-2.f,cy-ring-2.f,2.f*ring+4.f,2.f*ring+4.f,1.f);
  g.setColour(juce::Colour(0xff17202a));
  g.fillEllipse(cx-ring,cy-ring,2.f*ring,2.f*ring);
  for(int tick=0;tick<=10;++tick){
   const float theta=start+(end-start)*(tick/10.f);
   g.setColour(juce::Colour(tick%5==0?0xffbdc8d0:0xff657783));
   const auto a=polar(theta,ring*.91f);
   const auto b=polar(theta,ring*(tick%5==0?.78f:.83f));
   g.drawLine({a,b},tick%5==0?1.7f:1.f);
  }
  juce::Path background;
  background.addCentredArc(cx,cy,ring*.98f,ring*.98f,0,start,end,true);
  g.setColour(juce::Colour(0xff293640));
  g.strokePath(background,juce::PathStrokeType(4.f,juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
  juce::Path progress;
  progress.addCentredArc(cx,cy,ring*.98f,ring*.98f,0,start,angle,true);
  g.setColour(accent.withAlpha(slider.isEnabled()?1.f:.32f));
  g.strokePath(progress,juce::PathStrokeType(4.f,juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));
  // Machined 3D knob cap with a directional light source.
  g.setGradientFill(juce::ColourGradient(metal.brighter(.34f),cx-body,cy-body,
                                      juce::Colour(0xff161b24),cx+body,cy+body,false));
  g.fillEllipse(cx-body,cy-body,2.f*body,2.f*body);
  g.setColour(juce::Colour(0xffa3b2bf).withAlpha(.5f));
  g.drawEllipse(cx-body+1.f,cy-body+1.f,2.f*body-2.f,2.f*body-2.f,1.3f);
  const float inset=body*.79f;
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff303c49),cx-inset,cy-inset,
                                       juce::Colour(0xff111821),cx+inset,cy+inset,false));
  g.fillEllipse(cx-inset,cy-inset,2.f*inset,2.f*inset);
  g.setColour(juce::Colour(0xffd9e3ea).withAlpha(.22f));
  g.drawEllipse(cx-inset+1.f,cy-inset+1.f,2.f*inset-2.f,2.f*inset-2.f,1.f);

  // Pointer: two-stage illuminated machined index; no baked-in state.
  auto p1=polar(angle,inset*.26f),p2=polar(angle,inset*.81f);
  g.setColour(juce::Colours::black.withAlpha(.55f));
  g.drawLine({p1.translated(1.f,1.f),p2.translated(1.f,1.f)},4.5f);
  g.setColour(accent.brighter(.35f));
  g.drawLine({p1,p2},3.f);
  g.setColour(juce::Colour(0xffe9f5fa));
  g.fillEllipse(cx-2.2f,cy-2.2f,4.4f,4.4f);
  if(slider.hasKeyboardFocus(true)){
   g.setColour(accent.withAlpha(.7f));
   g.drawEllipse(cx-ring-5.f,cy-ring-5.f,2.f*ring+10.f,2.f*ring+10.f,1.4f);
  }
 }

};
