#include <juce_audio_utils/juce_audio_utils.h>
#include "ModuleRackHost.h"
#include "ParameterRegistry.h"
#include "StateAccessGate.h"
#include "LiveHostExchange.h"
#include "RackEditor.h"
#include "AssetLoader.h"
#include "RotaryParameterControl.h"
#include "AnalysisHistory.h"
#include "StudioControls.h"
#include "SpectralDisplay.h"
#include "../sdk/CompressorCurve.h"
#include "../sdk/StereoArchitectSettings.h"
#include <bit>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <mutex>
#include <optional>

namespace ml {
// Initial VST3 integration: fixed normalized automation lanes, no dynamic host parameter IDs.
class PluginProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
 explicit PluginProcessor(juce::File root=installedRoot())
  :AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),dataDirectory(std::move(root)){
  for(unsigned i=0;i<params.size();++i){
   auto* p=new juce::AudioParameterFloat(juce::ParameterID{"lane_"+juce::String(i),1},
      "Lane "+juce::String(i/64+1)+" Parameter "+juce::String(i%64+1),juce::NormalisableRange<float>{0.f,1.f,0.f},0.f);
   addParameter(p);params[i]=p;
  }
  bindHost(*host,*automation);
  host->scan(std::filesystem::path(dataDirectory.getChildFile("modules").getFullPathName().toWideCharPointer()));
  exchange.resetWhenStopped(host.get());
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
  const std::lock_guard<std::mutex> lock(controlMutex);
  exchange.resetWhenStopped(host.get());oldHost.reset();oldAutomation.reset();currentRate=rate;currentBlock=uint32_t(std::max(1,block));
  host->resetGraphWhenStopped();host->prepare(rate,currentBlock);host->submit(order,true);exchange.resetWhenStopped(host.get());
 }
 void releaseResources() override{}
 void processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) override{
  juce::ScopedNoDenormals noDenormals;
  StateAccessGate::AudioGuard access(stateGate);if(!access){buffer.clear();return;}
  if(buffer.getNumChannels()>=2&&!exchange.process(buffer.getWritePointer(0),buffer.getWritePointer(1),unsigned(buffer.getNumSamples())))buffer.clear();
 }
 bool hasEditor() const override{return true;}
 juce::AudioProcessorEditor* createEditor() override;
 static juce::File installedRoot(){return juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory).getChildFile("Jerzy").getChildFile("MasterLabPro");}
 const juce::File& dataRoot()const noexcept{return dataDirectory;}
 RackOrder editorOrder(){const std::lock_guard<std::mutex> lock(controlMutex);return order;}
 struct EditorParameterInfo{juce::AudioParameterFloat* parameter;float neutral;juce::String name,unit;double minimum,maximum,step;uint32_t flags;};
 struct EditorModuleInfo{juce::String name;std::vector<EditorParameterInfo> parameters;bool compressor=false,stereo=false;};
 std::optional<EditorModuleInfo> editorModule(uint64_t id){
  if(!id)return std::nullopt;
  const std::lock_guard<std::mutex> lock(controlMutex);const auto* d=host->descriptor(id);const auto lane=host->automationLane(id);
  if(!d||lane>=10||d->parameter_count>64)return std::nullopt;
  EditorModuleInfo result;result.name=juce::String::fromUTF8(d->name);
  result.compressor=d->type_id_hi==MLP_INPUT_COMPRESSOR_HI&&d->type_id_lo==MLP_INPUT_COMPRESSOR_LO&&d->parameter_count==46;
  result.stereo=d->type_id_hi==stereoTypeHi&&d->type_id_lo==stereoTypeLo&&d->parameter_count==14;
  for(uint32_t i=0;i<d->parameter_count;++i){const auto& v=d->parameters[i];result.parameters.push_back({params[lane*64+i],float(normalize(v,v.neutral_value)),juce::String::fromUTF8(v.name),juce::String::fromUTF8(v.unit?v.unit:""),v.minimum,v.maximum,v.step,v.flags});}
  return result;
 }
 juce::String editorLabel(uint64_t id){const std::lock_guard<std::mutex> lock(controlMutex);const auto* d=host->descriptor(id);return d?juce::String::fromUTF8(d->name):juce::String("UNAVAILABLE");}
 juce::StringArray moduleNames(){const std::lock_guard<std::mutex> lock(controlMutex);juce::StringArray names;for(const auto& t:host->availableTypes())names.add(juce::String::fromUTF8(t.descriptor->name));return names;}
 static double normalize(const MlpParameterDescriptor& d,double value){
  if(d.maximum<=d.minimum)return 0;
  if((d.flags&MLP_PARAMETER_LOGARITHMIC)&&d.minimum>0&&value>0)return std::clamp(std::log(value/d.minimum)/std::log(d.maximum/d.minimum),0.,1.);
  return std::clamp((value-d.minimum)/(d.maximum-d.minimum),0.,1.);
 }
 juce::AudioParameterFloat* editorParameter(uint64_t id,uint32_t index){
  const std::lock_guard<std::mutex> lock(controlMutex);const auto lane=host->automationLane(id);return lane<10&&index<64?params[lane*64+index]:nullptr;
 }
 bool addModule(int slot,size_t type){
  const std::lock_guard<std::mutex> lock(controlMutex);
  if(slot<0||slot>=10||order.slots()[size_t(slot)]||nextId==std::numeric_limits<uint64_t>::max())return false;
  for(const auto& missing:host->unavailable())if(missing.slot==uint32_t(slot))return false;
  const auto id=nextId++;if(!host->add(type,id))return false;
  const auto* d=host->descriptor(id);const auto lane=host->automationLane(id);
  for(uint32_t i=0;i<d->parameter_count;++i)*params[lane*64+i]=float(normalize(d->parameters[i],d->parameters[i].default_value));
  if(!order.insert(size_t(slot),id)){host->remove(id);return false;}
  dirty=true;return true;
 }
 bool moveModule(int from,int to){const std::lock_guard<std::mutex> lock(controlMutex);if(from<0||to<0||!host->unavailable().empty()||!order.move(size_t(from),size_t(to)))return false;dirty=true;return true;}
 bool removeModule(uint64_t id){const std::lock_guard<std::mutex> lock(controlMutex);for(size_t i=0;i<10;++i)if(order.slots()[i]==id&&host->remove(id)){order.remove(i);dirty=true;return true;}return false;}
 void analysisSource(uint64_t id,bool module,bool delta){const std::lock_guard<std::mutex> lock(controlMutex);host->setAnalysis(id,module,delta);}
 bool readMeters(ModuleRackMeters& m){const std::lock_guard<std::mutex> lock(controlMutex);return host->pollMeters(m);}
 bool readAnalysis(AnalysisFrame& f){const std::lock_guard<std::mutex> lock(controlMutex);return host->pollAnalysis(f);}
 float gainReduction(uint64_t id,uint32_t band){const std::lock_guard<std::mutex> lock(controlMutex);return host->gainReduction(id,band);}
 uint64_t moduleDynamics(uint64_t id,uint32_t band){const std::lock_guard<std::mutex> lock(controlMutex);return host->moduleDynamics(id,band);}
 uint64_t editorEpoch()const noexcept{return epoch.load();}

 void getStateInformation(juce::MemoryBlock& dest) override{
  const std::lock_guard<std::mutex> control(controlMutex);
  RackState state;
  if(host->supportsConcurrentStateRead())state=host->saveState(order);
  else {StateAccessGate::ControlGuard access(stateGate);state=host->saveState(order);}
  // Host automation is authoritative even before the next audio block.
  for(auto& m:state.modules)if(const auto* d=host->descriptor(m.instance)){
   for(auto& saved:m.parameters)for(uint32_t i=0;i<d->parameter_count;++i)if(d->parameters[i].id==saved.id)saved.value=automationValue(d->parameters[i],params[m.lane*64+i]->get());
  }
  std::vector<uint8_t> bytes;if(RackStateCodec::encode(state,bytes))dest.replaceAll(bytes.data(),bytes.size());
 }
 void setStateInformation(const void* data,int size) override{
  RackState state;if(size<0||!RackStateCodec::decode(static_cast<const uint8_t*>(data),size_t(size),state))return;
  const std::lock_guard<std::mutex> control(controlMutex);
  collectReplacement();
  // If an earlier transition is still in flight, retain the latest request for the control timer.
  if(oldHost){deferredState=std::move(state);return;}
  replaceHost(state);
 }
