#pragma once
#include "MeasurementModel.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
namespace ml {
// All setters called on GUI thread, with measured data from a separate producer.
class StereoLevelMeter final:public juce::Component {
 StereoLevels levels;
public:void setLevels(StereoLevels l){levels=l;repaint();}
 void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xff10161b));for(int c=0;c<2;++c){float w=getWidth()/2.f;auto r=juce::Rectangle<float>(c*w+5,22,w-10,float(getHeight()-44));if(r.getHeight()<=0||r.getWidth()<=0)continue;for(int i=0;i<60;++i){float db=-60.f+i;bool lit=levels.peak[c]>=db;g.setColour(lit?(db>=-3?juce::Colours::red:db>=-12?juce::Colours::yellow:juce::Colours::limegreen):juce::Colour(0xff293036));float h=r.getHeight()/60;g.fillRect(r.getX(),r.getBottom()-(i+1)*h,r.getWidth(),std::max(0.f,h-1));}g.setColour(levels.clip[c]?juce::Colours::red:juce::Colours::grey);g.drawText("CLIP",int(c*w),0,int(w),20,juce::Justification::centred);g.setColour(juce::Colours::white);g.drawText(c?"R":"L",int(c*w),getHeight()-20,int(w),20,juce::Justification::centred);}}
};
struct SpectrumBin {float hz,db;};
class SpectrumView final:public juce::Component {
 std::array<std::vector<SpectrumBin>,3> traces;float nyquist=20000;
public:void setTraces(std::array<std::vector<SpectrumBin>,3> measured,float maxHz){traces=std::move(measured);nyquist=maxHz;repaint();}
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff09151b));
  auto r=getLocalBounds().toFloat().reduced(12,3);
  auto legend=r.removeFromTop(14);r.removeFromBottom(14);r.removeFromLeft(29);
  if(r.isEmpty()||nyquist<=20)return;
  const juce::Colour grid(0xff23333d),label(0xff9cacb5);
  g.setFont(juce::FontOptions(10.f));
  for(float f:{20.f,100.f,500.f,2000.f,10000.f,20000.f})if(f<=nyquist){
   const float x=r.getX()+r.getWidth()*frequencyX(f,nyquist);
   g.setColour(grid);g.drawVerticalLine(int(x),r.getY(),r.getBottom());
   const auto text=f>=1000?juce::String(f/1000,0)+"k":juce::String(f,0);
   g.setColour(label);g.drawText(text,juce::Rectangle<int>(int(std::min(x-18,r.getRight()-36)),int(r.getBottom()),36,13),juce::Justification::centred);
  }
  for(int d:{-90,-60,-30,0}){
   const float y=r.getY()+r.getHeight()*dbY(float(d));
   g.setColour(grid);g.drawHorizontalLine(int(y),r.getX(),r.getRight());
   g.setColour(label);g.drawText(juce::String(d),int(r.getX())-29,int(y)-6,24,12,juce::Justification::centredRight);
  }
  const juce::Colour colors[]{juce::Colour(0xff58bde8),juce::Colour(0xffe3ad60),juce::Colour(0xffcb9cea)};
  const char* names[]{"BEFORE","AFTER","DELTA"};
  bool any=false;
  for(int c=0;c<3;++c){
   g.setColour(colors[c]);g.drawText(names[c],legend.removeFromLeft(58).toNearestInt(),juce::Justification::centred);
   juce::Path path;bool first=true;
   for(auto b:traces[c]){
    if(!std::isfinite(b.hz)||!std::isfinite(b.db)||b.hz<20||b.hz>nyquist)continue;
    const auto point=juce::Point<float>{r.getX()+r.getWidth()*frequencyX(b.hz,nyquist),r.getY()+r.getHeight()*dbY(b.db)};
    if(first){path.startNewSubPath(point);first=false;}else path.lineTo(point);
   }
   if(!first){any=true;g.strokePath(path,juce::PathStrokeType(1.8f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));}
  }
  if(!any){g.setColour(label);g.drawText("NO SIGNAL",r.toNearestInt(),juce::Justification::centred);}
 }
};
class HeatMapView final:public juce::Component {
 juce::Image before,after,delta;
public:void setImages(juce::Image b,juce::Image a,juce::Image d){before=std::move(b);after=std::move(a);delta=std::move(d);repaint();}
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff09151b));
  const juce::Image images[]{before,after,delta};
  const char* labels[]{"BEFORE","AFTER","DELTA AUDIO"};
  auto area=getLocalBounds().reduced(5,2);
  auto axis=area.removeFromBottom(13).withTrimmedLeft(101);
  g.setFont(juce::FontOptions(10.f));
  for(int i=0;i<3;++i){
   auto row=area.removeFromTop(area.getHeight()/(3-i)).reduced(0,1);
   auto name=row.removeFromLeft(101);
   g.setColour(juce::Colours::whitesmoke);g.drawFittedText(labels[i],name.reduced(4,0),juce::Justification::centredLeft,1);
   if(images[i].isValid())g.drawImage(images[i],row.toFloat());
   else{g.setColour(juce::Colour(0xff15242b));g.fillRect(row);g.setColour(juce::Colours::grey);g.drawText("NO SIGNAL",row,juce::Justification::centred);}
  }
  g.setColour(juce::Colour(0xff9cacb5));
  for(const auto hz:{20.f,100.f,500.f,2000.f,10000.f,20000.f}){
   const int x=axis.getX()+int(frequencyX(hz)*float(std::max(0,axis.getWidth()-38)));
   g.drawText(hz>=1000?juce::String(hz/1000,0)+"k":juce::String(hz,0),x,axis.getY(),38,13,juce::Justification::centredLeft);
  }
 }
};}
