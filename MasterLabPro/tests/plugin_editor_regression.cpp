#include <juce_audio_processors/juce_audio_processors.h>
#include "RackTiles.h"
#include "RackState.h"
#include "RotaryParameterControl.h"
#include "masterlabpro_input_compressor.h"
#include "StereoArchitectSettings.h"
#include <memory>
#include <vector>
#include <iostream>
#include <numbers>
#include <tuple>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
extern juce::AudioProcessor* createMasterLabProEditorFixture(const juce::File&);
namespace {
template<class T> T* find(juce::Component& c){
 if(auto* result=dynamic_cast<T*>(&c))return result;
 for(int i=0;i<c.getNumChildComponents();++i)if(auto* result=find<T>(*c.getChildComponent(i)))return result;
 return nullptr;
}
juce::Component* named(juce::Component& c,const juce::String& name){
 if(c.getName()==name)return &c;
 for(int i=0;i<c.getNumChildComponents();++i)if(auto* result=named(*c.getChildComponent(i),name))return result;
 return nullptr;
}
juce::MouseEvent mouse(juce::Component& c,juce::ModifierKeys mods,juce::Point<float> pos={10.f,10.f}){
 const auto now=juce::Time::getCurrentTime();
 return {juce::Desktop::getInstance().getMainMouseSource(),pos,mods,1.f,0.f,0.f,0.f,0.f,&c,&c,now,pos,now,1,false};
}
void click(juce::Component& c){juce::Component* component=&c;const auto point=c.getLocalBounds().toFloat().getCentre();component->mouseDown(mouse(c,juce::ModifierKeys::leftButtonModifier,point));component->mouseUp(mouse(c,juce::ModifierKeys{},point));}
bool saveSnapshot(juce::AudioProcessorEditor& editor,const juce::String& name,float scale=1.f){
 const auto snapshot=editor.createComponentSnapshot(editor.getLocalBounds(),true,scale);
 auto file=juce::File::getCurrentWorkingDirectory().getChildFile(name);
 if(!file.deleteFile())return false;
 juce::FileOutputStream stream(file);
 return snapshot.isValid()&&stream.openedOk()&&juce::PNGImageFormat().writeImageToStream(snapshot,stream);
}
}
int main(int argc,char** argv){
 std::cout<<std::unitbuf;std::cerr<<std::unitbuf;
 std::cout<<"Starting native editor fixture\n";
 juce::ScopedJuceInitialiser_GUI gui;
 auto create=[&]{return std::unique_ptr<juce::AudioProcessor>(argc==2?createMasterLabProEditorFixture(juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[1]))):createPluginFilter());};
 auto p=create();p->prepareToPlay(48000,256);
 std::cout<<"Gain processor prepared\n";
 ml::RackState state;ml::SavedModule m;m.instance=1;m.typeHi=0x4d4c505445434800ull;m.typeLo=0x6761696e00000002ull;m.version=1;m.lane=0;m.slot=0;m.parameters.push_back({1,6});state.modules.push_back(m);
 std::vector<uint8_t> bytes;if(!ml::RackStateCodec::encode(state,bytes))return 1;p->setStateInformation(bytes.data(),int(bytes.size()));
 std::unique_ptr<juce::AudioProcessorEditor> editor(p->createEditor());if(!editor)return 2;
 auto* tiles=find<ml::RackTiles>(*editor);if(!tiles||tiles->getNumChildComponents()!=10)return 3;
 for(int i=0;i<10;++i){auto* card=tiles->getChildComponent(i);if(card->getHeight()<55||card->getY()!=0||(i&&card->getX()<=tiles->getChildComponent(i-1)->getX())){std::cerr<<"Rack card layout invalid\n";return 24;}}
 for(int i=0;i<100;++i){
  auto* tile=tiles->getChildComponent(0);tile->mouseDown(mouse(*tile,juce::ModifierKeys::leftButtonModifier));
  auto* panel=named(*editor,"MasterLabPro Module Panel");if(!panel)return 4;
  auto* knob=find<ml::RotaryParameterControl>(*panel);if(!knob)return 5;
  if(knob->getName()!="Gain"||!knob->valueText().endsWith(" dB")||knob->dialDiameter()<65.f){
   std::cerr<<"Gain knob: "<<knob->getName()<<" / "<<knob->valueText()<<" / "<<knob->dialDiameter()<<'\n';return 7;
  }
  knob->mouseDown(mouse(*knob,juce::ModifierKeys::rightButtonModifier));
  if(knob->valueText()!="0.00 dB"){std::cerr<<"Reset: "<<knob->valueText()<<'\n';return 8;}
 }
 // Empty tile must clear selection and open a menu without dereferencing ID zero.
 if(argc==1){
  auto* empty=tiles->getChildComponent(1);empty->mouseDown(mouse(*empty,juce::ModifierKeys::leftButtonModifier));
  if(named(*editor,"MasterLabPro Module Panel"))return 6;
  juce::PopupMenu::dismissAllActiveMenus();
  std::cout<<"PASS native empty-slot popup\n";
 }
 editor.reset();p->releaseResources();p.reset();
 std::cout<<"Gain processor destroyed before compressor fixture\n";
 std::cout<<"PASS actual plugin editor: 100 selections and knob resets\n";
 std::cout<<"Creating compressor fixture\n";
 auto compressor=create();
 std::cout<<"Compressor processor created\n";
 compressor->prepareToPlay(48000,256);
 state.modules[0].typeHi=MLP_INPUT_COMPRESSOR_HI;
 state.modules[0].typeLo=MLP_INPUT_COMPRESSOR_LO;
 state.modules[0].parameters={{103,-20},{113,3}};
 if(!ml::RackStateCodec::encode(state,bytes))return 9;
 compressor->setStateInformation(bytes.data(),int(bytes.size()));
 editor.reset(compressor->createEditor());
 tiles=find<ml::RackTiles>(*editor);
 if(!tiles)return 10;
 auto* tile=tiles->getChildComponent(0);tile->mouseDown(mouse(*tile,juce::ModifierKeys::leftButtonModifier));
 auto* panel=named(*editor,"Input Compressor Panel");
 if(!panel||!named(*panel,"Stereo Link"))return 11;
 const juce::Component* firstHeaderButton=nullptr;
 for(const auto* name:{"Compressor LOW","Compressor MID","Compressor HIGH","Compressor SUM","Compressor CROSSOVER","Compressor Mode","Compressor Bypass","Compressor Delta Listen","Compressor Module Focus","Compressor Remove"}){
  auto* button=named(*panel,name);
  if(!button){std::cerr<<"Missing compressor header button: "<<name<<'\n';return 29;}
  const auto bounds=button->getBounds();
  if(bounds.isEmpty()||!panel->getLocalBounds().contains(bounds)||bounds.getWidth()<70){
   std::cerr<<"Compressor header button clipped or too small: "<<name<<" bounds="<<bounds.toString()<<'\n';return 30;
  }
  if(!firstHeaderButton)firstHeaderButton=button;
  else if(bounds.getY()!=firstHeaderButton->getY()||bounds.getHeight()!=firstHeaderButton->getHeight()){
   std::cerr<<"Compressor header is not a single aligned row\n";return 31;
  }
 }
 for(const auto* name:{"LOW compression enable","MID compression enable","HIGH compression enable","SUM compression enable"})if(named(*panel,name)){std::cerr<<"Separate ON/OFF button remains: "<<name<<'\n';return 32;}
 auto* lowButton=dynamic_cast<juce::Button*>(named(*panel,"Compressor LOW"));
 const auto compressorParameters=compressor->getParameters();
 auto* lowEnable=compressorParameters.getUnchecked(38);
 if(!lowButton||!lowEnable)return 33;
 juce::Component* lowComponent=lowButton;
 const auto lowButtonPoint=lowButton->getLocalBounds().toFloat().getCentre();
 const auto lowMouseDown=mouse(*lowButton,juce::ModifierKeys::leftButtonModifier,lowButtonPoint);
 const auto lowMouseUp=mouse(*lowButton,juce::ModifierKeys{},lowButtonPoint);
 lowComponent->mouseDown(lowMouseDown);lowComponent->mouseUp(lowMouseUp);
 if(!lowButton->getToggleState()){std::cerr<<"Single click failed to select compressor band\n";return 34;}
 auto* threshold=dynamic_cast<juce::RangedAudioParameter*>(compressorParameters.getUnchecked(6));auto* thresholdControl=named(*panel,"THRESHOLD");
 auto* thresholdKnob=dynamic_cast<ml::RotaryParameterControl*>(thresholdControl);
 if(!threshold||!thresholdKnob)return 56;
 const auto thresholdNeutral=threshold->convertTo0to1(0.f);threshold->setValue(.65f);
 const auto knobPoint=thresholdKnob->getLocalBounds().toFloat().getCentre();
 thresholdKnob->mouseDown(mouse(*thresholdKnob,juce::ModifierKeys::rightButtonModifier,knobPoint));
 if(std::abs(threshold->getValue()-thresholdNeutral)>.001f)return 57;
 const auto beforeAutomation=threshold->getValue();
 thresholdKnob->mouseDown(mouse(*thresholdKnob,juce::ModifierKeys::leftButtonModifier,knobPoint));
 thresholdKnob->mouseDrag(mouse(*thresholdKnob,juce::ModifierKeys::leftButtonModifier,{knobPoint.x,knobPoint.y-24.f}));
 thresholdKnob->mouseUp(mouse(*thresholdKnob,juce::ModifierKeys{},knobPoint));
 if(std::abs(threshold->getValue()-beforeAutomation)<.001f){std::cerr<<"Compressor knob gesture did not update its host parameter\n";return 58;}
 const auto enabledBeforeDoubleClick=lowEnable->getValue();
 const auto doubleClick=mouse(*lowButton,juce::ModifierKeys::leftButtonModifier,lowButton->getLocalBounds().toFloat().getCentre());
 lowComponent->mouseDoubleClick(doubleClick);
 if(lowEnable->getValue()==enabledBeforeDoubleClick){std::cerr<<"Double click failed to toggle compressor band DSP\n";return 35;}
 for(const auto* name:{"Transfer Analyzer Expand","Spectrum Analyzer Expand","Oscilloscope Analyzer Expand"})if(!named(*panel,name))return 36;
 // Exercise mode, delta, and focus controls before rendering from the live signal.
 auto* analogButton=named(*panel,"Compressor Mode");auto* deltaButton=named(*panel,"Compressor Delta Listen");auto* focusButton=named(*panel,"Compressor Module Focus");
 if(!analogButton||!deltaButton||!focusButton)return 43;
 click(*analogButton);if(compressorParameters.getUnchecked(4)->getValue()<.99f)return 44;
 if(!saveSnapshot(*editor,"input-compressor-ANALOG-control.png"))return 45;
 click(*analogButton);if(compressorParameters.getUnchecked(4)->getValue()>.01f)return 46;
 click(*deltaButton);if(dynamic_cast<juce::Button*>(deltaButton)->getButtonText()!="DELTA ON")return 47;
 click(*deltaButton); // keep the signal path out of delta monitoring during the DSP check
 // Selecting the compressor opens its editor in focus so the controls are immediately usable.
 if(panel->getHeight()<500||panel->getY()>12){std::cerr<<"Compressor editor did not open in Module Focus: "<<panel->getBounds().toString()<<'\n';return 49;}
 if(auto* dial=find<ml::RotaryParameterControl>(*panel);!dial||dial->dialDiameter()<75.f){std::cerr<<"Focused compressor dial diameter: "<<(dial?dial->dialDiameter():0.f)<<'\n';return 15;}
 click(*focusButton);
 if(panel->getHeight()>=500){std::cerr<<"Module Focus exit did not return to rack view\n";return 53;}
 click(*focusButton);
 if(panel->getHeight()<500)return 49;
 for(const auto [w,h,label]:{std::tuple<int,int,const char*>{1200,750,"100"},{960,600,"150"},{800,500,"200"}}){
  editor->setSize(w,h);juce::Timer::callPendingTimersSynchronously();
  if(panel->getHeight()<h-25)return 59;
  if(auto* control=find<ml::RotaryParameterControl>(*panel);!control||control->dialDiameter()<50.f)return 60;
  if(!saveSnapshot(*editor,juce::String("input-compressor-resize-")+label+".png"))return 61;
 }
 editor->setSize(1200,750);
 click(*focusButton);
 if(panel->getHeight()>=500){std::cerr<<"Module Focus did not return to rack view\n";return 53;}
 juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
 for(int block=0;block<120;++block){
  for(int ch=0;ch<2;++ch)for(int i=0;i<256;++i)audio.setSample(ch,i,0.5f);
  compressor->processBlock(audio,midi);
 }
 if(audio.getSample(0,255)>.2f)return 12;
 for(int block=0;block<64;++block){
  for(int i=0;i<256;++i){
   const auto time=double(block*256+i)/48000.;
   const auto value=float(.55*std::sin(2*std::numbers::pi*997*time)+.15*std::sin(2*std::numbers::pi*120*time));
   audio.setSample(0,i,value);audio.setSample(1,i,value*.85f);
  }
  compressor->processBlock(audio,midi);
  if(block%8==7){juce::Thread::sleep(35);juce::Timer::callPendingTimersSynchronously();}
 }
 juce::Timer::callPendingTimersSynchronously();
 for(const auto* name:{"Compressor LOW","Compressor MID","Compressor HIGH","Compressor SUM","Compressor CROSSOVER"}){
  auto* button=named(*panel,name);if(!button)return 41;click(*button);
  auto* bandButton=dynamic_cast<juce::Button*>(button);if(!bandButton||!bandButton->getToggleState())return 62;
  const auto bandName=juce::String(name).fromLastOccurrenceOf(" ",false,false);
  if(bandName!="CROSSOVER"){
   const size_t index=bandName=="LOW"?38:bandName=="MID"?39:bandName=="HIGH"?40:41;
   auto* enable=compressorParameters.getUnchecked(int(index));const auto before=enable->getValue();juce::Component* c=button;
   c->mouseDoubleClick(mouse(*button,juce::ModifierKeys::leftButtonModifier,button->getLocalBounds().toFloat().getCentre()));
   if(enable->getValue()==before)return 63;
   if(bandName=="LOW"&&enable->getValue()>.5f&&!saveSnapshot(*editor,"input-compressor-LOW-ON.png"))return 64;
   if(bandName!="LOW"&&!saveSnapshot(*editor,"input-compressor-"+bandName+"-OFF.png"))return 66;
   c->mouseDoubleClick(mouse(*button,juce::ModifierKeys::leftButtonModifier,button->getLocalBounds().toFloat().getCentre()));
   if(enable->getValue()!=before)return 65;
  }
  if(!saveSnapshot(*editor,"input-compressor-"+bandName+".png"))return 42;
 }
 click(*analogButton);if(!saveSnapshot(*editor,"input-compressor-ANALOG.png"))return 45;click(*analogButton);
 if(!saveSnapshot(*editor,"input-compressor-DIGITAL.png"))return 48;
 click(*deltaButton);
 for(int block=0;block<20;++block){for(int i=0;i<256;++i){const auto value=float(.4*std::sin(2*std::numbers::pi*997.*double(block*256+i)/48000.));audio.setSample(0,i,value);audio.setSample(1,i,value*.9f);}compressor->processBlock(audio,midi);if(block%4==3){juce::Thread::sleep(35);juce::Timer::callPendingTimersSynchronously();}}
 if(!saveSnapshot(*editor,"input-compressor-DELTA.png"))return 55;click(*deltaButton);
 click(*focusButton);
 if(!saveSnapshot(*editor,"input-compressor-MODULE-FOCUS-100.png")||!saveSnapshot(*editor,"input-compressor-MODULE-FOCUS-150.png",1.5f)||!saveSnapshot(*editor,"input-compressor-MODULE-FOCUS-200.png",2.f))return 50;
 click(*named(*panel,"Spectrum Analyzer Expand"));if(!saveSnapshot(*editor,"input-compressor-SPECTRUM-ZOOM.png"))return 51;click(*named(*panel,"Spectrum Analyzer Expand"));
 click(*named(*panel,"Oscilloscope Analyzer Expand"));if(!saveSnapshot(*editor,"input-compressor-OSCILLOSCOPE-ZOOM.png"))return 52;click(*named(*panel,"Oscilloscope Analyzer Expand"));
 click(*focusButton);
 if(panel->getHeight()>=500)return 53;
 for(int scale:{100,150,200}){
  auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds(),true,float(scale)/100);
  auto file=juce::File::getCurrentWorkingDirectory().getChildFile("input-compressor-"+juce::String(scale)+".png");
  if(!file.deleteFile())return 13;
  juce::FileOutputStream stream(file);
  if(!snapshot.isValid()||!stream.openedOk()||!juce::PNGImageFormat().writeImageToStream(snapshot,stream))return 14;
 }
 editor.reset();compressor->releaseResources();
 std::cout<<"PASS Input Compressor editor and three scaled renders\n";
 std::cout<<"Creating Stereo Architect fixture processor\n";
 auto stereo=create();
 std::cout<<"Stereo Architect processor created\n";
 stereo->prepareToPlay(48000,256);
 std::cout<<"Stereo Architect processor prepared\n";
 state.modules[0].typeHi=ml::stereoTypeHi;state.modules[0].typeLo=ml::stereoTypeLo;
 state.modules[0].parameters={{1,0},{2,75},{6,140},{7,125}};
 if(!ml::RackStateCodec::encode(state,bytes))return 16;
 std::cout<<"Stereo Architect state encoded\n";
 stereo->setStateInformation(bytes.data(),int(bytes.size()));
 std::cout<<"Stereo Architect state restored\n";
 editor.reset(stereo->createEditor());
 std::cout<<"Stereo Architect editor created\n";
 tiles=find<ml::RackTiles>(*editor);
 if(!tiles)return 17;
 tile=tiles->getChildComponent(0);tile->mouseDown(mouse(*tile,juce::ModifierKeys::leftButtonModifier));
 std::cout<<"Stereo Architect module selected\n";
 auto* stereoPanel=named(*editor,"Stereo Architect Panel");if(!stereoPanel)return 18;
 auto* modeButton=dynamic_cast<juce::Button*>(named(*stereoPanel,"Stereo Mode"));
 auto* stereoizeButton=dynamic_cast<juce::Button*>(named(*stereoPanel,"Stereoize Enable"));
 if(!modeButton||!stereoizeButton||stereo->getParameters().size()<27)return 25;
 juce::Component* modeComponent=modeButton;
 const auto modePoint=modeButton->getLocalBounds().toFloat().getCentre();
 const auto modeDown=mouse(*modeButton,juce::ModifierKeys::leftButtonModifier,modePoint);
 const auto modeUp=mouse(*modeButton,juce::ModifierKeys{},modePoint);
 modeComponent->mouseDown(modeDown);modeComponent->mouseUp(modeUp);
 std::cout<<"Stereo Architect mode button clicked\n";
 auto stereoParameters=stereo->getParameters();
 std::cout<<"Stereo host parameter count: "<<stereoParameters.size()<<"\n";
 if(stereoParameters.size()<27)return 33;
 auto* modeParameter=stereoParameters.getUnchecked(14);
 if(!modeParameter)return 34;
 const auto modeHostValue=modeParameter->getValue();
 std::cout<<"Stereo host mode value: "<<modeHostValue<<"\n";
 if(modeHostValue<.49f||modeHostValue>.51f)return 26;
 juce::Component* stereoizeComponent=stereoizeButton;
 const auto stereoizePoint=stereoizeButton->getLocalBounds().toFloat().getCentre();
 const auto stereoizeDown=mouse(*stereoizeButton,juce::ModifierKeys::leftButtonModifier,stereoizePoint);
 const auto stereoizeUp=mouse(*stereoizeButton,juce::ModifierKeys{},stereoizePoint);
 stereoizeComponent->mouseDown(stereoizeDown);stereoizeComponent->mouseUp(stereoizeUp);
 if(stereoParameters.getUnchecked(22)->getValue()<.99f)return 27;
 std::cout<<"Stereoize button clicked\n";
 const auto bandStateBefore=stereoParameters.getUnchecked(15)->getValue();
 const auto bandTogglePoint=juce::Point<float>(float(stereoPanel->getWidth())*.2f,float(stereoPanel->getHeight())*.44f);
 const auto bandToggle=mouse(*stereoPanel,juce::ModifierKeys::leftButtonModifier,bandTogglePoint);
 stereoPanel->mouseDown(bandToggle);stereoPanel->mouseUp(bandToggle);
 if(stereoParameters.getUnchecked(15)->getValue()==bandStateBefore){std::cerr<<"Stereo per-band switch did not change its host parameter\n";return 28;}
 // Exercise real editor gesture, not just screenshot creation.
 std::array<float,7> widthsBefore{};for(int i=0;i<7;++i)widthsBefore[size_t(i)]=stereoParameters.getUnchecked(i)->getValue();
 const auto point=juce::Point<float>(float(stereoPanel->getWidth())*.39f,float(stereoPanel->getHeight())*.24f);
 const auto down=mouse(*stereoPanel,juce::ModifierKeys::leftButtonModifier,point);
 stereoPanel->mouseDown(down);stereoPanel->mouseUp(down);
 bool changed=false;for(int i=0;i<7;++i)if(stereoParameters.getUnchecked(i)->getValue()!=widthsBefore[size_t(i)])changed=true;
 if(!changed){std::cerr<<"Stereo band mouse gesture did not change a parameter\n";return 21;}
 std::cout<<"PASS Stereo band gesture\n";
 for(int block=0;block<64;++block){
  for(int i=0;i<256;++i){
   const auto time=double(block*256+i)/48000.;
   const auto mid=float(.25*std::sin(2*std::numbers::pi*200*time));
   const auto side=float(.18*std::sin(2*std::numbers::pi*4500*time));
   audio.setSample(0,i,mid+side);audio.setSample(1,i,mid-side);
  }
  stereo->processBlock(audio,midi);
  if(block%8==7){juce::Thread::sleep(35);juce::Timer::callPendingTimersSynchronously();}
 }
 for(int scale:{100,150,200}){
  const auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds(),true,float(scale)/100);
  auto file=juce::File::getCurrentWorkingDirectory().getChildFile("stereo-architect-"+juce::String(scale)+".png");
  if(!file.deleteFile())return 19;
  juce::FileOutputStream stream(file);
  if(!snapshot.isValid()||!stream.openedOk()||!juce::PNGImageFormat().writeImageToStream(snapshot,stream))return 20;
 }
 editor.reset();stereo->releaseResources();
 std::cout<<"PASS Stereo Architect editor and three scaled renders\n";
}