private:
 void replaceHost(const RackState& state){
  auto replacement=std::make_unique<ModuleRackHost>();
  const auto root=dataRoot().getChildFile("modules");
  replacement->scan(std::filesystem::path(root.getFullPathName().toWideCharPointer()));
  replacement->prepare(currentRate,currentBlock);
  RackOrder restored;replacement->restoreStateWhenStopped(state,restored);
  // Freeze automation of the outgoing host at its current module values.
  // Existing snapshots retain the old reader; during the transition use a per-host context below.
  for(unsigned i=0;i<params.size();++i)automation->frozenValues[i]=params[i]->get();
  automation->frozen.store(true,std::memory_order_release);
  oldAutomation=std::move(automation);automation=std::make_unique<AutomationContext>();
  oldHost=std::move(host);host=std::move(replacement);order=restored;dirty=false;
  bindHost(*host,*automation);
  for(const auto& m:state.modules)if(m.instance>=nextId)nextId=m.instance==std::numeric_limits<uint64_t>::max()?m.instance:m.instance+1;
  epoch.fetch_add(1);
  for(const auto id:order.slots())if(id){
   const auto* d=host->descriptor(id);const auto lane=host->automationLane(id);if(!d||lane>=10)continue;
   for(uint32_t p=0;p<d->parameter_count;++p){const auto& desc=d->parameters[p];double value=desc.default_value;host->getParameter(id,desc.id,value);
    double normalized=desc.maximum>desc.minimum?(value-desc.minimum)/(desc.maximum-desc.minimum):0;
    if((desc.flags&MLP_PARAMETER_LOGARITHMIC)&&desc.minimum>0&&desc.maximum>desc.minimum&&value>0)
     normalized=std::log(value/desc.minimum)/std::log(desc.maximum/desc.minimum);
    *params[lane*64+p]=float(std::clamp(normalized,0.,1.));
   }
  }
  host->submit(order,true);
  exchange.publish(host.get());
 }
 struct AutomationContext{PluginProcessor* owner=nullptr;std::atomic<bool> frozen{false};std::array<float,640> frozenValues{};};
 void bindHost(ModuleRackHost& h,AutomationContext& context){context.owner=this;h.bindAutomation(&context,[](const void* raw,uint32_t index)noexcept{const auto& c=*static_cast<const AutomationContext*>(raw);return c.frozen.load(std::memory_order_acquire)?c.frozenValues[index]:c.owner->params[index]->get();});}
 void collectReplacement(){if(auto* retired=exchange.takeRetired();retired&&retired==oldHost.get()){oldHost.reset();oldAutomation.reset();}}
 void timerCallback() override{
  int latency=0;
  {const std::lock_guard<std::mutex> lock(controlMutex);collectReplacement();if(!oldHost&&deferredState){auto state=std::move(*deferredState);deferredState.reset();replaceHost(state);}host->collectRetired();if(dirty&&host->submit(order))dirty=false;latency=int(exchange.latency());}
  if(latency!=getLatencySamples())setLatencySamples(latency);
 }
 const juce::File dataDirectory;
 std::unique_ptr<ModuleRackHost> host=std::make_unique<ModuleRackHost>();
 std::unique_ptr<ModuleRackHost> oldHost;std::unique_ptr<AutomationContext> automation=std::make_unique<AutomationContext>(),oldAutomation;
 LiveHostExchange<ModuleRackHost> exchange;std::optional<RackState> deferredState;double currentRate=44100.;uint32_t currentBlock=512;
 StateAccessGate stateGate;std::mutex controlMutex;RackOrder order;uint64_t nextId=1;bool dirty=false;std::atomic<uint64_t> epoch{0};std::array<juce::AudioParameterFloat*,640> params{};
};
class PluginRackEditor final:public juce::AudioProcessorEditor,private juce::Timer {
 using EditorParam=PluginProcessor::EditorParameterInfo;
 static float physical(const EditorParam& p){
  MlpParameterDescriptor d{};d.minimum=p.minimum;d.maximum=p.maximum;d.step=p.step;d.flags=p.flags;
  return float(automationValue(d,p.parameter->get()));
 }
 static void adjust(const EditorParam& p,double value){
  MlpParameterDescriptor d{};d.minimum=p.minimum;d.maximum=p.maximum;d.flags=p.flags;
  p.parameter->setValueNotifyingHost(float(PluginProcessor::normalize(d,value)));
 }
 static void displayFrame(juce::Graphics& g,juce::Rectangle<float> area){
  g.setColour(juce::Colour(0xff0b1721));g.fillRoundedRectangle(area,4);
  g.setColour(juce::Colour(0xff253746));g.drawRoundedRectangle(area,4,1);
 }
 static float peakDb(float peak){return 20.f*std::log10(std::max(peak,1e-6f));}
 class CompressorPanel final:public juce::Component,private juce::Timer {
  class ResetButton final:public StudioButton {
  public:
   std::function<void()> reset;
   void mouseDown(const juce::MouseEvent& event)override{
    if(event.mods.isRightButtonDown()){if(reset)reset();return;}
    StudioButton::mouseDown(event);
   }
  };
 public:
  CompressorPanel(PluginProcessor& processor,uint64_t id,PluginProcessor::EditorModuleInfo descriptor,
                  juce::Image metal,juce::Image indicator,std::function<void()> remove)
   :p(processor),instance(id),info(std::move(descriptor)),cap(std::move(metal)),pointer(std::move(indicator)),
    mode(*info.parameters[4].parameter,[this](float v){analog.setToggleState(v>.5f,juce::dontSendNotification);analog.setButtonText(v>.5f?"ANALOG":"DIGITAL");analog.setIndicator(v>.5f?juce::Colour(0xffe3ad60):juce::Colour(0xff58bde8));}),
    bypassAttachment(*info.parameters[5].parameter,[this](float v){bypass.setToggleState(v>.5f,juce::dontSendNotification);bypass.setButtonText(v>.5f?"BYPASSED":"ACTIVE");bypass.setIndicator(v>.5f?juce::Colours::red:juce::Colours::limegreen);}) {
   setName("Input Compressor Panel");
   const char* names[]{"LOW","MID","HIGH","SUM","CROSSOVER"};
   for(size_t i=0;i<banks.size();++i){
    banks[i].setButtonText(names[i]);banks[i].setAccent(bandAccent(bandIds[i]));addAndMakeVisible(banks[i]);
    banks[i].onClick=[this,i]{bank=i;rebuild();};
   }
   addAndMakeVisible(analog);addAndMakeVisible(bypass);addAndMakeVisible(removeButton);addAndMakeVisible(bandActive);
   bandActive.loadSkins(p.dataRoot().getChildFile("assets"));
   bandActive.onClick=[this]{if(bank<4){auto& v=info.parameters[38+bank];v.parameter->beginChangeGesture();v.parameter->setValueNotifyingHost(v.parameter->get()<.5f?1.f:0.f);v.parameter->endChangeGesture();updateBandActive();}};
   analog.onClick=[this]{mode.setValueAsCompleteGesture(info.parameters[4].parameter->get()<.5f?1.f:0.f);};
   bypass.onClick=[this]{bypassAttachment.setValueAsCompleteGesture(info.parameters[5].parameter->get()<.5f?1.f:0.f);};
   analog.reset=[this]{mode.setValueAsCompleteGesture(info.parameters[4].neutral);};
   bypass.reset=[this]{bypassAttachment.setValueAsCompleteGesture(info.parameters[5].neutral);};
   removeButton.onClick=std::move(remove);
   for(auto& button:banks)button.loadSkins(p.dataRoot().getChildFile("assets"));
   for(auto* button:std::array<StudioButton*,3>{&analog,&bypass,&removeButton})button->loadSkins(p.dataRoot().getChildFile("assets"));
   mode.sendInitialUpdate();bypassAttachment.sendInitialUpdate();rebuild();startTimerHz(30);
  }
  ~CompressorPanel() override{stopTimer();finishGesture();}
  void resized() override {
   auto area=getLocalBounds().reduced(6);
   auto header=area.removeFromTop(30);
   const int width=juce::jmin(110,header.getWidth()/10);
   for(auto& button:banks)button.setBounds(header.removeFromLeft(width).reduced(2,0));
   removeButton.setBounds(header.removeFromRight(width).reduced(2,0));
   bypass.setBounds(header.removeFromRight(width).reduced(2,0));
   analog.setBounds(header.removeFromRight(width).reduced(2,0));
   bandActive.setBounds(header.removeFromRight(width).reduced(2,0));
   area.removeFromBottom(23);
   auto plots=area.removeFromTop(juce::jmax(38,juce::jmin(area.getHeight()*55/100,juce::jmax(38,area.getHeight()-140)))).reduced(1,2);
   transfer=plots.removeFromLeft(plots.getWidth()*38/100).reduced(2).toFloat();
   timeline=plots.reduced(4,2).toFloat();
   for(size_t i=0;i<knobs.size();++i){
    const auto left=area.getX()+area.getWidth()*int(i)/int(knobs.size());
    const auto right=area.getX()+area.getWidth()*int(i+1)/int(knobs.size());
    knobs[i]->setBounds(left,area.getY(),right-left,area.getHeight());
   }
  }
  void paint(juce::Graphics& g) override {
   const auto meter=p.gainReduction(instance,uint32_t(std::min<size_t>(bank,3)));
   auto strip=getLocalBounds().reduced(12,6).removeFromBottom(18);
   g.setColour(juce::Colours::whitesmoke);
   g.drawText("GAIN REDUCTION",strip.removeFromLeft(140),juce::Justification::centredLeft);
   g.drawText(juce::String(meter,1)+" dB",strip.removeFromRight(76),juce::Justification::centredRight);
   g.setColour(juce::Colour(0xff101820));g.fillRoundedRectangle(strip.toFloat(),4);
   g.setColour(accent());g.fillRoundedRectangle(strip.toFloat().withWidth(float(strip.getWidth())*juce::jlimit(0.f,1.f,meter/24)),4);
   drawPlots(g);
  }
  void mouseDown(const juce::MouseEvent& event)override{
   if(!transfer.contains(event.position)||bank>=4)return;
   const auto region=graphArea(transfer);
   const auto base=6+bank*7;
   if(event.mods.isShiftDown())dragIndex=base+1;
   else if(event.mods.isAltDown())dragIndex=base+4;
   else dragIndex=base;
   auto& param=info.parameters[*dragIndex];
   param.parameter->beginChangeGesture();
   if(event.mods.isRightButtonDown()){
    param.parameter->setValueNotifyingHost(param.neutral);finishGesture();return;
   }
   updateGesture(event.position,region);
  }
  void mouseDrag(const juce::MouseEvent& event)override{if(dragIndex)updateGesture(event.position,graphArea(transfer));}
  void mouseUp(const juce::MouseEvent&)override{finishGesture();}
  void mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel)override{
   if(!timeline.contains(event.position))return;
   if(event.mods.isShiftDown())verticalRange=juce::jlimit(24.f,90.f,verticalRange-wheel.deltaY*32);
   else windowSeconds=juce::jlimit(2.f,10.f,windowSeconds-wheel.deltaY*4);
   repaint();
  }
 private:
  struct Sample{float input=-120,output=-120,reduction=0;};
  void finishGesture(){if(dragIndex){info.parameters[*dragIndex].parameter->endChangeGesture();dragIndex.reset();}}
  void updateGesture(juce::Point<float> point,juce::Rectangle<float> area){
   auto base=6+bank*7;
   const float x=juce::jlimit(0.f,1.f,(point.x-area.getX())/std::max(1.f,area.getWidth()));
   const float y=juce::jlimit(0.f,1.f,(point.y-area.getY())/std::max(1.f,area.getHeight()));
   adjust(info.parameters[*dragIndex],*dragIndex==base?std::min(0.f,x*66-60):*dragIndex==base+1?std::pow(20.f,y):24*y);
   repaint();
  }
  static juce::Rectangle<float> graphArea(juce::Rectangle<float> area){area.removeFromTop(18);return area.reduced(8,4);}
  void drawPlots(juce::Graphics& g){
   displayFrame(g,transfer);displayFrame(g,timeline);
   g.setColour(juce::Colours::whitesmoke);g.setFont(11.f);
   g.drawFittedText(bank==4?"SUM TRANSFER / dBFS":"TRANSFER / dBFS   Shift: ratio   Alt: knee",transfer.toNearestInt().removeFromTop(17).reduced(7,0),juce::Justification::centredLeft,1);
   auto legend=timeline.toNearestInt().removeFromTop(17).reduced(7,0);
   constexpr uint32_t colours[]{0xff58bde8,0xffe3ad60,0xffcb9cea};
   const char* labels[]{"BEFORE","AFTER","GR"};
   for(int i=0;i<3;++i){g.setColour(juce::Colour(colours[i]));g.drawFittedText(labels[i],legend.removeFromLeft(juce::jmin(66,legend.getWidth()/ (4-i))),juce::Justification::centredLeft,1);}
   g.setColour(juce::Colours::whitesmoke);g.drawFittedText(juce::String(windowSeconds,1)+" s",legend,juce::Justification::centredRight,1);
   const auto displayBank=std::min<size_t>(bank,3);
   const auto base=6+displayBank*7;
   const float threshold=physical(info.parameters[base]),ratio=physical(info.parameters[base+1]);
   const float knee=physical(info.parameters[base+4]),mix=physical(info.parameters[base+5])*.01f;
   const float shape=info.parameters[4].parameter->get()>.5f?1.f:0.f;
   const auto a=graphArea(transfer),b=graphArea(timeline);
   const auto valueY=[](juce::Rectangle<float> area,float db,float minimum,float maximum){
    return area.getBottom()-area.getHeight()*juce::jlimit(0.f,1.f,(db-minimum)/(maximum-minimum));
   };
   const auto transferPoint=[&](float in,float out){
    return juce::Point<float>(a.getX()+a.getWidth()*juce::jlimit(0.f,1.f,(in+60)/66),valueY(a,out,-60,6));
   };
   g.setColour(juce::Colour(0xff324450));g.drawLine({a.getBottomLeft(),a.getTopRight()},1.f);
   juce::Path curve;
   for(int i=0;i<=66;++i){
    const auto in=float(i)-60;
    const float gr=compressorReduction(in,threshold,ratio,knee,shape);
    const auto point=transferPoint(in,in+peakDb(std::lerp(1.f,std::pow(10.f,-gr/20),mix)));
    if(i==0)curve.startNewSubPath(point);else curve.lineTo(point);
   }
   g.setColour(accent());g.strokePath(curve,juce::PathStrokeType(2));
   const auto marker=transferPoint(threshold,threshold);
   g.drawEllipse(juce::Rectangle<float>(9,9).withCentre(marker),1.5f);
   const auto& recent=history[displayBank][(position+299)%300];
   if(recent.input>-100){g.setColour(juce::Colours::white);g.fillEllipse(juce::Rectangle<float>(5,5).withCentre(transferPoint(recent.input,recent.output)));}
   g.setColour(accent().withAlpha(.5f));
   const auto thresholdY=valueY(b,threshold,-verticalRange,0);
   g.drawHorizontalLine(int(thresholdY),b.getX(),b.getRight());
   const int count=std::max(2,int(windowSeconds*30));
   for(int line=0;line<3;++line){
    juce::Path path;
    for(int i=0;i<count;++i){
     const auto& sample=history[displayBank][(position+300-size_t(count)+size_t(i))%300];
     const auto x=b.getX()+b.getWidth()*float(i)/float(count-1);
     const auto y=valueY(b,line==0?sample.input:line==1?sample.output:-sample.reduction,line<2?-verticalRange:-24.f,0);
     if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);
    }
    g.setColour(juce::Colour(colours[line]));
    g.strokePath(path,juce::PathStrokeType(line==2?2.f:1.5f));
    if(line<2){path.lineTo(b.getBottomRight());path.lineTo(b.getBottomLeft());path.closeSubPath();g.setColour(juce::Colour(colours[line]).withAlpha(.12f));g.fillPath(path);}
   }
  }
  juce::Colour accent()const {
   return bandAccent(bandIds[bank]);
  }
  void rebuild(){
   knobs.clear();
   for(size_t i=0;i<banks.size();++i)banks[i].setToggleState(i==bank,juce::dontSendNotification);
   auto add=[&](size_t index){
    const auto v=info.parameters[index];
    auto knob=std::make_unique<RotaryParameterControl>(*v.parameter,v.neutral,cap,pointer,accent(),v.name,[v](float normal){
     MlpParameterDescriptor d{};d.minimum=v.minimum;d.maximum=v.maximum;d.step=v.step;d.flags=v.flags;
     const double value=automationValue(d,normal);
     if(v.unit==":1")return juce::String(value,2)+":1";
     return juce::String(value,v.unit=="Hz"||v.unit=="%"?0:1)+" "+v.unit;
    });
    addAndMakeVisible(*knob);knobs.push_back(std::move(knob));
   };
   if(bank==4){add(0);add(1);add(2);add(3);}
   else {for(size_t i=0;i<7;++i)add(6+bank*7+i);add(34+bank);add(42+bank);}
   updateBandActive();
   resized();repaint();
  }
  void updateBandActive(){bandActive.setVisible(bank<4);if(bank<4){const bool on=info.parameters[38+bank].parameter->get()>.5f;bandActive.setButtonText(on?"BAND ON":"BAND OFF");bandActive.setToggleState(on,juce::dontSendNotification);bandActive.setIndicator(on?juce::Colours::limegreen:juce::Colours::red);}}
  void timerCallback()override {
   updateBandActive();
   for(uint32_t b=0;b<4;++b){
    const auto packed=p.moduleDynamics(instance,b);
    history[b][position]={peakDb(std::bit_cast<float>(uint32_t(packed>>32))),
                          peakDb(std::bit_cast<float>(uint32_t(packed))),p.gainReduction(instance,b)};
   }
   position=(position+1)%300;repaint();
  }
  PluginProcessor& p;uint64_t instance;PluginProcessor::EditorModuleInfo info;
  juce::Image cap,pointer;size_t bank=3;
  juce::Rectangle<float> transfer,timeline;
  std::optional<size_t> dragIndex;
  std::array<std::array<Sample,300>,4> history{};
  size_t position=0;
  float windowSeconds=10,verticalRange=60;
  static constexpr std::array<int,5> bandIds{1,3,5,7,7};
  std::array<StudioButton,5> banks;
  ResetButton analog,bypass;
  StudioButton removeButton{"REMOVE"},bandActive{"BAND ON"};
  juce::ParameterAttachment mode,bypassAttachment;
  std::vector<std::unique_ptr<RotaryParameterControl>> knobs;
 };
 class StereoPanel final:public juce::Component,private juce::Timer {
 public:
  StereoPanel(PluginProcessor& processor,uint64_t id,PluginProcessor::EditorModuleInfo descriptor,std::function<void()> remove)
   :p(processor),instance(id),info(std::move(descriptor)),
    bypassAttachment(*info.parameters[13].parameter,[this](float value){
     bypass.setToggleState(value>.5f,juce::dontSendNotification);
     bypass.setButtonText(value>.5f?"BYPASSED":"ACTIVE");
     bypass.setIndicator(value>.5f?juce::Colours::red:juce::Colours::limegreen);
    }){
   setName("Stereo Architect Panel");
   addAndMakeVisible(bypass);addAndMakeVisible(removeButton);
   removeButton.onClick=std::move(remove);
   bypass.onClick=[this]{bypassAttachment.setValueAsCompleteGesture(info.parameters[13].parameter->get()<.5f?1.f:0.f);};
   bypass.loadSkins(p.dataRoot().getChildFile("assets"));removeButton.loadSkins(p.dataRoot().getChildFile("assets"));
   for(size_t i=0;i<7;++i){
    const auto v=info.parameters[i];
    auto control=std::make_unique<RotaryParameterControl>(*v.parameter,v.neutral,juce::ImageFileFormat::loadFrom(p.dataRoot().getChildFile("assets/knob_cap.png")),juce::ImageFileFormat::loadFrom(p.dataRoot().getChildFile("assets/knob_pointer.png")),bandAccent(int(i)),stereoBands[i],[](float n){return juce::String(n*200.f,0)+"%";});
    addAndMakeVisible(*control);widthKnobs.push_back(std::move(control));
   }
   bypassAttachment.sendInitialUpdate();startTimerHz(30);
  }
  ~StereoPanel()override{stopTimer();finishGesture();}
  void resized()override{
   auto area=getLocalBounds().reduced(4);
   auto header=area.removeFromTop(30);
   removeButton.setBounds(header.removeFromRight(104));
   bypass.setBounds(header.removeFromRight(104).reduced(3,0));
   area.removeFromBottom(29);
   auto controls=area.removeFromBottom(std::min(115,area.getHeight()/2));
   for(size_t i=0;i<widthKnobs.size();++i){const int left=controls.getX()+controls.getWidth()*int(i)/7,right=controls.getX()+controls.getWidth()*int(i+1)/7;widthKnobs[i]->setBounds(left,controls.getY(),right-left,controls.getHeight());}
   plot=area.reduced(7,4).toFloat();
  }
  void paint(juce::Graphics& g)override {
   g.setColour(juce::Colours::whitesmoke);g.setFont(12.f);
   g.drawFittedText("STEREO WIDTH / drag dividers or band handles",getLocalBounds().removeFromTop(27).reduced(8,0).withTrimmedRight(218),juce::Justification::centredLeft,1);
   displayFrame(g,plot);
   const auto cutoffs=splits();
   const auto graph=graphBounds();
   if(graph.isEmpty())return;
   for(size_t band=0;band<7;++band){
    const float start=band==0?20:cutoffs[band-1],end=band==6?20000:cutoffs[band];
    const auto x1=xAt(graph,start),x2=xAt(graph,end);
    const auto y=graph.getBottom()-physical(info.parameters[band])*graph.getHeight()/200;
    const auto colour=bandAccent(int(band));
    g.setColour(colour.withAlpha(.13f));g.fillRect(x1,graph.getY(),x2-x1,graph.getHeight());
    g.setColour(colour);g.drawHorizontalLine(int(y),x1,x2);
    g.fillEllipse(juce::Rectangle<float>(8,8).withCentre({(x1+x2)*.5f,y}));
    auto label=juce::Rectangle<int>(int(x1),int(graph.getBottom()+3),int(x2-x1),16);
    g.drawFittedText(stereoBands[band],label.reduced(2,0),juce::Justification::centred,1,.65f);
    if(band==6)continue;
    g.setColour(juce::Colours::white.withAlpha(.5f));g.drawVerticalLine(int(x2),graph.getY(),graph.getBottom());
    g.drawFittedText(juce::String(cutoffs[band],0)+" Hz",juce::Rectangle<int>(int(x2)-33,int(graph.getY())-17,66,16),juce::Justification::centred,1,.75f);
   }
   for(int level:{0,50,100,150,200}){
    const float y=graph.getBottom()-graph.getHeight()*float(level)/200.f;
    g.setColour(juce::Colours::white.withAlpha(level==100?.6f:.16f));g.drawHorizontalLine(int(y),graph.getX(),graph.getRight());
    g.setColour(juce::Colours::whitesmoke);g.drawFittedText(juce::String(level)+"%",juce::Rectangle<int>(int(plot.getX()+4),int(y)-7,40,14),juce::Justification::centredRight,1);
   }
   const auto data=p.moduleDynamics(instance,1);
   const auto corr=std::bit_cast<float>(uint32_t(p.moduleDynamics(instance,0)));
   const auto mid=peakDb(std::bit_cast<float>(uint32_t(data>>32))),side=peakDb(std::bit_cast<float>(uint32_t(data)));
   auto line=getLocalBounds().removeFromBottom(25).reduced(12,0);
   g.setColour(corr<0?juce::Colour(0xffec8d79):juce::Colours::whitesmoke);
   g.drawFittedText("CORRELATION  "+juce::String(corr,2)+"    MID  "+juce::String(mid,1)+" dBFS    SIDE  "+juce::String(side,1)+" dBFS",
                    line.removeFromRight(line.getWidth()/2),juce::Justification::centredRight,1,.72f);
   const auto limits=rangeLabel(cutoffs);
   g.setColour(bandAccent(int(selectedBand)));g.drawFittedText(limits,line,juce::Justification::centredLeft,1,.7f);
  }
  void mouseDown(const juce::MouseEvent& event)override{
   if(!graphBounds().contains(event.position))return;
   finishGesture();
   const auto area=graphBounds();const auto cutoffs=splits();
   size_t index=6;
   for(size_t band=0;band<cutoffs.size();++band){
    const auto boundary=xAt(area,cutoffs[band]);
    if(std::abs(event.position.x-boundary)<8){index=7+band;break;}
    if(event.position.x<boundary){index=band;break;}
   }
   if(index<7)selectedBand=index;
   auto& param=info.parameters[index];dragIndex=index;
   param.parameter->beginChangeGesture();
   if(event.mods.isRightButtonDown()){param.parameter->setValueNotifyingHost(param.neutral);finishGesture();}
   else update(event.position);
  }
  void mouseDrag(const juce::MouseEvent& event)override{if(dragIndex)update(event.position);}
  void mouseUp(const juce::MouseEvent&)override{finishGesture();}
 private:
  juce::Rectangle<float> graphBounds()const{return plot.reduced(10,20).withTrimmedLeft(40);}
  std::array<float,6> splits()const{
   std::array<float,6> result{};
   for(size_t i=0;i<result.size();++i)result[i]=physical(info.parameters[7+i]);
   const float rate=float(p.getSampleRate());
   return orderedStereoSplits(result,rate>0?std::min(20000.f,rate*.45f):20000.f);
  }
  juce::String rangeLabel(const std::array<float,6>& cuts)const{
   const auto low=selectedBand==0?20.f:cuts[selectedBand-1],high=selectedBand==6?20000.f:cuts[selectedBand];
   return juce::String(stereoBands[selectedBand])+"   "+juce::String(low,0)+"-"+juce::String(high,0)+" Hz   "+juce::String(physical(info.parameters[selectedBand]),0)+"%";
  }
  static float xAt(juce::Rectangle<float> area,float hz){return area.getX()+area.getWidth()*std::log(hz/20)/std::log(1000.f);}
  void update(juce::Point<float> point){
   const auto area=graphBounds();
   const size_t index=*dragIndex;
   if(index<7)adjust(info.parameters[index],std::clamp(double((area.getBottom()-point.y)/std::max(1.f,area.getHeight()))*200,0.,200.));
   else{
    const size_t crossing=index-7;
    const auto cutoffs=splits();
    const double fraction=std::clamp(double((point.x-area.getX())/std::max(1.f,area.getWidth())),0.,1.);
    const double lower=crossing?cutoffs[crossing-1]*1.05:20;
    const double upper=crossing+1<cutoffs.size()?cutoffs[crossing+1]/1.05:19000;
    adjust(info.parameters[index],std::clamp(20*std::pow(1000.,fraction),std::min(lower,upper),upper));
   }
   repaint();
  }
  void finishGesture(){if(dragIndex){info.parameters[*dragIndex].parameter->endChangeGesture();dragIndex.reset();}}
  void timerCallback()override{repaint();}
  PluginProcessor& p;uint64_t instance;PluginProcessor::EditorModuleInfo info;
  StudioButton bypass,removeButton{"REMOVE"};juce::ParameterAttachment bypassAttachment;
  std::vector<std::unique_ptr<RotaryParameterControl>> widthKnobs;
  juce::Rectangle<float> plot;std::optional<size_t> dragIndex;size_t selectedBand=0;
 };
 class Panel final:public juce::Component{
 public:
  Panel(const PluginProcessor::EditorModuleInfo& info,juce::Image cap,juce::Image pointer,std::function<void()> remove){
   setName("MasterLabPro Module Panel");title.setText(info.name,juce::dontSendNotification);addAndMakeVisible(title);
   for(const auto& parameter:info.parameters)if(parameter.parameter){
    auto knob=std::make_unique<RotaryParameterControl>(*parameter.parameter,parameter.neutral,cap,pointer,juce::Colour(0xff58bde8),parameter.name,[parameter](float normalized){MlpParameterDescriptor d{};d.minimum=parameter.minimum;d.maximum=parameter.maximum;d.step=parameter.step;d.flags=parameter.flags;const double value=automationValue(d,normalized);const int decimals=(d.flags&MLP_PARAMETER_INTEGER)?0:2;return juce::String(value,decimals)+(parameter.unit.isEmpty()?juce::String():" "+parameter.unit);});
    controlStrip.addAndMakeVisible(*knob);knobs.push_back(std::move(knob));
   }
   addAndMakeVisible(button);button.onClick=std::move(remove);addAndMakeVisible(viewport);viewport.setViewedComponent(&controlStrip,false);viewport.setScrollBarsShown(false,true);
  }
  void resized()override{auto r=getLocalBounds().reduced(4);auto header=r.removeFromTop(24);button.setBounds(header.removeFromRight(110));title.setBounds(header);viewport.setBounds(r);const int cell=juce::jlimit(96,180,r.getHeight()+12);const int width=juce::jmax(r.getWidth(),cell*int(knobs.size()));const int height=juce::jmax(1,r.getHeight()-(width>r.getWidth()?viewport.getScrollBarThickness():0));controlStrip.setSize(width,height);auto area=controlStrip.getLocalBounds();for(auto& k:knobs)k->setBounds(area.removeFromLeft(cell).reduced(4,0));}
 private:juce::Label title;StudioButton button{"REMOVE"};juce::Component controlStrip;juce::Viewport viewport;std::vector<std::unique_ptr<RotaryParameterControl>> knobs;
 };
