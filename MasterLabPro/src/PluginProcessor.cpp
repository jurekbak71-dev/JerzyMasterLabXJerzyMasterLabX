#include <juce_audio_utils/juce_audio_utils.h>
#include "ModuleRackHost.h"
#include "ParameterRegistry.h"
#include "StateAccessGate.h"
#include "LiveHostExchange.h"
#include "RackEditor.h"
#include "AssetLoader.h"
#include "RotaryParameterControl.h"
#include "AnalysisHistory.h"
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
 PluginProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){
  for(unsigned i=0;i<params.size();++i){
   auto* p=new juce::AudioParameterFloat(juce::ParameterID{"lane_"+juce::String(i),1},
      "Lane "+juce::String(i/64+1)+" Parameter "+juce::String(i%64+1),juce::NormalisableRange<float>{0.f,1.f,0.f},0.f);
   addParameter(p);params[i]=p;
  }
  bindHost(*host,*automation);
  const auto root=juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory).getChildFile("Jerzy").getChildFile("MasterLabPro").getChildFile("modules");
  host->scan(std::filesystem::path(root.getFullPathName().toWideCharPointer()));
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
 static juce::File dataRoot(){return juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory).getChildFile("Jerzy").getChildFile("MasterLabPro");}
 RackOrder editorOrder(){const std::lock_guard<std::mutex> lock(controlMutex);return order;}
 struct EditorParameterInfo{juce::AudioParameterFloat* parameter;float neutral;};
 struct EditorModuleInfo{juce::String name;std::vector<EditorParameterInfo> parameters;};
 std::optional<EditorModuleInfo> editorModule(uint64_t id){
  if(!id)return std::nullopt;
  const std::lock_guard<std::mutex> lock(controlMutex);const auto* d=host->descriptor(id);const auto lane=host->automationLane(id);
  if(!d||lane>=10||d->parameter_count>64)return std::nullopt;
  EditorModuleInfo result;result.name=juce::String::fromUTF8(d->name);
  for(uint32_t i=0;i<d->parameter_count;++i)result.parameters.push_back({params[lane*64+i],float(normalize(d->parameters[i],d->parameters[i].neutral_value))});
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
 std::unique_ptr<ModuleRackHost> host=std::make_unique<ModuleRackHost>();
 std::unique_ptr<ModuleRackHost> oldHost;std::unique_ptr<AutomationContext> automation=std::make_unique<AutomationContext>(),oldAutomation;
 LiveHostExchange<ModuleRackHost> exchange;std::optional<RackState> deferredState;double currentRate=44100.;uint32_t currentBlock=512;
 StateAccessGate stateGate;std::mutex controlMutex;RackOrder order;uint64_t nextId=1;bool dirty=false;std::atomic<uint64_t> epoch{0};std::array<juce::AudioParameterFloat*,640> params{};
};
class PluginRackEditor final:public juce::AudioProcessorEditor,private juce::Timer {
 class Panel final:public juce::Component{
 public:
  Panel(const PluginProcessor::EditorModuleInfo& info,juce::Image cap,juce::Image pointer,std::function<void()> remove){
   setName("MasterLabPro Module Panel");title.setText(info.name,juce::dontSendNotification);addAndMakeVisible(title);
   for(const auto& parameter:info.parameters)if(parameter.parameter){
    auto knob=std::make_unique<RotaryParameterControl>(*parameter.parameter,parameter.neutral,cap,pointer,juce::Colour(0xff58bde8));
    addAndMakeVisible(*knob);knobs.push_back(std::move(knob));
   }
   addAndMakeVisible(button);button.onClick=std::move(remove);
  }
  void resized()override{auto r=getLocalBounds().reduced(4);title.setBounds(r.removeFromTop(24));button.setBounds(r.removeFromRight(120).withHeight(28));const int w=knobs.empty()?0:juce::jmin(170,r.getWidth()/int(knobs.size()));for(auto& k:knobs)k->setBounds(r.removeFromLeft(w));}
 private:juce::Label title;juce::TextButton button{"REMOVE"};std::vector<std::unique_ptr<RotaryParameterControl>> knobs;
 };
public:
 explicit PluginRackEditor(PluginProcessor& processor):AudioProcessorEditor(processor),p(processor),assets(PluginProcessor::dataRoot().getChildFile("assets")),order(p.editorOrder()),cap(assets.loadKnobCap()),pointer(assets.loadKnobPointer()),
 rack(order,assets.loadBands(),{
 [this](int a,int b){if(p.moveModule(a,b))refresh();},
 [this](uint64_t id,int pos){if(id)select(id);else menu(pos);},
 [this](uint64_t id){return p.editorLabel(id);},
 [](uint64_t,int){}
 },assets.loadPanel()){
  rack.setRackSkin(assets.loadSlot());addAndMakeVisible(rack);addAndMakeVisible(scope);addAndMakeVisible(delta);
  scope.setClickingTogglesState(true);delta.setClickingTogglesState(true);scope.onClick=[this]{scope.setButtonText(scope.getToggleState()?"MODULE":"GLOBAL");sourceChanged();};delta.onClick=[this]{sourceChanged();};
  setResizable(true,false);setResizeLimits(800,500,3200,2000);if(auto* c=getConstrainer())c->setFixedAspectRatio(1.6);setSize(1200,750);seenEpoch=p.editorEpoch();startTimerHz(30);
 }
 ~PluginRackEditor()override{stopTimer();p.analysisSource(0,false,false);}
 void resized()override{rack.setBounds(getLocalBounds());scope.setBounds(getWidth()*44/100,8,100,24);delta.setBounds(getWidth()*44/100+105,8,120,24);}
private:
 void refresh(){order=p.editorOrder();rack.refreshRack();}
 void sourceChanged(){history.clear();rack.setSpectrum({},20000);rack.setHeatmaps({},{},{});p.analysisSource(selected,scope.getToggleState(),delta.getToggleState());}
 void select(uint64_t id){selected=id;rack.setSelection(id);rack.setBandFromController(7);rack.setBandSelectionEnabled(false);auto info=p.editorModule(id);if(!info){rack.setModuleEditor(nullptr);sourceChanged();return;}
  juce::Component::SafePointer<PluginRackEditor> safe(this);
  rack.setModuleEditor(std::make_unique<Panel>(*info,cap,pointer,[safe,id]{juce::MessageManager::callAsync([safe,id]{if(safe!=nullptr&&safe->p.removeModule(id)){safe->select(0);safe->refresh();}});}));sourceChanged();
 }
 void menu(int slot){select(0);auto names=p.moduleNames();juce::PopupMenu m;for(int i=0;i<names.size();++i)m.addItem(i+1,"Add "+names[i]);if(names.isEmpty())m.addItem(999,"No compatible modules installed",false,false);juce::Component::SafePointer<PluginRackEditor> safe(this);m.showMenuAsync(juce::PopupMenu::Options(),[safe,slot](int choice){if(safe!=nullptr&&choice>0&&choice<999&&safe->p.addModule(slot,size_t(choice-1))){safe->refresh();safe->select(safe->order.slots()[size_t(slot)]);}});}
 void timerCallback()override{
  if(seenEpoch!=p.editorEpoch()){seenEpoch=p.editorEpoch();select(0);refresh();}
  ModuleRackMeters m;for(int i=0;i<7&&p.readMeters(m);++i)rack.setGlobalLevels(m.input,m.output);
  bool got=false;for(int i=0;i<4&&p.readAnalysis(frame);++i)if(frame.sourceId==(scope.getToggleState()?selected:0))got=history.accept(frame)||got;
  if(!got)return;const auto rate=history.sampleRate();std::array<std::vector<SpectrumBin>,3> traces;
  for(int t=0;t<3;++t)for(size_t k=1;k<fftBins;++k){const auto hz=float(k*rate/fftSize);if(hz>=20&&hz<=20000)traces[t].push_back({hz,history.spectra[t][k]});}
  rack.setSpectrum(std::move(traces),float(std::min(20000.,rate*.5)));
  std::array<juce::Image,3> images;const juce::Colour colours[]{juce::Colour(0xff58bde8),juce::Colour(0xffe3ad60),juce::Colour(0xffcb9cea)};
  for(int t=0;t<3;++t){images[t]=juce::Image(juce::Image::RGB,AnalysisHistory::width,AnalysisHistory::height,true);juce::Image::BitmapData pixels(images[t],juce::Image::BitmapData::writeOnly);for(int x=0;x<AnalysisHistory::width;++x)for(int y=0;y<AnalysisHistory::height;++y){auto colour=juce::Colour(0xff09151b);const auto& col=history.column(x);if(col.valid){const auto v=col.values[t][y];colour=colour.interpolatedWith(t==2&&v<0?colours[0]:colours[t],t==2?std::clamp(std::abs(v)/24.f,0.f,1.f):std::clamp((v+100.f)/100.f,0.f,1.f));}pixels.setPixelColour(x,y,colour);}}
  rack.setHeatmaps(images[0],images[1],images[2]);
 }
 PluginProcessor& p;AssetLoader assets;RackOrder order;juce::Image cap,pointer;RackEditor rack;juce::TextButton scope{"GLOBAL"},delta{"DELTA LISTEN"};uint64_t selected=0,seenEpoch=0;AnalysisHistory history;AnalysisFrame frame;
};
juce::AudioProcessorEditor* PluginProcessor::createEditor(){return new PluginRackEditor(*this);}
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ml::PluginProcessor();}
