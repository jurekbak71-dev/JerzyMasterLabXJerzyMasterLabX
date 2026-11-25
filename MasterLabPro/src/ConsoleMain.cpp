#include "RackEditor.h"
#include "RackController.h"
#include "ModuleRackHost.h"
#include "AnalysisHistory.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "RotaryParameterControl.h"
#include <map>
#include "AssetLoader.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace ml {
// Development console: explicit WAV playback only, no microphone or host automation.
class ModulePanel final : public juce::Component {
public:
 ModulePanel(const MlpModuleDescriptor& descriptor,std::vector<std::unique_ptr<juce::AudioParameterFloat>>& parameters,
             juce::Image cap,juce::Image pointer,std::function<void()> remove) {
   title.setText(juce::String::fromUTF8(descriptor.name),juce::dontSendNotification);title.setJustificationType(juce::Justification::centredLeft);addAndMakeVisible(title);
   for(std::size_t i=0;i<parameters.size();++i){auto control=std::make_unique<RotaryParameterControl>(*parameters[i],float(descriptor.parameters[i].neutral_value),cap,pointer,juce::Colour(0xff58bde8));addAndMakeVisible(*control);knobs.push_back(std::move(control));}
   addAndMakeVisible(removeButton);
   removeButton.onClick=std::move(remove);
 }
 void resized() override {
   auto r=getLocalBounds().reduced(4);
   title.setBounds(r.removeFromTop(26));
   removeButton.setBounds(r.removeFromRight(juce::jmin(140,r.getWidth()/3)).withHeight(28));
   const int width=knobs.empty()?0:juce::jmin(170,r.getWidth()/int(knobs.size()));for(auto& knob:knobs)knob->setBounds(r.removeFromLeft(width));
 }
private:
 juce::Label title;std::vector<std::unique_ptr<RotaryParameterControl>> knobs;
 juce::TextButton removeButton{"REMOVE MODULE"};
};
class ConsoleContent final : public juce::AudioAppComponent, private juce::Timer {
public:
 ConsoleContent(AssetLoader& assets,const std::filesystem::path& modules)
 : cap(assets.loadKnobCap()),pointer(assets.loadKnobPointer()),
   editor(controller.order(),assets.loadBands(),{
   [this](int a,int b){if(controller.move(a,b)){editor.refreshRack();submitConfiguration();}},
   [this](RackOrder::Id id,int pos){if(id)showInstance(id);else chooseModule(pos);},
   [this](RackOrder::Id id){const auto* d=moduleHost.descriptor(id);return d?juce::String::fromUTF8(d->name):juce::String("UNAVAILABLE");},
   [this](RackOrder::Id id,int){editor.setBandFromController(controller.band(id));}
 },assets.loadPanel()) {
   moduleHost.scan(modules);
   editor.setRackSkin(assets.loadSlot());
   addAndMakeVisible(editor);
   addAndMakeVisible(loadButton);addAndMakeVisible(stopButton);addAndMakeVisible(scopeButton);
   scopeButton.onClick=[this]{moduleScope=scopeButton.getToggleState();scopeButton.setButtonText(moduleScope?"MODULE":"GLOBAL");clearAnalysis();submitConfiguration();};
   addAndMakeVisible(deltaButton);deltaButton.setClickingTogglesState(true);
   deltaButton.onClick=[this]{submitConfiguration();};
   scopeButton.setClickingTogglesState(true);
   loadButton.onClick=[this]{chooseWav();};stopButton.onClick=[this]{stopPlayback();};
   addAndMakeVisible(status);status.setText("NO AUDIO",juce::dontSendNotification);
   setSize(1600,1000);
 }
 ~ConsoleContent() override {stopTimer();shutdownAudio();}
 void resized() override {
   editor.setBounds(getLocalBounds());
   const int y=juce::jmax(0,getHeight()*10/1000);
   auto bar=juce::Rectangle<int>(getWidth()*43/100,y+8,getWidth()*56/100,28);
   loadButton.setBounds(bar.removeFromLeft(getWidth()*12/100).reduced(2,0));
   stopButton.setBounds(bar.removeFromLeft(getWidth()*8/100).reduced(2,0));
   scopeButton.setBounds(bar.removeFromLeft(getWidth()*11/100).reduced(2,0));
   deltaButton.setBounds(bar.removeFromLeft(getWidth()*14/100).reduced(2,0));
   status.setBounds(bar);
 }
 void prepareToPlay(int samplesPerBlock,double sampleRate) override {
   // Device may choose a different rate. Never silently change pitch.
   moduleHost.prepare(sampleRate,uint32_t(std::clamp(samplesPerBlock,1,2048)));submitConfiguration();cursor=0;
   mismatch.store(std::abs(sampleRate-fileRate)>0.5,std::memory_order_release);
 }
 void releaseResources() override {}
 void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override {
   if(!info.buffer||info.buffer->getNumChannels()<1)return;
   info.clearActiveBufferRegion();
   if(mismatch.load(std::memory_order_acquire))return;
   const int n=juce::jmin(info.numSamples,source.getNumSamples()-cursor);
   if(n<=0){finished.store(true,std::memory_order_release);return;}
   auto* l=info.buffer->getWritePointer(0,info.startSample);
   auto* r=info.buffer->getNumChannels()>1?info.buffer->getWritePointer(1,info.startSample):nullptr;
   std::copy_n(source.getReadPointer(0,cursor),n,l);
   if(r)std::copy_n(source.getReadPointer(source.getNumChannels()>1?1:0,cursor),n,r);
   moduleHost.process(l,r,static_cast<std::size_t>(n));cursor+=n;
   if(cursor>=source.getNumSamples())finished.store(true,std::memory_order_release);
 }
private:
 void stopPlayback(){
   stopTimer();shutdownAudio();deltaButton.setToggleState(false,juce::dontSendNotification);source.setSize(0,0);
   editor.setGlobalLevels({},{});clearAnalysis();status.setText("STOPPED",juce::dontSendNotification);
 }
 void chooseWav(){
   if(chooser)return;
   chooser=std::make_unique<juce::FileChooser>("Choose WAV for development playback",juce::File{},"*.wav");
   juce::Component::SafePointer<ConsoleContent> safe(this);
   chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
      [safe](const juce::FileChooser& fc){
       if(safe==nullptr)return;
       auto file=fc.getResult();
       juce::MessageManager::callAsync([safe,file]{
        if(safe!=nullptr){safe->chooser.reset();if(file.existsAsFile())safe->playWav(file);}
       });
      });
 }
 void playWav(const juce::File& file){
   stopPlayback();
   juce::AudioFormatManager formats;formats.registerBasicFormats();
   std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
   if(!reader||reader->numChannels<1||reader->numChannels>2||!std::isfinite(reader->sampleRate)||reader->sampleRate<8000||
      reader->sampleRate>192000||reader->lengthInSamples<=0||
      reader->lengthInSamples>reader->sampleRate*60){
    status.setText("WAV: mono/stereo, max 60 s",juce::dontSendNotification);return;
   }
   fileRate=reader->sampleRate;
   source.setSize(static_cast<int>(reader->numChannels),static_cast<int>(reader->lengthInSamples));
   if(!reader->read(&source,0,source.getNumSamples(),0,true,source.getNumChannels()>1)){
    source.setSize(0,0);status.setText("READ FAILED",juce::dontSendNotification);return;
   }
   finished.store(false);mismatch.store(false);
   submitConfiguration();
   setAudioChannels(0,2); // never open microphone
   if(auto* device=deviceManager.getCurrentAudioDevice()){
    if(std::abs(device->getCurrentSampleRate()-fileRate)>0.5){
     auto setup=deviceManager.getAudioDeviceSetup();setup.sampleRate=fileRate;
     auto error=deviceManager.setAudioDeviceSetup(setup,true);
     if(error.isNotEmpty()){stopPlayback();status.setText("DEVICE RATE MISMATCH",juce::dontSendNotification);return;}
    }
   }else{stopPlayback();status.setText("NO AUDIO DEVICE",juce::dontSendNotification);return;}
   status.setText("WAV PLAYBACK",juce::dontSendNotification);startTimerHz(30);timerCallback();
 }
 void timerCallback() override {
   if(mismatch.load(std::memory_order_acquire)){stopPlayback();status.setText("DEVICE RATE MISMATCH",juce::dontSendNotification);return;}
   if(finished.load(std::memory_order_acquire)){stopPlayback();return;}
   submitConfiguration();
   ModuleRackMeters latest,m;bool got=false;while(moduleHost.pollMeters(m)){latest=m;got=true;}
   if(got)editor.setGlobalLevels(latest.input,latest.output);
   updateAnalysis();
 }
 void submitConfiguration(){
   moduleHost.setAnalysis(controller.selection(),moduleScope,deltaButton.getToggleState()&&(!moduleScope||controller.selection()!=0));
   for(auto& [id,state]:ui)for(std::size_t i=0;i<state.parameters.size();++i)moduleHost.setParameter(id,state.descriptor->parameters[i].id,state.parameters[i]->get());
   const auto slots=controller.order().slots();if((graphDirty||slots!=submittedOrder)&&moduleHost.submit(controller.order())){submittedOrder=slots;graphDirty=false;}
 }
 void clearAnalysis(){
   analysis.clear();editor.setSpectrum({},20000);editor.setHeatmaps({},{},{});
 }
 void updateAnalysis(){
   const auto wanted=moduleScope?controller.selection():0;
   if(wanted!=displaySource){clearAnalysis();displaySource=wanted;}
   bool got=false;
   // Bound consumer work per timer tick, never FFT in audio callback.
   for(int i=0;i<4&&moduleHost.pollAnalysis(frame);++i){
    if((moduleScope&&!wanted)||frame.sourceId!=wanted)continue;
    got=analysis.accept(frame)||got;
   }
   if(!got)return;
   const auto rate=analysis.sampleRate();
   std::array<std::vector<SpectrumBin>,3> traces;
   for(auto& trace:traces)trace.reserve(fftBins);
   for(int t=0;t<3;++t)for(std::size_t k=1;k<fftBins;++k){
    const auto hz=float(k*rate/fftSize);
    if(hz>=20&&hz<=20000)traces[t].push_back({hz,analysis.spectra[t][k]});
   }
   editor.setSpectrum(std::move(traces),float(std::min(20000.,rate*.5)));
   std::array<juce::Image,3> images;
   const juce::Colour colours[]{juce::Colour(0xff58bde8),juce::Colour(0xffe3ad60),juce::Colour(0xffcb9cea)};
   for(int t=0;t<3;++t){
    images[t]=juce::Image(juce::Image::RGB,AnalysisHistory::width,AnalysisHistory::height,true);
    juce::Image::BitmapData pixels(images[t],juce::Image::BitmapData::writeOnly);
    for(int x=0;x<AnalysisHistory::width;++x){
     const auto& col=analysis.column(x);
     for(int y=0;y<AnalysisHistory::height;++y){
      auto colour=juce::Colour(0xff09151b);
      if(col.valid){
       float v=col.values[t][y];
       if(t==2)colour=colour.interpolatedWith(v<0?juce::Colour(0xff58bde8):colours[t],std::clamp(std::abs(v)/24.f,0.f,1.f));
       else colour=colour.interpolatedWith(colours[t],std::clamp((v+100.f)/100.f,0.f,1.f));
      }
      pixels.setPixelColour(x,y,colour);
     }
    }
   }
   editor.setHeatmaps(images[0],images[1],images[2]);
 }
 void chooseModule(int pos){
   controller.select(0);editor.setModuleEditor(nullptr);
   juce::PopupMenu menu;int command=1;for(const auto& type:moduleHost.availableTypes())menu.addItem(command++,"Add "+juce::String::fromUTF8(type.descriptor->name));if(command==1)menu.addItem(999,"No compatible modules found",false,false);
   juce::Component::SafePointer<ConsoleContent> safe(this);
   menu.showMenuAsync(juce::PopupMenu::Options(),[safe,pos](int choice){
    if(safe!=nullptr&&choice>0&&choice<999)safe->addModule(pos,std::size_t(choice-1));
   });
 }
 void addModule(int pos,std::size_t type){
   if(pos<0||pos>=10)return;
   auto id=controller.add(static_cast<std::size_t>(pos));if(!id)return;
   if(!moduleHost.add(type,id)){controller.remove(static_cast<std::size_t>(pos));return;}const auto* descriptor=moduleHost.descriptor(id);UiState state;state.descriptor=descriptor;
   for(uint32_t i=0;i<descriptor->parameter_count;++i){const auto& p=descriptor->parameters[i];auto range=juce::NormalisableRange<float>(float(p.minimum),float(p.maximum),float(p.step));state.parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"module_"+juce::String(static_cast<juce::int64>(id))+"_"+juce::String(p.id),1},juce::String::fromUTF8(p.name),range,float(p.default_value),juce::AudioParameterFloatAttributes().withLabel(juce::String::fromUTF8(p.unit?p.unit:""))));}
   ui.emplace(id,std::move(state));graphDirty=true;submitConfiguration();
   showInstance(id);editor.refreshRack();
 }
 void showInstance(RackOrder::Id id){
   auto it=ui.find(id);if(it==ui.end()||!controller.select(id))return;
   editor.setSelection(id);editor.setBandFromController(7);
   editor.setBandSelectionEnabled(false); // Technical Package B modules are full-band.
   juce::Component::SafePointer<ConsoleContent> safe(this);
   editor.setModuleEditor(std::make_unique<ModulePanel>(*it->second.descriptor,it->second.parameters,cap,pointer,[safe,id]{
    juce::MessageManager::callAsync([safe,id]{if(safe!=nullptr)safe->removeModule(id);});
   }));
 }
 void removeModule(RackOrder::Id id){
   const auto& slots=controller.order().slots();
   auto it=std::find(slots.begin(),slots.end(),id);if(it==slots.end())return;
   const auto pos=static_cast<std::size_t>(it-slots.begin());
   if(controller.selection()==id){editor.setModuleEditor(nullptr);editor.setSelection(0);}
   if(controller.remove(pos)){moduleHost.remove(id);ui.erase(id);graphDirty=true;submitConfiguration();}
   editor.refreshRack();
 }
 RackController controller;
 struct UiState{const MlpModuleDescriptor* descriptor=nullptr;std::vector<std::unique_ptr<juce::AudioParameterFloat>> parameters;};
 std::map<RackOrder::Id,UiState> ui;
 juce::Image cap,pointer;
 RackEditor editor; // destroyed before parameters
 ModuleRackHost moduleHost;std::array<RackOrder::Id,10> submittedOrder{};bool graphDirty=true;juce::AudioBuffer<float> source;double fileRate=44100.;int cursor=0;
 std::atomic<bool> finished{false},mismatch{false};
 AnalysisHistory analysis;AnalysisFrame frame;bool moduleScope=false;RackOrder::Id displaySource=0;
 juce::TextButton loadButton{"PLAY WAV"},stopButton{"STOP"},scopeButton{"GLOBAL"},deltaButton{"DELTA LISTEN"};juce::Label status;
 std::unique_ptr<juce::FileChooser> chooser;
};

