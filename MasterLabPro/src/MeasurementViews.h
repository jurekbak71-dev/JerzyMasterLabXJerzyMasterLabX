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
 void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xff09151b));auto r=getLocalBounds().toFloat().reduced(8);if(r.isEmpty())return;g.setColour(juce::Colour(0xff29424c));for(float f:{20.f,50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f,20000.f})if(f<=nyquist){float x=r.getX()+r.getWidth()*frequencyX(f,nyquist);g.drawVerticalLine(int(x),r.getY(),r.getBottom());}for(int d=-90;d<=12;d+=12){float y=r.getY()+r.getHeight()*dbY(float(d));g.drawHorizontalLine(int(y),r.getX(),r.getRight());}juce::Colour colors[]{juce::Colour(0xff58bde8),juce::Colour(0xffe3ad60),juce::Colour(0xffcb9cea)};bool any=false;for(int c=0;c<3;++c){juce::Path p;bool first=true;for(auto b:traces[c]){if(!std::isfinite(b.hz)||!std::isfinite(b.db)||b.hz<20||b.hz>nyquist)continue;float x=r.getX()+r.getWidth()*frequencyX(b.hz,nyquist),y=r.getY()+r.getHeight()*dbY(b.db);if(first){p.startNewSubPath(x,y);first=false;}else p.lineTo(x,y);}if(!first){any=true;g.setColour(colors[c]);g.strokePath(p,juce::PathStrokeType(1.5f));}}if(!any){g.setColour(juce::Colours::grey);g.drawText("NO SIGNAL",getLocalBounds(),juce::Justification::centred);}}
};
class HeatMapView final:public juce::Component {
 juce::Image before,after,delta;
public:void setImages(juce::Image b,juce::Image a,juce::Image d){before=std::move(b);after=std::move(a);delta=std::move(d);repaint();}
 void paint(juce::Graphics& g)override{g.fillAll(juce::Colour(0xff09151b));juce::Image images[]{before,after,delta};const char* labels[]{"BEFORE","AFTER","LEVEL DELTA dB"};for(int i=0;i<3;++i){auto r=juce::Rectangle<int>(0,i*getHeight()/3,getWidth(),(i+1)*getHeight()/3-i*getHeight()/3).reduced(3);auto label=r.removeFromLeft(110);g.setColour(juce::Colours::white);g.drawText(labels[i],label,juce::Justification::centred);if(images[i].isValid())g.drawImage(images[i],r.toFloat());else{g.setColour(juce::Colour(0xff15242b));g.fillRect(r);g.setColour(juce::Colours::grey);g.drawText("NO SIGNAL",r,juce::Justification::centred);}}}
};}