public:
 explicit PluginRackEditor(PluginProcessor& processor):AudioProcessorEditor(processor),p(processor),assets(p.dataRoot().getChildFile("assets")),order(p.editorOrder()),cap(assets.loadKnobCap()),pointer(assets.loadKnobPointer()),
 rack(order,assets.loadBands(),{
 [this](int a,int b){if(p.moveModule(a,b))refresh();},
 [this](uint64_t id,int pos){if(id)select(id);else menu(pos);},
 [this](uint64_t id){return p.editorLabel(id);},
 [](uint64_t,int){}
 },assets.loadPanel()){
  rack.setRackSkin(assets.loadSlot());rack.setFooter("MASTERLABPRO / DEVELOPMENT BUILD");addAndMakeVisible(rack);addAndMakeVisible(scope);addAndMakeVisible(delta);
  scope.loadSkins(p.dataRoot().getChildFile("assets"));delta.loadSkins(p.dataRoot().getChildFile("assets"));
  scope.setIndicator(juce::Colour(0xff58bde8));delta.setAccent(juce::Colour(0xffcb9cea));
  scope.setClickingTogglesState(true);delta.setClickingTogglesState(true);scope.onClick=[this]{scope.setButtonText(scope.getToggleState()?"MODULE":"GLOBAL");scope.setIndicator(scope.getToggleState()?juce::Colour(0xffe3ad60):juce::Colour(0xff58bde8));sourceChanged();};delta.onClick=[this]{sourceChanged();};
  setResizable(true,false);setResizeLimits(800,500,3200,2000);if(auto* c=getConstrainer())c->setFixedAspectRatio(1.6);setSize(1200,750);seenEpoch=p.editorEpoch();startTimerHz(30);
 }
 ~PluginRackEditor()override{stopTimer();p.analysisSource(0,false,false);}
 void resized()override{rack.setBounds(getLocalBounds());const int y=getHeight()*10/1000+8,h=std::max(16,getHeight()*52/1000-16);scope.setBounds(getWidth()*44/100,y,100,h);delta.setBounds(getWidth()*44/100+105,y,120,h);}
