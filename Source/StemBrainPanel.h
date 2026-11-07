#pragma once
// Master Brain / 4-stem remixer. Real neural separation is an OFFLINE native
// child process. The plugin callback only sums immutable, time-aligned stems.
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <memory>
#include <thread>
class StemBrainPanel final : public juce::Component,private juce::Timer {
 Processor& processor_;
 StudioTheme& theme_;
 std::array<juce::Label,4> header_{};
 std::array<juce::Slider,4> gain_,tone_,width_;
 std::array<juce::ToggleButton,4> mute_,solo_;
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,12> sliders_{};
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,8> toggles_{};
 juce::ComboBox sourceMode_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment_;
 juce::TextButton split_{"SEPARATE WAV (HTDemucs)"},import_{"LOAD FOUR STEMS"},
                  original_{"ORIGINAL MIX"},natural_{"Natural balance"},
                  vocal_{"Vocal forward"},clear_{"Unload stems"};
 juce::Label state_;
 std::unique_ptr<juce::FileChooser> chooser_;
 std::jthread worker_;
 std::atomic<int> job_{0}; // 0 idle, 1 running, 2 success, negative failure
 juce::File resultDir_;
 juce::String status_{"Choose WAV to separate with HTDemucs, or import aligned real stems."};
 static juce::File engineDirectory(){
  return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
         .getChildFile("JerzyMasterLabX").getChildFile("StemEngine");
 }
 void showStatus(const juce::String& s){
  status_=s;state_.setText(s,juce::dontSendNotification);repaint();
 }
 void chooseSong(){
  if(job_.load()==1)return;
  chooser_=std::make_unique<juce::FileChooser>("Separate a song into four stems",
                                               juce::File{},"*.wav");
  auto safe=juce::Component::SafePointer<StemBrainPanel>(this);
  chooser_->launchAsync(juce::FileBrowserComponent::openMode|
                        juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& f){
   if(!safe)return;
   const auto file=f.getResult();
   if(file.existsAsFile())safe->beginSeparation(file);
  });
 }
 void beginSeparation(juce::File song){
  if(job_.load()==1)return;
  const auto engine=engineDirectory();
  const auto exe=engine.getChildFile("stems-split.exe");
  const auto model=engine.getChildFile("models").getChildFile("htdemucs-42M-v1.0-F16.gguf");
  if(!exe.existsAsFile()||!model.existsAsFile()){
   showStatus("HTDemucs native runtime/model missing. Run INSTALL_STEM_ENGINE.cmd from the plugin ZIP once.");
   return;
  }
  resultDir_=song.getParentDirectory().getChildFile(song.getFileNameWithoutExtension()+"_MasterLabX_stems");
  resultDir_.createDirectory();
  const auto out=resultDir_.getFullPathName();
  const juce::StringArray args{exe.getFullPathName(),"-m",model.getFullPathName(),
                                "-i",song.getFullPathName(),"-o",out,"--float32"};
  if(worker_.joinable())worker_.join();
  job_.store(1);
  showStatus("AI separation running offline (CPU/GPU); original FL Studio playback remains active.");
  worker_=std::jthread([this,args](std::stop_token stop){
   juce::ChildProcess child;
   if(!child.start(args)){job_.store(-2);return;}
   while(child.isRunning()&&!stop.stop_requested())
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
   if(stop.stop_requested()){
    child.kill();job_.store(-4);return;
   }
   job_.store(child.getExitCode()==0?2:-3);
  });
 }
 void chooseFolder(){
  if(job_.load()==1)return;
  chooser_=std::make_unique<juce::FileChooser>("Four aligned HTDemucs WAV stems",
                                               juce::File{},"*");
  auto safe=juce::Component::SafePointer<StemBrainPanel>(this);
  chooser_->launchAsync(juce::FileBrowserComponent::openMode|
                        juce::FileBrowserComponent::canSelectDirectories,[safe](const juce::FileChooser& f){
   if(!safe)return;
   const auto folder=f.getResult();
   if(folder.isDirectory())safe->load(folder);
  });
 }
 void load(const juce::File& directory){
  showStatus("Loading all four WAV stems into memory (not the audio callback)...");
  auto issue=processor_.loadStemsUI(directory);
  if(issue.isNotEmpty()){showStatus(issue);return;}
  const auto bank=processor_.stemRemix.snapshot();
  showStatus("Four real stems loaded: "+juce::String(bank->samples/bank->sampleRate,1)+
             " seconds. Play from start in FL Studio; audition gain/tone/width/solo.");
 }
 void timerCallback()override{
  const auto status=job_.load();
  if(status==1)return;
  if(status==2){
   job_.store(0);
   if(worker_.joinable())worker_.join();
   load(resultDir_);
  }else if(status<0){
   job_.store(0);
   if(worker_.joinable())worker_.join();
   showStatus(status==-2?"Could not launch native HTDemucs helper. Reinstall StemEngine.":
              status==-3?"HTDemucs separation failed. WAV/model/driver issue; review native engine.":"Separation cancelled.");
  }
 }
 void addSlider(juce::Slider& slider,const juce::String& id,
                std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach){
  slider.setSliderStyle(juce::Slider::LinearHorizontal);
  slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,76,24);
  slider.setComponentID(id);
  addAndMakeVisible(slider);
  attach=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,id,slider);
 }
