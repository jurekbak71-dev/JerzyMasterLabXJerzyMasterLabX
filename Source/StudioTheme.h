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
  const auto& face=(active||primary)?switchOn_:switchOff_;
  if(!face.isNull()){
   g.setOpacity(button.isEnabled()?1.f:.36f);
   g.drawImage(face,r);
   g.setOpacity(1.f);
  }
  else {g.setColour(juce::Colour(0xff152c3f));g.fillRoundedRectangle(r,5.f);}
  if(hover||down){g.setColour(juce::Colour(0xff16c8ff).withAlpha(down?.45f:.23f));g.drawRoundedRectangle(r.reduced(1.f),5.f,1.5f);}
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
 juce::Image redOn_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::led_red_on_png,MasterLabGuiAssets::led_red_on_pngSize)};
 juce::Image redOff_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::led_red_off_png,MasterLabGuiAssets::led_red_off_pngSize)};
 juce::Image grBezel_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::analog_gr_bezel_png,MasterLabGuiAssets::analog_gr_bezel_pngSize)};
 void drawAnalogGrBezel(juce::Graphics& g,juce::Rectangle<float> rect) const{
  if(!grBezel_.isNull())
   g.drawImage(grBezel_,rect);
  else{
   g.setColour(juce::Colour(0xffaab5bc));g.fillRoundedRectangle(rect,12.f);
   g.setColour(juce::Colour(0xffe8d7b6));g.fillRoundedRectangle(rect.reduced(13.f),8.f);
  }
 }
 juce::Image powderTile_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::powdercoat_tile_png,MasterLabGuiAssets::powdercoat_tile_pngSize)};
 juce::Image ledRing96_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::led_wreath_96_atlas_png,MasterLabGuiAssets::led_wreath_96_atlas_pngSize)};
 juce::Image ledRing160_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::led_wreath_160_atlas_png,MasterLabGuiAssets::led_wreath_160_atlas_pngSize)};
 juce::Image glassScreen_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::glass_analyzer_png,MasterLabGuiAssets::glass_analyzer_pngSize)};
 juce::Image glassReflection_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::glass_overlay_png,MasterLabGuiAssets::glass_overlay_pngSize)};
 juce::Image glassReadout_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::glass_readout_png,MasterLabGuiAssets::glass_readout_pngSize)};
 juce::Image moduleGlowOn_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::module_glow_on_png,MasterLabGuiAssets::module_glow_on_pngSize)};
 juce::Image moduleGlowOff_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::module_glow_off_png,MasterLabGuiAssets::module_glow_off_pngSize)};
 juce::Image switchOn_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::switch_on_png,MasterLabGuiAssets::switch_on_pngSize)};
 juce::Image switchOff_{juce::ImageCache::getFromMemory(
     MasterLabGuiAssets::switch_off_png,MasterLabGuiAssets::switch_off_pngSize)};
 void paintGlassScreen(juce::Graphics& g,juce::Rectangle<float> rect)const{
  if(!glassScreen_.isNull())g.drawImage(glassScreen_,rect);
 }
 void paintGlassReflection(juce::Graphics& g,juce::Rectangle<float> rect)const{
  if(!glassReflection_.isNull()){g.setOpacity(.45f);g.drawImage(glassReflection_,rect);g.setOpacity(1.f);}
 }
 void paintModuleGlow(juce::Graphics& g,juce::Rectangle<float> rect,bool active)const{
  const auto& img=active?moduleGlowOn_:moduleGlowOff_;
  if(!img.isNull()){g.setOpacity(active?.85f:.25f);g.drawImage(img,rect);g.setOpacity(1.f);}
 }
 void paintSmallRedLed(juce::Graphics& g,int x,int y,int size,bool active)const{
  const auto& img=active?redOn_:redOff_;
  if(!img.isNull())g.drawImage(img,x,y,size,size,0,0,img.getWidth(),img.getHeight());
 }

 void paintPowderCoat(juce::Graphics& g,juce::Rectangle<int> area)const{
  g.setColour(juce::Colour(0xffedf0f2));g.fillRect(area);
  if(!powderTile_.isNull()){
   juce::Graphics::ScopedSaveState save(g);
   g.reduceClipRegion(area);
   g.setTiledImageFill(powderTile_,area.getX(),area.getY(),1.f);
   g.fillRect(area);
  }
 }
 void paintStatusLed(juce::Graphics& g,int x,int y,int size,bool warning)const{
  const auto& sprite=warning?redOn_:greenOn_;
  if(!sprite.isNull())
   g.drawImage(sprite,x,y,size,size,0,0,sprite.getWidth(),sprite.getHeight());
 }
 void paintPowerLed(juce::Graphics& g,int x,int y,int size,bool enabled)const{
  const auto& sprite=enabled?greenOn_:greenOff_;
  if(!sprite.isNull())
   g.drawImage(sprite,x,y,size,size,0,0,sprite.getWidth(),sprite.getHeight());
 }
 void drawLinearSlider(juce::Graphics& g,int x,int y,int width,int height,
                       float sliderPos,float minSliderPos,float maxSliderPos,
                       juce::Slider::SliderStyle style,juce::Slider& slider) override {
  const bool horizontal=style==juce::Slider::LinearHorizontal;
  const bool vertical=style==juce::Slider::LinearVertical;
  if(!horizontal&&!vertical){
   juce::LookAndFeel_V4::drawLinearSlider(g,x,y,width,height,sliderPos,minSliderPos,maxSliderPos,style,slider);
   return;
  }
  const float centreX=static_cast<float>(x)+width*.5f,centreY=static_cast<float>(y)+height*.5f;
  const float trackWidth=horizontal?static_cast<float>(width-22):6.f;
  const float trackHeight=vertical?static_cast<float>(height-22):6.f;
  const auto track=juce::Rectangle<float>(horizontal?x+11.f:centreX-3.f,
                           vertical?y+11.f:centreY-3.f,trackWidth,trackHeight);
  g.setColour(juce::Colour(0xff07141e));g.fillRoundedRectangle(track,3.f);
  g.setColour(juce::Colour(0xff5c7184));g.drawRoundedRectangle(track,3.f,.8f);
  const float position=horizontal?juce::jlimit(track.getX(),track.getRight(),sliderPos)
                                 :juce::jlimit(track.getY(),track.getBottom(),sliderPos);
  const auto lit=juce::Colour(slider.isEnabled()?0xff067bff:0xff546675);
  g.setColour(lit);
  if(horizontal)g.fillRoundedRectangle(track.withWidth(juce::jmax(1.f,position-track.getX())),3.f);
  else g.fillRoundedRectangle(track.withTop(position),3.f);
  const int cap=juce::jmin(34,horizontal?height:width);
  const int px=juce::roundToInt(horizontal?position-cap*.5f:centreX-cap*.5f);
  const int py=juce::roundToInt(vertical?position-cap*.5f:centreY-cap*.5f);
  if(!knob96_.isNull())g.drawImage(knob96_,px,py,cap,cap,6*96,4*96,96,96);
  if(slider.hasKeyboardFocus(true)){
   g.setColour(juce::Colour(0xff0c8bff));
   g.drawEllipse(static_cast<float>(px-1),static_cast<float>(py-1),
                 static_cast<float>(cap+2),static_cast<float>(cap+2),1.f);
  }
 }
 void drawLabel(juce::Graphics& g,juce::Label& label) override{
  if(dynamic_cast<juce::Slider*>(label.getParentComponent())!=nullptr){
   const auto area=label.getLocalBounds().toFloat();
   if(!glassReadout_.isNull())g.drawImage(glassReadout_,area);
   else {g.setColour(juce::Colour(0xff0a1e31));g.fillRoundedRectangle(area,4.f);}
   g.setColour(label.isEnabled()?juce::Colour(0xffb5f1ff):juce::Colour(0xff607887));
   g.setFont(font(juce::jmin(14.f,area.getHeight()*.65f)));
   g.drawFittedText(label.getText(),label.getLocalBounds().reduced(3,0),label.getJustificationType(),1);
   return;
  }
  juce::LookAndFeel_V4::drawLabel(g,label);
 }
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
   g.drawImage(led,juce::roundToInt(ring.getX()),juce::roundToInt(ring.getY()),
               juce::roundToInt(ring.getWidth()),juce::roundToInt(ring.getHeight()),
               0,0,led.getWidth(),led.getHeight());
   g.setOpacity(1.f);
  }
  const int sourceX=(frame%11)*sourceSide;
  const int sourceY=(frame/11)*sourceSide;
  g.setOpacity(slider.isEnabled()?1.f:0.4f);
  g.drawImage(atlas,juce::roundToInt(bounds.getX()),juce::roundToInt(bounds.getY()),
              juce::roundToInt(bounds.getWidth()),juce::roundToInt(bounds.getHeight()),
              sourceX,sourceY,sourceSide,sourceSide);
  g.setOpacity(1.f);
  // Segmented electric-blue ring is pre-rendered RGBA, not an animated fake light.
  const auto& wreath=large?ledRing160_:ledRing96_;
  if(!wreath.isNull()){
   // Same normalized frame as the knob: lit prefix is the physical value.
   // Inactive controls keep their unlit ring, never a misleading active glow.
   const int arcFrame=slider.isEnabled()?frame:0;
   const int arcX=(arcFrame%11)*sourceSide,arcY=(arcFrame/11)*sourceSide;
   g.setOpacity(slider.isEnabled()?1.f:.48f);
   g.drawImage(wreath,juce::roundToInt(bounds.getX()),juce::roundToInt(bounds.getY()),
               juce::roundToInt(bounds.getWidth()),juce::roundToInt(bounds.getHeight()),
               arcX,arcY,sourceSide,sourceSide);
   g.setOpacity(1.f);
  }
  if(slider.hasKeyboardFocus(true)){
   g.setColour(juce::Colour(0xff087bff).withAlpha(.8f));
   g.drawEllipse(bounds.expanded(1.5f),1.5f);
  }
 }

};
