#include <juce_audio_processors/juce_audio_processors.h>
#include "RackState.h"
#include "../sdk/masterlabpro_input_compressor.h"
#include "../sdk/StereoArchitectSettings.h"
#include <numbers>
#include <string_view>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>
#include <memory>
int main(int argc,char** argv){
 if(argc<2||argc>3)return 1;
 const bool compressor=argc==3&&std::string_view(argv[2])=="compressor";
 const bool stereo=argc==3&&std::string_view(argv[2])=="stereo";
 if(argc==3&&!compressor&&!stereo)return 1;
 juce::ScopedJuceInitialiser_GUI gui;
 juce::AudioPluginFormatManager manager;
 auto* format=new juce::VST3PluginFormat();manager.addFormat(format);
 juce::OwnedArray<juce::PluginDescription> descriptions;format->findAllTypesForFile(descriptions,juce::String::fromUTF8(argv[1]));
 if(descriptions.size()!=1)return 2;
 std::vector<std::unique_ptr<juce::AudioPluginInstance>> plugins;
 constexpr int count=8,frames=256;
 for(int i=0;i<count;++i){
  juce::String error;auto plugin=manager.createPluginInstance(*descriptions[0],48000,frames,error);
  if(!plugin){std::cerr<<error.toStdString();return 3;}
  plugin->prepareToPlay(48000,frames);plugins.push_back(std::move(plugin));
 }
 // Restore a native Gain module in each binary instance; unique gain proves isolation.
 for(int i=0;i<count;++i){
  ml::RackState state;ml::SavedModule module;module.instance=1;module.typeHi=0x4d4c505445434800ull;module.typeLo=0x6761696e00000002ull;module.version=1;module.slot=0;module.lane=0;module.parameters.push_back({1,double(i)});
  if(compressor){module.typeHi=MLP_INPUT_COMPRESSOR_HI;module.typeLo=MLP_INPUT_COMPRESSOR_LO;}
  if(stereo){module.typeHi=ml::stereoTypeHi;module.typeLo=ml::stereoTypeLo;module.parameters.clear();for(uint32_t id=1;id<=7;++id)module.parameters.push_back({id,100});}
  state.modules.push_back(module);std::vector<uint8_t> bytes;if(!ml::RackStateCodec::encode(state,bytes))return 4;
  juce::XmlElement envelope("VST3PluginState");
  envelope.createNewChildElement("IComponent")->addTextElement(juce::MemoryBlock(bytes.data(),bytes.size()).toBase64Encoding());
  juce::MemoryBlock wrapped;juce::AudioProcessor::copyXmlToBinary(envelope,wrapped);
  plugins[size_t(i)]->setStateInformation(wrapped.getData(),int(wrapped.getSize()));
 }
 juce::AudioBuffer<float> audio(2,frames);juce::MidiBuffer midi;
 // The compressor ramps restored Input Gain with a 10 ms time constant.
 // Allow that ramp to settle before applying the existing 1e-4 sample tolerance.
 if(stereo){
  auto& plugin=*plugins[0];auto& params=plugin.getParameters();
  if(params.size()<14)return 7;
  auto measure=[&](double hz){
   double mid=0,side=0;
   for(int block=0;block<180;++block){
    for(int n=0;n<frames;++n){
     const double phase=2*std::numbers::pi*hz*(block*frames+n)/48000.;
     const float m=float(.1*std::sin(phase)),s=float(.15*std::cos(phase));
     audio.setSample(0,n,m+s);audio.setSample(1,n,m-s);
    }
    plugin.processBlock(audio,midi);
    if(block>=160)for(int n=0;n<frames;++n){
     const double m=(audio.getSample(0,n)+audio.getSample(1,n))*.5,s=(audio.getSample(0,n)-audio.getSample(1,n))*.5;
     mid+=m*m;side+=s*s;
    }
   }
   return std::array<double,2>{std::sqrt(mid),std::sqrt(side)};
  };
  constexpr std::array<double,7> tones{30,120,350,1000,2800,5600,12000};
  for(int band=0;band<7;++band){
   for(int i=0;i<7;++i)params[i]->setValueNotifyingHost(.5f);
   const auto neutral=measure(tones[size_t(band)]);
   params[band]->setValueNotifyingHost(0.f);const auto narrow=measure(tones[size_t(band)]);
   params[band]->setValueNotifyingHost(1.f);const auto wide=measure(tones[size_t(band)]);
   std::cout<<"Band "<<band<<" side ratio "<<narrow[1]/neutral[1]<<" / "<<wide[1]/neutral[1]<<std::endl;
   if(!(neutral[1]>0&&narrow[1]<neutral[1]*.9&&wide[1]>neutral[1]*1.1&&std::abs(wide[0]-neutral[0])<neutral[0]*.001))return 8;
  }
  for(int i=0;i<7;++i)params[i]->setValueNotifyingHost(0.f);
  if(measure(1000)[1]>1e-5)return 9;
  for(int i=0;i<7;++i)params[i]->setValueNotifyingHost(1.f);
  const auto wide=measure(1000);
  for(int i=0;i<7;++i)params[i]->setValueNotifyingHost(.5f);
  const auto neutral=measure(1000);
  if(std::abs(wide[1]/neutral[1]-2)>1e-4)return 10;
  for(auto& item:plugins)item->releaseResources();
  std::cout<<"PASS actual VST3 host automation to seven DSP bands"<<std::endl;return 0;
 }
 const int warmupBlocks=compressor?48:10;
 for(int block=0;block<warmupBlocks+2;++block)for(int i=0;i<count;++i){
  for(int ch=0;ch<2;++ch)for(int n=0;n<frames;++n)audio.setSample(ch,n,ch?-.1f:.1f);
  auto& plugin=*plugins[size_t(i)];plugin.processBlock(audio,midi);
  if(block>=warmupBlocks){const float expected=.1f*float(std::pow(10.,double(i)/20.));for(int n=0;n<frames;++n)if(std::abs(audio.getSample(0,n)-expected)>1e-4f||std::abs(audio.getSample(1,n)+expected)>1e-4f){std::cerr<<"instance "<<i<<" expected "<<expected<<" got "<<audio.getSample(0,n)<<'\n';return 5;}}
 }
 for(auto& plugin:plugins){juce::MemoryBlock state;plugin->getStateInformation(state);if(state.getSize()==0)return 6;plugin->releaseResources();}
 plugins.clear();std::cout<<"{\"loaded_vst3_instances\":8,\"module\":\""<<(compressor?"Input Compressor":"Gain")<<"\",\"gain_isolation\":true,\"state_save\":true}\n";return 0;
}