class ConsoleWindow final : public juce::DocumentWindow {
public:
 explicit ConsoleWindow(std::unique_ptr<ConsoleContent> content)
 : DocumentWindow("Jerzy MasterLabPro - GUI development",juce::Colour(0xff171b21),DocumentWindow::allButtons) {
   setUsingNativeTitleBar(true);
   setContentOwned(content.release(),true);
   setResizable(true,false);
   setResizeLimits(800,500,3200,2000);
   if(auto* c=getConstrainer()) c->setFixedAspectRatio(1.6);
   const auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
   const auto area=display?display->userArea:juce::Rectangle<int>(0,0,1600,1000);
   const int w=juce::jmin(1600,juce::jmin(area.getWidth()-40,int((area.getHeight()-80)*1.6)));
   centreWithSize(juce::jmax(800,w),juce::jmax(500,int(w/1.6)));
   setVisible(true);
 }
 void closeButtonPressed() override {juce::JUCEApplication::getInstance()->systemRequestedQuit();}
};
class ConsoleApp final : public juce::JUCEApplication {
public:
 const juce::String getApplicationName() override {return "Jerzy MasterLabPro GUI";}
 const juce::String getApplicationVersion() override {return "0.1.0-dev";}
 void initialise(const juce::String& commandLine) override {
   auto root=juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("assets");
   const auto args=juce::StringArray::fromTokens(commandLine,true);
   for(int i=0;i+1<args.size();++i)
     if(args[i]=="--assets")root=juce::File(args[i+1].unquoted());
   AssetLoader loader(root);
   auto content=std::make_unique<ConsoleContent>(loader,root.getSiblingFile("modules").getFullPathName().toStdString());
   if(!loader.missingFiles().isEmpty()){
     juce::Logger::writeToLog("Missing GUI assets: "+loader.missingFiles().joinIntoString(", "));
     juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"GUI assets missing",
       "Extract the approved G3.04 assets folder alongside the executable. Missing files: "+juce::String(loader.missingFiles().size()));
   }
   window=std::make_unique<ConsoleWindow>(std::move(content));
 }
 void shutdown() override {window.reset();}
 void systemRequestedQuit() override {quit();}
private:std::unique_ptr<ConsoleWindow> window;
};
}
START_JUCE_APPLICATION(ml::ConsoleApp)
