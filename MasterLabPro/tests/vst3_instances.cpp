#include <juce_audio_processors/juce_audio_processors.h>
#include "RackState.h"
#include <array>
#include <cmath>
#include <iostream>
#include <vector>
#include <memory>
int main(int argc,char** argv){
 if(argc!=2)return 1;
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
  state.modules.push_back(module);std::vector<uint8_t> bytes;if(!ml::RackStateCodec::encode(state,bytes))return 4;
  plugins[size_t(i)]->setStateInformation(bytes.data(),int(bytes.size()));
 }
 juce::AudioBuffer<float> audio(2,frames);juce::MidiBuffer midi;
 for(int block=0;block<12;++block)for(int i=0;i<count;++i){
  for(int ch=0;ch<2;++ch)for(int n=0;n<frames;++n)audio.setSample(ch,n,ch?-.1f:.1f);
  auto& plugin=*plugins[size_t(i)];plugin.processBlock(audio,midi);
  if(block>=10){const float expected=.1f*float(std::pow(10.,double(i)/20.));for(int n=0;n<frames;++n)if(std::abs(audio.getSample(0,n)-expected)>1e-4f||std::abs(audio.getSample(1,n)+expected)>1e-4f){std::cerr<<"instance "<<i<<" expected "<<expected<<" got "<<audio.getSample(0,n);return 5;}}
 }
 for(auto& plugin:plugins){juce::MemoryBlock state;plugin->getStateInformation(state);if(state.getSize()==0)return 6;plugin->releaseResources();}
 plugins.clear();std::cout<<"{\"loaded_vst3_instances\":8,\"gain_isolation\":true,\"state_save\":true}\n";return 0;
}
