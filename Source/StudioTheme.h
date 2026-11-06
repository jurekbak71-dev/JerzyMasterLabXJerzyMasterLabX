#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>
#include <ConceptCImages.h>

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
  // State comes from the real JUCE button / APVTS attachment; PNGs are decorative only.
  auto& lamp=b.hasKeyboardFocus(true)?blueOn_:(b.getToggleState()?greenOn_:greenOff_);
  if(!lamp.isNull()){
   const int side=juce::jmin(27,b.getHeight());
   g.setOpacity(b.isEnabled()?1.f:.3f);
   g.drawImage(lamp,0,(b.getHeight()-side)/2,side,side,0,0,lamp.getWidth(),lamp.getHeight());
   g.setOpacity(1.f);
  }
  g.setFont(font(12.f));g.setColour(juce::Colour(hover?0xff101c28:0xff25394c).withAlpha(b.isEnabled()?1.f:.5f));
  g.drawText(b.getButtonText(),32,0,b.getWidth()-32,b.getHeight(),juce::Justification::left);
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
 // Concept C rendered sprites: decoded once, shared by all APVTS-backed sliders.
 // The knob indicator is part of each immutable pre-rendered frame, not vector paint.
 juce::Image knob96_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::knob_96_atlas_png,MasterLabGuiAssets::knob_96_atlas_pngSize)};
 juce::Image knob160_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::knob_160_atlas_png,MasterLabGuiAssets::knob_160_atlas_pngSize)};
 juce::Image greenOn_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::led_green_on_png,MasterLabGuiAssets::led_green_on_pngSize)};
 juce::Image greenOff_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::led_green_off_png,MasterLabGuiAssets::led_green_off_pngSize)};
 juce::Image blueOn_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::led_blue_on_png,MasterLabGuiAssets::led_blue_on_pngSize)};
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,
                       float position,float,float,juce::Slider& slider) override {
  if(width<24||height<24)return;
  const auto side=static_cast<float>(std::min(width,height));
  const auto frame=juce::jlimit(0,100,juce::roundToInt(juce::jlimit(0.f,1.f,position)*100.f));
  const bool large=side>=124.f;
  const auto& atlas=large?knob160_:knob96_;
  const int sourceSide=large?160:96;
  if(atlas.isNull())return;
  // Reserve an edge for the separately rendered LED housing.
  const float shown=std::min(160.f,side*.92f);
  const float dx=x+(width-shown)*.5f;
  const float dy=y+(height-shown)*.5f;
  auto bounds=juce::Rectangle<float>(dx,dy,shown,shown);
  const auto& led=!slider.isEnabled()?greenOff_:
                  slider.hasKeyboardFocus(true)?blueOn_:greenOn_;
  if(!led.isNull()) {
   const auto margin=shown*.02f;
   auto ring=bounds.expanded(margin);
   g.setOpacity(slider.isEnabled()?0.90f:0.35f);
   g.drawImage(led,ring,juce::Rectangle<float>(0.f,0.f,
                 static_cast<float>(led.getWidth()),static_cast<float>(led.getHeight())));
   g.setOpacity(1.f);
  }
  const int sourceX=(frame%11)*sourceSide;
  const int sourceY=(frame/11)*sourceSide;
  g.setOpacity(slider.isEnabled()?1.f:0.4f);
  g.drawImage(atlas,juce::roundToInt(bounds.getX()),juce::roundToInt(bounds.getY()),
              juce::roundToInt(bounds.getWidth()),juce::roundToInt(bounds.getHeight()),
              sourceX,sourceY,sourceSide,sourceSide);
  g.setOpacity(1.f);
  if(slider.hasKeyboardFocus(true)){
   g.setColour(juce::Colour(0xff087bff).withAlpha(.8f));
   g.drawEllipse(bounds.expanded(1.5f),1.5f);
  }
 }

};
