#include "RackEditor.h"
#include "RackController.h"
#include "RotaryParameterControl.h"
#include <map>
#include "AssetLoader.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace ml {
// Development-only console: parameter and rack configuration test, no audio device.
// GainDSP is tested separately; the console does not yet stream audio.
class GainPanel final : public juce::Component {
public:
 GainPanel(juce::RangedAudioParameter& p,juce::Image cap,juce::Image pointer,
           std::function<void()> remove)
 : knob(p,0.f,std::move(cap),std::move(pointer),juce::Colour(0xff58bde8)) {
   addAndMakeVisible(knob);
   addAndMakeVisible(removeButton);
   removeButton.onClick=std::move(remove);
 }
 void resized() override {
   auto r=getLocalBounds().reduced(4);
   removeButton.setBounds(r.removeFromRight(juce::jmin(140,r.getWidth()/3)).withHeight(28));
   knob.setBounds(r.removeFromLeft(juce::jmin(170,r.getWidth())));
 }
private:
 RotaryParameterControl knob;
 juce::TextButton removeButton{"REMOVE GAIN"};
};
class ConsoleContent final : public juce::Component {
public:
 explicit ConsoleContent(AssetLoader& assets)
 : cap(assets.loadKnobCap()),pointer(assets.loadKnobPointer()),
   editor(controller.order(),assets.loadBands(),{
   [this](int a,int b){if(controller.move(a,b))editor.refreshRack();},
   [this](RackOrder::Id id,int pos){if(id)showInstance(id);else chooseModule(pos);},
   [this](RackOrder::Id id){return params.count(id)?juce::String("GAIN [TEST]"):juce::String("UNKNOWN");},
   [this](RackOrder::Id id,int){editor.setBandFromController(controller.band(id));}
 },assets.loadPanel()) {
   editor.setRackSkin(assets.loadSlot());
   addAndMakeVisible(editor);
   setSize(1600,1000);
 }
 void resized() override { editor.setBounds(getLocalBounds()); }
private:
 void chooseModule(int pos){
   controller.select(0);editor.setModuleEditor(nullptr);
   juce::PopupMenu menu;menu.addItem(1,"Add Gain (full-band test)");
   juce::Component::SafePointer<ConsoleContent> safe(this);
   menu.showMenuAsync(juce::PopupMenu::Options(),[safe,pos](int choice){
    if(safe!=nullptr&&choice==1)safe->addGain(pos);
   });
 }
 void addGain(int pos){
   if(pos<0||pos>=10)return;
   auto id=controller.add(static_cast<std::size_t>(pos));if(!id)return;
   params.emplace(id,std::make_unique<juce::AudioParameterFloat>(
     juce::ParameterID{"gain_"+juce::String(static_cast<juce::int64>(id)),1},
     "Gain",juce::NormalisableRange<float>(-24.f,24.f,.01f),0.f,
     juce::AudioParameterFloatAttributes().withLabel("dB")));
   showInstance(id);editor.refreshRack();
 }
 void showInstance(RackOrder::Id id){
   auto it=params.find(id);if(it==params.end()||!controller.select(id))return;
   editor.setSelection(id);editor.setBandFromController(7);
   editor.setBandSelectionEnabled(false); // Gain is intentionally full-band only.
   juce::Component::SafePointer<ConsoleContent> safe(this);
   editor.setModuleEditor(std::make_unique<GainPanel>(*it->second,cap,pointer,[safe,id]{
    juce::MessageManager::callAsync([safe,id]{if(safe!=nullptr)safe->removeGain(id);});
   }));
 }
 void removeGain(RackOrder::Id id){
   const auto& slots=controller.order().slots();
   auto it=std::find(slots.begin(),slots.end(),id);if(it==slots.end())return;
   const auto pos=static_cast<std::size_t>(it-slots.begin());
   if(controller.selection()==id){editor.setModuleEditor(nullptr);editor.setSelection(0);}
   if(controller.remove(pos))params.erase(id);
   editor.refreshRack();
 }
 RackController controller;
 std::map<RackOrder::Id,std::unique_ptr<juce::AudioParameterFloat>> params;
 juce::Image cap,pointer;
 RackEditor editor; // destroyed before parameters
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
   auto content=std::make_unique<ConsoleContent>(loader);
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
