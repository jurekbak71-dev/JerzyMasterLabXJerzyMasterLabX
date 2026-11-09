#pragma once
// Controls for actual restoration DSP; all values are stored in APVTS.
#include <array>
#include <memory>
class RestorationPanel final : public juce::Component {
 Processor& processor_;StudioTheme& theme_;
 struct Toggle {
  juce::ToggleButton button;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
 };
 struct Knob {
  juce::Label label;juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 std::array<Toggle,10> toggles_;
 std::array<Knob,5> knobs_;
 juce::ComboBox humHz_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> humAttachment_;
 masterlab::RestorationReading reading_{};
public:
 RestorationPanel(Processor& p,StudioTheme& t):processor_(p),theme_(t){
  setComponentID("audio-restoration-panel");setLookAndFeel(&theme_);
  constexpr std::array<const char*,10> ids{"restorationPresent","restorationOn","restoreHiss","restoreHum","restoreClick","restoreClip","restoreLearn","restoreFreeze","restoreDelta","restoreABDry"};
  constexpr std::array<const char*,10> labels{"In chain","Active","De-Hiss","De-Hum","De-Click","De-Clip","Learn noise","Freeze print","DELTA","A/B dry"};
  for(size_t i=0;i<ids.size();++i){
   toggles_[i].button.setButtonText(labels[i]);toggles_[i].button.setComponentID(ids[i]);
   addAndMakeVisible(toggles_[i].button);
   toggles_[i].attachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,ids[i],toggles_[i].button);
  }
  addAndMakeVisible(humHz_);humHz_.addItem("50 Hz mains",1);humHz_.addItem("60 Hz mains",2);
  humAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor_.state,"restoreHumHz",humHz_);
  constexpr std::array<const char*,5> param{"restoreThreshold","restoreReduction","restoreQuality","restoreProtect","restoreMix"};
  constexpr std::array<const char*,5> names{"Threshold dBFS","Max reduction dB","Smoothing/quality","Artifact protect","Dry / wet"};
  for(size_t i=0;i<knobs_.size();++i){
   auto& knob=knobs_[i];knob.label.setText(names[i],juce::dontSendNotification);
   knob.label.setColour(juce::Label::textColourId,juce::Colours::whitesmoke);
   knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
   knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,24);
   knob.slider.setComponentID(param[i]);addAndMakeVisible(knob.label);addAndMakeVisible(knob.slider);
   knob.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,param[i],knob.slider);
  }
 }
 ~RestorationPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& frame){reading_=frame.restoration;repaint();}
 void resized()override{
  const int columns=5;const int cw=(getWidth()-48)/columns;
  for(size_t i=0;i<toggles_.size();++i)
   toggles_[i].button.setBounds(24+static_cast<int>(i%columns)*cw,86+static_cast<int>(i/columns)*40,cw-8,32);
  humHz_.setBounds(24,180,174,30);
  for(size_t i=0;i<knobs_.size();++i){
   const int x=24+static_cast<int>(i)*cw;
   knobs_[i].label.setBounds(x,232,cw-10,25);
   knobs_[i].slider.setBounds(x,260,cw-10,104);
  }
 }
 void paint(juce::Graphics& g)override {
  g.fillAll(juce::Colour(0xff1e2b38));g.setColour(juce::Colour(0xff82dfcd));
  g.setFont(juce::FontOptions(23.f));
  g.drawText("AUDIO RESTORATION / signal restoration",24,24,getWidth()-48,40,juce::Justification::left);
  g.setFont(juce::FontOptions(14.f));g.setColour(juce::Colours::whitesmoke);
  g.drawText("Input RMS "+juce::String(reading_.inputRms,4)+"  Output RMS "+juce::String(reading_.outputRms,4)+"  Removed RMS "+juce::String(reading_.removedRms,4),
   24,getHeight()-128,getWidth()-48,24,juce::Justification::left);
  g.drawText("Repaired impulses "+juce::String(reading_.impulsesRepaired)+"   Approx. de-clipped plateaus "+juce::String(reading_.plateausReconstructed)+
     (reading_.learned?"   Noise print learned":"   No noise print"),
    24,getHeight()-96,getWidth()-48,25,juce::Justification::left);
  g.setColour(juce::Colour(0xffaab9c9));
  g.drawText("Experimental: four-region noise print, harmonic notches, causal click correction and approximate de-clip. Inspect DELTA for artefacts.",
    24,getHeight()-64,getWidth()-48,30,juce::Justification::left);
 }
};
