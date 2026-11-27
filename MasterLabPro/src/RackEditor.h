#pragma once
#include "RackLayout.h"
#include "RackTiles.h"
#include "FrequencyBandSelector.h"
#include "MeasurementViews.h"
#include <functional>
namespace ml {
// Message-thread integration surface. Controller and model must outlive editor.
// No DSP mutation, fabricated telemetry, or module instances created here.
class RackEditor final : public juce::Component {
public:
 struct Callbacks {
  std::function<void(int,int)> move;
  std::function<void(RackOrder::Id,int)> select;
  std::function<juce::String(RackOrder::Id)> label;
  std::function<void(RackOrder::Id,int)> band;
 };
 RackEditor(const RackOrder& order,FrequencyBandSelector::Skins skins,Callbacks cb,
            juce::Image panel = {}): callbacks(std::move(cb)),tiles(order),bands(std::move(skins)),panelSkin(std::move(panel)) {
  addAndMakeVisible(spectrum);addAndMakeVisible(heatmap);
  addAndMakeVisible(input);addAndMakeVisible(output);
  addAndMakeVisible(tiles);addChildComponent(bands);
  bands.setEnabled(false);
  tiles.label=callbacks.label;
  tiles.onMove=[this](int a,int b){if(callbacks.move)callbacks.move(a,b);};
  tiles.onSelect=[this](RackOrder::Id id,int pos){
   selected=id;bands.setEnabled(id!=0);
   if(callbacks.select)callbacks.select(id,pos);
   repaint();
  };
  bands.onSelection=[this](int n){if(selected&&callbacks.band)callbacks.band(selected,n);};
  setSize(1600,1000);
 }
 void setSelection(RackOrder::Id id){selected=id;tiles.setSelection(id);bands.setEnabled(id!=0);repaint();}
 void setRackSkin(juce::Image skin){tiles.setSkin(std::move(skin));}
 void refreshRack(){tiles.repaintTiles();}
 void setBandSelectionEnabled(bool enabled){bands.setEnabled(enabled && selected!=0);}
 void setBandFromController(int n){bands.setSelected(n);}
 void setGlobalLevels(StereoLevels in,StereoLevels out){input.setLevels(in);output.setLevels(out);}
 void setSpectrum(std::array<std::vector<SpectrumBin>,3> measured,float maxHz){spectrum.setTraces(std::move(measured),maxHz);}
 void setHeatmaps(juce::Image before,juce::Image after,juce::Image delta){heatmap.setImages(std::move(before),std::move(after),std::move(delta));}
 void setAnalysisMode(juce::String mode){analysisMode=std::move(mode);repaint();}
 void setFooter(juce::String text){footer=std::move(text);repaint();}
 // Editor ownership is transferred; host binds real parameters before calling.
 void setModuleEditor(std::unique_ptr<juce::Component> next,bool ownsBandArea=false){
  if(module)removeChildComponent(module.get());
  module=std::move(next);
  moduleOwnsBandArea=module&&ownsBandArea;
  bands.setVisible(false);
  if(module)addAndMakeVisible(*module);
  resized();
 }
 void resized()override{
  auto interior=[this](int i){auto a=box(i).reduced(22,10);a.removeFromTop(27);a=a.reduced(2,1);return a;};
  spectrum.setBounds(interior(1));heatmap.setBounds(interior(2));
  input.setBounds(interior(3));output.setBounds(interior(4));
  auto slots=box(5).reduced(22,8);slots.removeFromTop(27);tiles.setBounds(slots);bands.setBounds({});
  if(module)module->setBounds(interior(6));
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff171b21));
  for(int i=0;i<8;++i){auto r=box(i);if(panelSkin.isValid()&&panelSkin.getWidth()==406&&panelSkin.getHeight()==178){
    for(const auto& slice:nineSlice({0,0,406,178},{r.getX(),r.getY(),r.getWidth(),r.getHeight()},36,30,36,30))
     if(slice.source.w>0&&slice.source.h>0&&slice.target.w>0&&slice.target.h>0)
      g.drawImage(panelSkin,slice.target.x,slice.target.y,slice.target.w,slice.target.h,
                  slice.source.x,slice.source.y,slice.source.w,slice.source.h);
   }else{g.setColour(juce::Colour(0xff272d34));g.fillRoundedRectangle(r.toFloat(),5);g.setColour(juce::Colour(0xff65717c));g.drawRoundedRectangle(r.toFloat(),5,1);}}
  const char* names[]{"JERZY MASTERLAB PRO","SPECTRAL MONITOR","SPECTRAL HEAT MAP","INPUT","OUTPUT","SIGNAL ROUTING","MODULE EDITOR"};
  g.setColour(juce::Colours::whitesmoke);g.setFont(14.f);
  for(int i=0;i<7;++i){
   auto header=box(i).reduced(24,9).removeFromTop(23);
   if(i==0)header=header.withWidth(header.getWidth()*40/100);
   if(header.isEmpty())continue;
   g.setColour(juce::Colour(0xff11181e));g.fillRoundedRectangle(header.toFloat(),2);
   g.setColour(juce::Colour(0xff47515a));g.drawRoundedRectangle(header.toFloat(),2,1);
   g.setColour(juce::Colour(0xffc4cbd0));g.setFont(juce::FontOptions(12.f,juce::Font::bold));
   if(i==0){auto brand=header.reduced(8,1);auto pro=brand.removeFromRight(45);g.setFont(juce::FontOptions(17.f,juce::Font::bold|juce::Font::italic));g.drawFittedText("JERZY MASTERLAB",brand,juce::Justification::centredLeft,1,.65f);g.setColour(juce::Colour(0xffff5344));g.drawFittedText("PRO",pro,juce::Justification::centred,1,.75f);}else g.drawFittedText(names[i],header.reduced(8,2),juce::Justification::centredLeft,1,.75f);
  }
  g.setColour(juce::Colour(0xffaeb9c1));g.setFont(juce::FontOptions(10.f));
  g.drawFittedText(footer,box(7).reduced(40,5),juce::Justification::centredLeft,1,.72f);
  if(analysisMode.isNotEmpty())g.drawText(analysisMode,box(0).reduced(36,10),juce::Justification::centredRight,true);
  if(!module){auto r=box(6);r.removeFromTop(76*getHeight()/1000);g.drawText(selected?"MODULE EDITOR NOT CONNECTED":"SELECT A MODULE",r,juce::Justification::centred);}
 }
private:
 juce::Rectangle<int> box(int i)const{
  auto raw=rackLayout[static_cast<std::size_t>(i)];
  if(i==1)raw={12,72,619,270};
  if(i==2)raw={641,72,619,270};
  if(i==3||i==4)raw.h=270;
  if(i==5)raw={12,352,1576,146};
  if(i==6)raw={12,508,1576,442};
  const auto r=scale(raw,getWidth(),getHeight());return {r.x,r.y,r.w,r.h};
 }
 Callbacks callbacks;RackTiles tiles;FrequencyBandSelector bands;SpectrumView spectrum;HeatMapView heatmap;StereoLevelMeter input,output;
 juce::Image panelSkin;juce::String analysisMode,footer{"DEVELOPMENT / GAIN TEST RACK"};RackOrder::Id selected=0;std::unique_ptr<juce::Component> module;bool moduleOwnsBandArea=false;
};
}
