#include "RackEditor.h"
#include "AssetLoader.h"
#include <juce_gui_extra/juce_gui_extra.h>

namespace ml {
// Development-only console: no audio processing or host automation.
// The empty rack is deliberate; no fictitious modules are inserted.
class ConsoleContent final : public juce::Component {
public:
 explicit ConsoleContent(AssetLoader& assets)
 : editor(order,assets.loadBands(),{},assets.loadPanel()) {
   addAndMakeVisible(editor);
   setSize(1600,1000);
 }
 void resized() override { editor.setBounds(getLocalBounds()); }
private:
 RackOrder order;
 RackEditor editor;
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
   const auto area=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
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
