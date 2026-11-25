#include "RackEditor.h"
#include "AssetLoader.h"
#include "RotaryParameterControl.h"
#include "AudioRack.h"
#include "AnalysisHistory.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
#include <memory>
#include <numbers>
// Smoke test of actual JUCE painting; strict mode uses a real gain parameter, no fake telemetry.
// Optional explicit asset root makes missing or invalid skins a test failure.
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 ml::RackOrder order;
 ml::FrequencyBandSelector::Skins skins;
 juce::Image panel,slot,cap,pointer;
 const bool strict=argc>2;
 const juce::String scenario=argc>3?juce::String::fromUTF8(argv[3]):"gain";
 const float scale=argc>4?juce::String::fromUTF8(argv[4]).getFloatValue():1.f;
 if(scale<1.f||scale>2.f)return 10;
 if(strict){
  ml::AssetLoader assets(juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[2])));
  skins=assets.loadBands();panel=assets.loadPanel();slot=assets.loadSlot();cap=assets.loadKnobCap();pointer=assets.loadKnobPointer();
  if(!assets.missingFiles().isEmpty()){
   std::cerr<<assets.missingFiles().joinIntoString(", ").toStdString()<<"\n";return 4;
  }
  if(slot.getWidth()!=356||slot.getHeight()!=58)return 5;
  if(cap.getWidth()!=68||cap.getHeight()!=69||pointer.getWidth()!=68||pointer.getHeight()!=68)return 6;
 }
 juce::AudioParameterFloat gain(juce::ParameterID{"gain_test",1},"Gain",juce::NormalisableRange<float>(-24.f,24.f,.01f),0.f);
 ml::RackEditor::Callbacks callbacks;callbacks.label=[](ml::RackOrder::Id){return juce::String("GAIN [TEST]");};
 ml::RackEditor editor(order,std::move(skins),std::move(callbacks),panel);
 if(strict)editor.setRackSkin(slot);
 editor.setSize(1600,1000);
 if(strict){
  const int count=scenario=="empty"?0:scenario=="ten"?10:1;
  for(int i=0;i<count;++i)if(!order.insert(static_cast<std::size_t>(i),static_cast<ml::RackOrder::Id>(i+1)))return 7;
  if(count){
   gain.setValueNotifyingHost(gain.convertTo0to1(6.f));
   editor.setSelection(1);editor.setBandFromController(7);editor.setBandSelectionEnabled(false);
   editor.setModuleEditor(std::make_unique<ml::RotaryParameterControl>(gain,0.f,cap,pointer,juce::Colour(0xff58bde8)));
  }
  editor.refreshRack();
  if(scenario.startsWith("signal")){
   auto rack=std::make_unique<ml::AudioRack>();rack->prepare(48000.);ml::RackConfig config;config.stages[0]={1,6.0205999f};
   config.deltaId=1;config.moduleAnalysis=scenario.contains("module");config.listenDelta=scenario.contains("delta");
   if(!rack->submit(config))return 8;
   auto history=std::make_unique<ml::AnalysisHistory>();ml::AnalysisFrame frame;ml::RackMeters levels,latest;bool gotLevels=false;
   std::array<float,257> left{},right{};std::uint64_t sample=0;
   for(int block=0;block<40;++block){
    for(std::size_t i=0;i<left.size();++i,++sample){const float x=.18f*std::sin(float(2*std::numbers::pi*997.*sample/48000.));left[i]=x;right[i]=x*.72f;}
    rack->process(left.data(),right.data(),left.size());
    while(rack->pollAnalysis(frame))history->accept(frame);
    while(rack->poll(levels)){latest=levels;gotLevels=true;}
   }
   if(!gotLevels)return 9;
   editor.setGlobalLevels(latest.input,latest.output);
   std::array<std::vector<ml::SpectrumBin>,3> traces;
   for(int t=0;t<3;++t)for(std::size_t k=1;k<ml::fftBins;++k){const auto hz=float(k*48000./ml::fftSize);if(hz>=20&&hz<=20000)traces[t].push_back({hz,history->spectra[t][k]});}
   editor.setSpectrum(std::move(traces),20000.f);
   std::array<juce::Image,3> images;
   const juce::Colour colours[]{juce::Colour(0xff58bde8),juce::Colour(0xffe3ad60),juce::Colour(0xffcb9cea)};
   for(int t=0;t<3;++t){
    images[t]=juce::Image(juce::Image::RGB,ml::AnalysisHistory::width,ml::AnalysisHistory::height,true);
    juce::Image::BitmapData pixels(images[t],juce::Image::BitmapData::writeOnly);
    for(int x=0;x<ml::AnalysisHistory::width;++x){const auto& col=history->column(x);for(int y=0;y<ml::AnalysisHistory::height;++y){
     auto colour=juce::Colour(0xff09151b);if(col.valid){const float v=col.values[t][y];colour=colour.interpolatedWith(t==2&&v<0?juce::Colour(0xff58bde8):colours[t],t==2?std::clamp(std::abs(v)/24.f,0.f,1.f):std::clamp((v+100.f)/100.f,0.f,1.f));}pixels.setPixelColour(x,y,colour);
    }}
   }
   editor.setHeatmaps(images[0],images[1],images[2]);
   editor.setAnalysisMode(scenario.contains("module")?"MODULE / ID 1":scenario.contains("delta")?"GLOBAL / DELTA LISTEN":"GLOBAL");
  }else editor.setAnalysisMode(scenario.toUpperCase());
 }
 auto img=editor.createComponentSnapshot(editor.getLocalBounds(),true,scale);
 if(!img.isValid()||img.getWidth()!=juce::roundToInt(1600*scale)||img.getHeight()!=juce::roundToInt(1000*scale))return 1;
 if(argc>1){
  auto file=juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[1]));
  if(!file.deleteFile())return 2;
  juce::FileOutputStream stream(file);
  juce::PNGImageFormat png;
  if(!stream.openedOk()||!png.writeImageToStream(img,stream))return 3;
 }
 std::cout<<"PASS: offscreen rack render ("<<(strict?"approved assets":"fallback skin")<<", "<<scenario<<", scale "<<scale<<")\n";
 return 0;
}