private:
 void refresh(){order=p.editorOrder();rack.refreshRack();}
 void sourceChanged(){history.clear();spectralDisplay.clear();rack.setSpectrum({},20000);rack.setHeatmaps({},{},{});p.analysisSource(selected,scope.getToggleState(),delta.getToggleState());}
 void select(uint64_t id){selected=id;rack.setSelection(id);rack.setBandFromController(7);rack.setBandSelectionEnabled(false);auto info=p.editorModule(id);if(!info){rack.setModuleEditor(nullptr);sourceChanged();return;}
  juce::Component::SafePointer<PluginRackEditor> safe(this);
  auto remove=[safe,id]{juce::MessageManager::callAsync([safe,id]{if(safe!=nullptr&&safe->p.removeModule(id)){safe->select(0);safe->refresh();}});};
  if(info->compressor)rack.setModuleEditor(std::make_unique<CompressorPanel>(p,id,std::move(*info),cap,pointer,std::move(remove)),true);
  else if(info->stereo)rack.setModuleEditor(std::make_unique<StereoPanel>(p,id,std::move(*info),std::move(remove)),true);
  else rack.setModuleEditor(std::make_unique<Panel>(*info,cap,pointer,std::move(remove)));
  sourceChanged();
 }
 void menu(int slot){select(0);auto names=p.moduleNames();juce::PopupMenu m;for(int i=0;i<names.size();++i)m.addItem(i+1,"Add "+names[i]);if(names.isEmpty())m.addItem(999,"No compatible modules installed",false,false);juce::Component::SafePointer<PluginRackEditor> safe(this);m.showMenuAsync(juce::PopupMenu::Options(),[safe,slot](int choice){if(safe!=nullptr&&choice>0&&choice<999&&safe->p.addModule(slot,size_t(choice-1))){safe->refresh();safe->select(safe->order.slots()[size_t(slot)]);}});}
 void timerCallback()override{
  if(seenEpoch!=p.editorEpoch()){seenEpoch=p.editorEpoch();select(0);refresh();}
  ModuleRackMeters m;for(int i=0;i<7&&p.readMeters(m);++i)rack.setGlobalLevels(m.input,m.output);
  bool got=false;for(int i=0;i<4&&p.readAnalysis(frame);++i)if(frame.sourceId==(scope.getToggleState()?selected:0))got=history.accept(frame)||got;
  if(!got)return;
  rack.setSpectrum(spectralDisplay.traces(history,1./30.),float(std::min(20000.,history.sampleRate()*.5)));
  auto images=renderSpectralCurtain(history);
  rack.setHeatmaps(images[0],images[1],images[2]);
 }
 PluginProcessor& p;AssetLoader assets;RackOrder order;juce::Image cap,pointer;RackEditor rack;StudioButton scope{"GLOBAL"},delta{"DELTA LISTEN"};uint64_t selected=0,seenEpoch=0;AnalysisHistory history;AnalysisFrame frame;SpectralDisplay spectralDisplay;
};
juce::AudioProcessorEditor* PluginProcessor::createEditor(){return new PluginRackEditor(*this);}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ml::PluginProcessor();}
#if defined(MLP_EDITOR_TEST)
juce::AudioProcessor* createMasterLabProEditorFixture(const juce::File& root){return new ml::PluginProcessor(root);}
#endif
