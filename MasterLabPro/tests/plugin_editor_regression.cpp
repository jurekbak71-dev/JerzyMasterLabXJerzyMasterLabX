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
 editor.reset();p->releaseResources();
 std::cout<<"PASS actual plugin editor: 100 selections and knob resets\n";
 std::cout<<"Creating compressor fixture\n";
 auto compressor=create();
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
 if(auto* dial=find<ml::RotaryParameterControl>(*panel);!dial||dial->dialDiameter()<75.f){std::cerr<<"Compressor dial diameter: "<<(dial?dial->dialDiameter():0.f)<<'\n';return 15;}
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
 for(int scale:{100,150,200}){
  auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds(),true,float(scale)/100);
  auto file=juce::File::getCurrentWorkingDirectory().getChildFile("input-compressor-"+juce::String(scale)+".png");
  if(!file.deleteFile())return 13;
  juce::FileOutputStream stream(file);
  if(!snapshot.isValid()||!stream.openedOk()||!juce::PNGImageFormat().writeImageToStream(snapshot,stream))return 14;
 }
 editor.reset();compressor->releaseResources();
 std::cout<<"PASS Input Compressor editor and three scaled renders\n";
 auto stereo=create();
 stereo->prepareToPlay(48000,256);
 state.modules[0].typeHi=ml::stereoTypeHi;state.modules[0].typeLo=ml::stereoTypeLo;
 state.modules[0].parameters={{1,0},{2,75},{6,140},{7,125}};
 if(!ml::RackStateCodec::encode(state,bytes))return 16;
 stereo->setStateInformation(bytes.data(),int(bytes.size()));
 editor.reset(stereo->createEditor());
 tiles=find<ml::RackTiles>(*editor);
 if(!tiles)return 17;
 tile=tiles->getChildComponent(0);tile->mouseDown(mouse(*tile,juce::ModifierKeys::leftButtonModifier));
 auto* stereoPanel=named(*editor,"Stereo Architect Panel");if(!stereoPanel)return 18;
 // Exercise real editor gesture, not just screenshot creation.
 std::array<float,7> widthsBefore{};for(int i=0;i<7;++i)widthsBefore[size_t(i)]=stereo->getParameters()[i]->getValue();
 const auto point=juce::Point<float>(float(stereoPanel->getWidth())*.59f,float(stereoPanel->getHeight())*.24f);
 const auto down=mouse(*stereoPanel,juce::ModifierKeys::leftButtonModifier,point);
 stereoPanel->mouseDown(down);stereoPanel->mouseUp(down);
 bool changed=false;for(int i=0;i<7;++i)if(stereo->getParameters()[i]->getValue()!=widthsBefore[size_t(i)])changed=true;
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
