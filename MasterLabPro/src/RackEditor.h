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
  addAndMakeVisible(tiles);addAndMakeVisible(bands);
  bands.setEnabled(false);
  tiles.label=callbacks.label;
  tiles.onMove=[this](int a,int b){if(callbacks.move)callbacks.move(a,b);};
  tiles.onSelect=[this](RackOrder::Id id,int pos){
   selected=id;bands.setEnabled(id!=0);
   if(callbacks.select)callbacks.select(id,pos);repaint();
  };
  bands.onSelection=[this](int n){if(selected&&callbacks.band)callbacks.band(selected,n);};
  setSize(1600,1000);
 }
 void setSelection(RackOrder::Id id){selected=id;tiles.setSelection(id);bands.setEnabled(id!=0);repaint();}
 void setRackSkin(juce::Image skin){tiles.setSkin(std::move(skin));}
 void refreshRack(){tiles.repaintTiles();}
 void setBandFromController(int n){bands.setSelected(n);}
 void setGlobalLevels(StereoLevels in,StereoLevels out){input.setLevels(in);output.setLevels(out);}
 void setSpectrum(std::array<std::vector<SpectrumBin>,3> measured,float maxHz){spectrum.setTraces(std::move(measured),maxHz);}
 void setHeatmaps(juce::Image before,juce::Image after,juce::Image delta){heatmap.setImages(std::move(before),std::move(after),std::move(delta));}
 // Editor ownership is transferred; host binds real parameters before calling.
 void setModuleEditor(std::unique_ptr<juce::Component> next){
  if(module)removeChildComponent(module.get());module=std::move(next);
  if(module)addAndMakeVisible(*module);resized();
 }
 void resized()override{
  auto a=box(1);a.removeFromTop(24);spectrum.setBounds(a.reduced(4));
  a=box(2);a.removeFromTop(24);heatmap.setBounds(a.reduced(4));
  a=box(3);a.removeFromTop(24);input.setBounds(a.reduced(4));
  a=box(4);a.removeFromTop(24);output.setBounds(a.reduced(4));
  a=box(5);a.removeFromTop(24);tiles.setBounds(a.reduced(4));
  bands.setBounds(box(8));
  if(module){a=box(6);a.removeFromTop(76*getHeight()/1000);module->setBounds(a.reduced(8));}
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff171b21));
  for(int i=0;i<8;++i){auto r=box(i);if(panelSkin.isValid())g.drawImage(panelSkin,r.toFloat());else{g.setColour(juce::Colour(0xff272d34));g.fillRoundedRectangle(r.toFloat(),5);g.setColour(juce::Colour(0xff65717c));g.drawRoundedRectangle(r.toFloat(),5,1);}}
  const char* names[]{"JERZY MASTERLAB PRO","SPECTRAL MONITOR","SPECTRAL HEAT MAP","INPUT","OUTPUT","SIGNAL ROUTING","MODULE EDITOR","DEVELOPMENT / TELEMETRY NOT CONNECTED"};
  g.setColour(juce::Colours::whitesmoke);g.setFont(14.f);
  for(int i=0;i<8;++i)g.drawText(names[i],box(i).removeFromTop(24).reduced(8,0),juce::Justification::centredLeft,true);
  if(!module){auto r=box(6);r.removeFromTop(76*getHeight()/1000);g.drawText(selected?"MODULE EDITOR NOT CONNECTED":"SELECT A MODULE",r,juce::Justification::centred);}
 }
private:
 juce::Rectangle<int> box(int i)const{auto r=scale(rackLayout[static_cast<std::size_t>(i)],getWidth(),getHeight());return {r.x,r.y,r.w,r.h};}
 Callbacks callbacks;RackTiles tiles;FrequencyBandSelector bands;SpectrumView spectrum;HeatMapView heatmap;StereoLevelMeter input,output;
 juce::Image panelSkin;RackOrder::Id selected=0;std::unique_ptr<juce::Component> module;
};
}
