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
 juce::ToggleButton live_{"LIVE NEURAL"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> liveAttachment_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment_;
 juce::TextButton split_{"SEPARATE WAV (HTDemucs)"},import_{"LOAD FOUR STEMS"},
                  original_{"ORIGINAL MIX"},natural_{"Natural balance"},
                  vocal_{"Vocal forward"},clear_{"Unload stems"},diagnose_{"DIAGNOSE"};
 juce::Label state_;
 std::unique_ptr<juce::FileChooser> chooser_;
 std::jthread worker_;
 std::atomic<int> job_{0}; // 0 idle, 1 running, 2 success, negative failure
 juce::File resultDir_;
 juce::String status_{"Choose WAV to separate with HTDemucs, or import aligned real stems."};
 bool lastLiveWanted_{false};
 static juce::File engineDirectory(){
  return juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
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
   showStatus("SEPARATE WAV unavailable: offline engine/model not installed.");
   juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
      "HTDemucs offline separator unavailable",
      "The LIVE neural model does not separate imported WAV files offline. "
      "Run INSTALL_OFFLINE_STEMS.cmd from the MasterLabX package once, "
      "then choose SEPARATE WAV again. The installer downloads and checks "
      "the upstream stems.cpp engine and HTDemucs model.");
   return;
  }
  resultDir_=song.getParentDirectory().getChildFile(song.getFileNameWithoutExtension()+"_MasterLabX_stems");
  if(resultDir_.createDirectory().failed()){
   showStatus("SEPARATE WAV ERROR: cannot create output folder near selected WAV.");
   return;
  }
  const auto out=resultDir_.getFullPathName();
  const juce::StringArray args{exe.getFullPathName(),"-m",model.getFullPathName(),
                                "-i",song.getFullPathName(),"-o",out,"--float32"};
  if(worker_.joinable())worker_.join();
  job_.store(1);
  showStatus("HTDemucs offline: processing the selected WAV into four files. This can take time; result folder: "+resultDir_.getFileName());
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
  showStatus("FOUR WAV STEMS LOADED ("+juce::String(bank->samples/bank->sampleRate,1)+
             " s). LIVE OFF; press PLAY in FL Studio from 0:00 to hear the four WAV stems.");
 }
 void timerCallback()override{
  // Synchronize the visible status with actual backend readiness rather than
  // claiming LIVE is active immediately after a button click.
  const bool liveWanted=processor_.state.getRawParameterValue("stemLiveOn")->load()>.5f;
  sourceMode_.setEnabled(!liveWanted);
  if(liveWanted){
   if(!processor_.liveNeuralAvailable())processor_.requestLiveNeuralUI();
   const auto neuralStatus=processor_.liveNeuralStatusUI();
   if(status_!=neuralStatus)showStatus(neuralStatus);
  }else if(lastLiveWanted_){
   showStatus("LIVE OFF: original DAW stream. Offline WAV mode can be selected.");
  }
  lastLiveWanted_=liveWanted;
  const auto status=job_.load();
  if(status==1)return;
  if(status==2){
   job_.store(0);
   if(worker_.joinable())worker_.join();
   load(resultDir_);
  }else if(status<0){
   job_.store(0);
   if(worker_.joinable())worker_.join();
   showStatus(status==-2?"SEPARATE WAV: failed to launch stems-split.exe (offline engine missing or blocked).":
              status==-3?"SEPARATE WAV: helper exited with error; verify input WAV, model and helper diagnostics.":"SEPARATE WAV cancelled.");
  }
 }
 void diagnoseStems(){
  if(processor_.state.getRawParameterValue("stemLiveOn")->load()>.5f){
   auto report=processor_.liveNeuralStatusUI();
   report+="\n\nA positive WET count alone does not qualify real-time throughput.";
   report+="\nAt 44.1 kHz / 128 samples, the audio deadline is about 2.90 ms.";
   report+="\nCompare hops/output/WET during uninterrupted playback; stopped WET and growing output means results arrive too late or out of alignment.";
   juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
      "STEMS LIVE - worker diagnostics",report);
   return;
  }
  auto bank=processor_.stemRemix.snapshot();
  if(!bank){showStatus("Separate or import four stems first.");return;}
  juce::String report("MASTER BRAIN — measured STEM diagnostics\n");
  report+="Source: "+bank->folder+"\n";
  report+="Duration "+juce::String(bank->samples/bank->sampleRate,1)+
          " s. Numeric RMS is unweighted dBFS, not LUFS.\n\n";
  bool issues=false;
  for(int i=0;i<4;++i){
   const auto name=juce::String(masterlab::stemNames[static_cast<size_t>(i)]).toUpperCase();
   report+=name+": RMS "+juce::String(juce::Decibels::gainToDecibels(
      bank->rms[static_cast<size_t>(i)],-120.f),1)+
      " dBFS | peak "+juce::String(juce::Decibels::gainToDecibels(
      bank->peak[static_cast<size_t>(i)],-120.f),1)+
      " dBFS | crest "+juce::String(bank->crestDb[static_cast<size_t>(i)],1)+
      " dB | stereo r "+juce::String(bank->stereoCorrelation[static_cast<size_t>(i)],2)+"\n";
   if(bank->nearClip[static_cast<size_t>(i)]>bank->samples/1000){
    issues=true;report+=" - Many samples at/near full scale: investigate clipping and limiter drive.\n";
   }
   if(bank->crestDb[static_cast<size_t>(i)]<4.f&&bank->peak[static_cast<size_t>(i)]>.3f){
    issues=true;report+=" - Very low crest contrast; check whether intentionally dense or over-limited.\n";
   }
   if(bank->stereoCorrelation[static_cast<size_t>(i)]<-.1f){
    issues=true;report+=" - Negative L/R correlation: check mono fold-down and phase.\n";
   }
  }
  if(bank->rms[3]>1.e-6f&&bank->rms[2]>bank->rms[3]*2.25f){
   issues=true;report+="\n - Other instruments exceed vocal RMS by >7 dB; vocal-forward A/B may help.\n";
  }
  if(bank->rms[0]>1.e-6f&&bank->rms[1]>bank->rms[0]*2.f){
   issues=true;report+="\n - Bass exceeds drum RMS by >6 dB; verify sub-frequency balance.\n";
  }
  if(!issues)report+="\nNo severe problems detected by these limited objective checks.\n";
  report+="\nAvoid aggressive isolation gain: stem leakage and transient artifacts are model limitations."
          "\nCheck output True Peak, LUFS, and A/B at matched listening level. These checks do not reconstruct lost audio.";
  juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
                                         "Stem Brain — objective mastering diagnostics",report);
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
  // This is an OFFLINE WAV routing selector, not the neural LIVE source.
  // Disable it while LIVE is selected to avoid misleading the user.
  sourceMode_.setComponentID("stem-source-mode");
  modeAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
    p.state,"stemMode",sourceMode_);
  for(auto* button:{&split_,&import_,&original_,&natural_,&vocal_,&clear_,&diagnose_})
    addAndMakeVisible(button);
  split_.setComponentID("stem-separate");
  import_.setComponentID("stem-import");
  original_.setComponentID("stem-original");
  natural_.setComponentID("stem-variant-a");vocal_.setComponentID("stem-variant-b");
  clear_.setComponentID("stem-unload");diagnose_.setComponentID("stem-diagnose");
  addAndMakeVisible(sourceMode_);
  live_.setComponentID("stem-live-neural");addAndMakeVisible(live_);
  liveAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
    p.state,"stemLiveOn",live_);
  live_.onClick=[this]{
   processor_.requestLiveNeuralUI();
   showStatus(processor_.liveNeuralStatusUI());
  };
  state_.setColour(juce::Label::textColourId,juce::Colour(0xffb9ddeb));
  state_.setFont(theme_.font(13));state_.setJustificationType(juce::Justification::centredLeft);
  state_.setText(status_,juce::dontSendNotification);addAndMakeVisible(state_);
  split_.onClick=[this]{chooseSong();};import_.onClick=[this]{chooseFolder();};
  original_.onClick=[this]{
   // Original means ORIGINAL: disable neural LIVE as well as offline stem routing.
   auto* neural=processor_.state.getParameter("stemLiveOn");
   if(neural->getValue()>.5f){
    neural->beginChangeGesture();neural->setValueNotifyingHost(0.f);
    neural->endChangeGesture();processor_.requestLiveNeuralUI();
   }
   auto* parameter=processor_.state.getParameter("stemMode");
   parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(0.f));
   parameter->endChangeGesture();
   showStatus("ORIGINAL: LIVE off, offline stems off; source DAW mix unchanged.");
  };
  natural_.onClick=[this]{
   if(!processor_.applyStemVariantUI(0))showStatus("Load four offline WAV stems, or enable LIVE first.");
   else showStatus("Natural: LIVE uses neutral stem gain; offline uses measured balancing.");
  };
  vocal_.onClick=[this]{
   if(!processor_.applyStemVariantUI(1))showStatus("Load four offline WAV stems, or enable LIVE first.");
   else showStatus("Vocal forward: vocal gain +2 dB, backing slightly reduced (LIVE).");
  };
  clear_.onClick=[this]{
   auto* liveParam=processor_.state.getParameter("stemLiveOn");
   if(liveParam->getValue()>.5f){
    liveParam->beginChangeGesture();liveParam->setValueNotifyingHost(0.f);
    liveParam->endChangeGesture();processor_.requestLiveNeuralUI();
   }
   processor_.clearStemsUI();
   showStatus("UNLOAD STEMS: LIVE stopped, offline WAV memory freed, DAW source restored.");
  };
  diagnose_.onClick=[this]{diagnoseStems();};
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
  live_.setBounds(568,24,116,34);
  sourceMode_.setBounds(692,25,juce::jmax(100,w-714),32);
  state_.setBounds(margin,64,w-44,56);
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
  diagnose_.setBounds(503,juce::jmax(448,getHeight()-80),130,34);
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
  const auto footer=processor_.liveNeuralAvailable()
       ? "LIVE (44.1 kHz) + offline HTDemucs. Check latency, mono and True Peak."
       : "HTDemucs offline; LIVE requires StemgenRT noncommercial models, 44.1 kHz and a Live VST3 build.";
   g.drawFittedText(footer,
        22,getHeight()-34,getWidth()-44,22,juce::Justification::centredLeft,1);
 }
};