public:
 StemBrainPanel(Processor& p,StudioTheme& t):processor_(p),theme_(t){
  setComponentID("stem-brain-panel");setLookAndFeel(&theme_);
  for(int i=0;i<4;++i){
   header_[i].setText(juce::String(masterlab::stemNames[static_cast<size_t>(i)]).toUpperCase(),
                      juce::dontSendNotification);
   header_[i].setFont(theme_.font(17));header_[i].setColour(juce::Label::textColourId,juce::Colour(0xffc5edff));
   addAndMakeVisible(header_[i]);
   const auto stem=juce::String(std::array<const char*,4>{"Drums","Bass","Other","Vocals"}[i]);
   addSlider(gain_[i],"stem"+stem+"Gain",sliders_[static_cast<size_t>(i*3)]);
   addSlider(tone_[i],"stem"+stem+"Tone",sliders_[static_cast<size_t>(i*3+1)]);
   addSlider(width_[i],"stem"+stem+"Width",sliders_[static_cast<size_t>(i*3+2)]);
   mute_[i].setButtonText("Mute");solo_[i].setButtonText("Solo");
   mute_[i].setComponentID("stem-"+juce::String(masterlab::stemNames[static_cast<size_t>(i)])+"-mute");
   solo_[i].setComponentID("stem-"+juce::String(masterlab::stemNames[static_cast<size_t>(i)])+"-solo");
   addAndMakeVisible(mute_[i]);addAndMakeVisible(solo_[i]);
   toggles_[static_cast<size_t>(i*2)]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
      p.state,"stem"+stem+"Mute",mute_[i]);
   toggles_[static_cast<size_t>(i*2+1)]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
      p.state,"stem"+stem+"Solo",solo_[i]);
  }
  sourceMode_.addItem("DAW mix unchanged",1);
  sourceMode_.addItem("4 stems replace DAW",2);
  sourceMode_.addItem("4 stems + DAW",3);
  sourceMode_.setComponentID("stem-source-mode");
  modeAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
    p.state,"stemMode",sourceMode_);
  for(auto* button:{&split_,&import_,&original_,&natural_,&vocal_,&clear_})
    addAndMakeVisible(button);
  split_.setComponentID("stem-separate");
  import_.setComponentID("stem-import");
  original_.setComponentID("stem-original");
  natural_.setComponentID("stem-variant-a");vocal_.setComponentID("stem-variant-b");
  clear_.setComponentID("stem-unload");
  addAndMakeVisible(sourceMode_);
  state_.setColour(juce::Label::textColourId,juce::Colour(0xffb9ddeb));
  state_.setFont(theme_.font(13));state_.setJustificationType(juce::Justification::centredLeft);
  state_.setText(status_,juce::dontSendNotification);addAndMakeVisible(state_);
  split_.onClick=[this]{chooseSong();};import_.onClick=[this]{chooseFolder();};
  original_.onClick=[this]{
   auto* parameter=processor_.state.getParameter("stemMode");
   parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(0.f));
   parameter->endChangeGesture();
   showStatus("Original mix only; the source audio is unchanged.");
  };
  natural_.onClick=[this]{
   if(!processor_.applyStemVariantUI(0))showStatus("Load stems first.");
   else showStatus("Natural balance active. Undo restores prior proportions.");
  };
  vocal_.onClick=[this]{
   if(!processor_.applyStemVariantUI(1))showStatus("Load stems first.");
   else showStatus("Vocal-forward master remix active. Undo restores prior proportions.");
  };
  clear_.onClick=[this]{processor_.clearStemsUI();showStatus("Stem WAVs unloaded; original DAW mix restored.");};
  startTimerHz(3);
 }
 ~StemBrainPanel()override{
  stopTimer();
  if(worker_.joinable()){worker_.request_stop();worker_.join();}
  setLookAndFeel(nullptr);
 }
 void resized()override{
  const int w=getWidth(),margin=22;
  split_.setBounds(margin,24,215,34);
  import_.setBounds(244,24,160,34);
  original_.setBounds(411,24,145,34);
  sourceMode_.setBounds(568,25,juce::jmax(170,w-590),32);
  state_.setBounds(margin,66,w-44,42);
  const int cell=(w-54)/4;
  for(int i=0;i<4;++i){
   const int x=18+i*cell;
   header_[i].setBounds(x+8,126,cell-16,28);
   gain_[i].setBounds(x+8,186,cell-16,34);
   tone_[i].setBounds(x+8,262,cell-16,34);
   width_[i].setBounds(x+8,338,cell-16,34);
   mute_[i].setBounds(x+8,390,juce::jmax(55,cell/2-12),34);
   solo_[i].setBounds(x+cell/2,390,juce::jmax(55,cell/2-12),34);
  }
  natural_.setBounds(22,juce::jmax(448,getHeight()-80),150,34);
  vocal_.setBounds(180,juce::jmax(448,getHeight()-80),155,34);
  clear_.setBounds(343,juce::jmax(448,getHeight()-80),150,34);
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xffe7edf2));
  g.setColour(juce::Colour(0xff122538));g.fillRoundedRectangle(10.f,9.f,
                           static_cast<float>(getWidth()-20),106.f,12.f);
  g.setColour(juce::Colour(0xff51ccff).withAlpha(.6f));
  g.drawRoundedRectangle(11.f,10.f,static_cast<float>(getWidth()-22),104.f,11.f,1.f);
  const int cell=(getWidth()-54)/4;
  for(int i=0;i<4;++i){
   const float x=18.f+i*cell;
   g.setColour(juce::Colour(0xff0c2034));
   g.fillRoundedRectangle(x,124.f,cell-5.f,320.f,10.f);
   g.setColour(juce::Colour(0xff05b6ff).withAlpha(.70f));
   g.drawRoundedRectangle(x+.5f,124.5f,cell-6.f,319.f,10.f,1.f);
   g.setColour(juce::Colour(0xffb4d5e8));g.setFont(theme_.font(12));
   for(const auto line:std::array<std::pair<const char*,int>,3>{{
       {"GAIN / dB",158},{"TONE / tilt",235},{"WIDTH / M-S",313}}})
    g.drawText(line.first,juce::roundToInt(x)+11,line.second,cell-17,20,juce::Justification::left);
   if(auto bank=processor_.stemRemix.snapshot()){
    g.setFont(theme_.font(12));g.setColour(juce::Colour(0xff75e5ff));
    const float db=juce::Decibels::gainToDecibels(
      bank->rms[static_cast<size_t>(i)],-120.f);
    g.drawText("RMS "+juce::String(db,1)+" dBFS",juce::roundToInt(x)+10,
              445,cell-15,20,juce::Justification::left);
   }
  }
  g.setColour(juce::Colour(0xff32536a));g.setFont(theme_.font(12));
  g.drawFittedText("Real HTDemucs 4-source separation runs offline. Unavoidable bleed/artifacts: compare at matched loudness, check mono and True Peak.",
        22,getHeight()-34,getWidth()-44,22,juce::Justification::centredLeft,1);
 }
};
