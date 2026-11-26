#include <juce_audio_utils/juce_audio_utils.h>
#include "ModuleRackHost.h"
#include "ParameterRegistry.h"
#include <array>
#include <atomic>
#include <cmath>
#include <limits>

namespace ml {
// Initial VST3 integration: fixed normalized automation lanes, no dynamic host parameter IDs.
class PluginProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
 PluginProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){
  for(unsigned i=0;i<params.size();++i){
   auto* p=new juce::AudioParameterFloat(juce::ParameterID{"lane_"+juce::String(i),1},
      "Lane "+juce::String(i/64+1)+" Parameter "+juce::String(i%64+1),0.f,1.f,0.f);
   addParameter(p);params[i]=p;
  }
  const auto root=juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory).getChildFile("Jerzy").getChildFile("MasterLabPro").getChildFile("modules");
  host->scan(std::filesystem::path(root.getFullPathName().toWideCharPointer()));
  startTimerHz(20);
 }
 ~PluginProcessor() override {stopTimer();}
 const juce::String getName() const override{return "Jerzy MasterLabPro";}
 bool acceptsMidi() const override{return false;}
 bool producesMidi() const override{return false;}
 bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return 0.0;}
 int getNumPrograms() override{return 1;}
 int getCurrentProgram() override{return 0;}
 void setCurrentProgram(int) override{}
 const juce::String getProgramName(int) override{return {};}
 void changeProgramName(int,const juce::String&) override{}
 bool isBusesLayoutSupported(const BusesLayout& layout) const override{
  return layout.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
 }
 void prepareToPlay(double rate,int block) override{
  host->resetGraphWhenStopped();host->prepare(rate,uint32_t(std::max(1,block)));host->submit(order);
 }
 void releaseResources() override{}
 void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override{
  juce::ScopedNoDenormals noDenormals;
  // Mapping and topology changes are serialized with JUCE's callback lock off the audio thread.
  for(const auto id:order.slots())if(id){
   const auto* d=host->descriptor(id);const auto lane=host->automationLane(id);
   if(!d||lane>=10)continue;
   for(uint32_t p=0;p<d->parameter_count;++p){
    const auto& desc=d->parameters[p];const auto normalized=double(params[lane*64+p]->get());
    double value=desc.minimum+normalized*(desc.maximum-desc.minimum);
    if((desc.flags&MLP_PARAMETER_LOGARITHMIC)&&desc.minimum>0&&desc.maximum>0)
      value=desc.minimum*std::pow(desc.maximum/desc.minimum,normalized);
    if(desc.step>0)value=desc.minimum+std::round((value-desc.minimum)/desc.step)*desc.step;
    value=std::clamp(value,desc.minimum,desc.maximum);
    host->setParameter(id,desc.id,value);
   }
  }
  if(buffer.getNumChannels()>=2&&!host->process(buffer.getWritePointer(0),buffer.getWritePointer(1),size_t(buffer.getNumSamples())))buffer.clear();
 }
 bool hasEditor() const override{return true;}
 juce::AudioProcessorEditor* createEditor() override{return new juce::GenericAudioProcessorEditor(*this);}
 void getStateInformation(juce::MemoryBlock& dest) override{
  const juce::ScopedLock lock(getCallbackLock());
  std::vector<uint8_t> bytes;if(RackStateCodec::encode(host->saveState(order),bytes))dest.replaceAll(bytes.data(),bytes.size());
 }
 void setStateInformation(const void* data,int size) override{
  RackState state;if(size<0||!RackStateCodec::decode(static_cast<const uint8_t*>(data),size_t(size),state))return;
  const juce::ScopedLock lock(getCallbackLock());
  host->restoreStateWhenStopped(state,order);
  for(const auto id:order.slots())if(id){
   const auto* d=host->descriptor(id);const auto lane=host->automationLane(id);if(!d||lane>=10)continue;
   for(uint32_t p=0;p<d->parameter_count;++p){const auto& desc=d->parameters[p];double value=desc.default_value;host->getParameter(id,desc.id,value);
    double normalized=desc.maximum>desc.minimum?(value-desc.minimum)/(desc.maximum-desc.minimum):0;
    if((desc.flags&MLP_PARAMETER_LOGARITHMIC)&&desc.minimum>0&&desc.maximum>desc.minimum&&value>0)
     normalized=std::log(value/desc.minimum)/std::log(desc.maximum/desc.minimum);
    params[lane*64+p]->setValue(float(std::clamp(normalized,0.,1.)));
   }
  }
  host->submit(order);
 }
private:
 void timerCallback() override{
  const auto latency=int(host->diagnostics().latency);if(latency!=getLatencySamples())setLatencySamples(latency);
  const juce::ScopedLock lock(getCallbackLock());host->collectRetired();
 }
 std::unique_ptr<ModuleRackHost> host=std::make_unique<ModuleRackHost>();
 RackOrder order;std::array<juce::AudioParameterFloat*,640> params{};
};
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ml::PluginProcessor();}
