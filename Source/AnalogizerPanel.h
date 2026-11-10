#pragma once
// Unified Analog Exciter controls: four harmonic bands plus Tube/FET/Tape colour.
#include <memory>
#include <vector>
class AnalogizerPanel final : public juce::Component {
 Processor& processor_;StudioTheme& theme_;
 juce::ToggleButton present_{"Colour in"},enabled_{"Colour on"},harmonicsPresent_{"Harmonics in"},harmonicsEnabled_{"Harmonics on"};
 juce::ComboBox mode_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> presentAttachment_,enabledAttachment_,harmonicsPresentAttachment_,harmonicsEnabledAttachment_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment_;
 struct Control {
  juce::Label label;juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 std::vector<std::unique_ptr<Control>> controls_;
 masterlab::AnalogizerReading reading_{};
 void add(const char* title,const char* id){
  auto item=std::make_unique<Control>();item->label.setText(title,juce::dontSendNotification);
  item->label.setColour(juce::Label::textColourId,juce::Colour(0xffc0cedb));
  item->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  item->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,24);
  item->slider.setComponentID(id);
  addAndMakeVisible(item->label);addAndMakeVisible(item->slider);
  item->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,id,item->slider);
  controls_.push_back(std::move(item));
 }
public:
 AnalogizerPanel(Processor& processor,StudioTheme& theme):processor_(processor),theme_(theme) {
  setComponentID("analogizer-panel");setLookAndFeel(&theme_);
  addAndMakeVisible(present_);addAndMakeVisible(enabled_);addAndMakeVisible(harmonicsPresent_);addAndMakeVisible(harmonicsEnabled_);addAndMakeVisible(mode_);
  present_.setComponentID("analogizer-present");enabled_.setComponentID("analogizer-enabled");
  mode_.setComponentID("analogizer-model");mode_.addItem("Tube",1);mode_.addItem("FET",2);mode_.addItem("Tape",3);
  presentAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"analogizerPresent",present_);
  enabledAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"analogizerOn",enabled_);
  modeAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor_.state,"analogizerMode",mode_);
  harmonicsPresentAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"exciterPresent",harmonicsPresent_);
  harmonicsEnabledAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"exciterOn",harmonicsEnabled_);
  add("Drive dB","analogizerDrive");add("Warmth","analogizerWarmth");
  add("Dynamic de-fizz","analogizerDeFizz");add("Dry / wet","analogizerMix");
  add("Output dB","analogizerOutput");
  add("Low harmonics","exciterAmountLow");add("Mid harmonics","exciterAmountMid");
  add("High harmonics","exciterAmountHigh");add("Air harmonics","exciterAmountAir");
  add("Even H2","exciterEven");add("Odd H3","exciterOdd");
  add("Bias","analogBias");add("Tone","analogTone");
  add("Dynamics","analogDynamics");add("Impact","analogImpact");
 }
 ~AnalogizerPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& frame){reading_=frame.analogizer;repaint();}
 void resized()override{
  present_.setBounds(24,12,130,30);enabled_.setBounds(162,12,110,30);
  harmonicsPresent_.setBounds(282,12,145,30);harmonicsEnabled_.setBounds(438,12,145,30);
  mode_.setBounds(596,12,170,30);
  const int columns=std::min(5,std::max(2,getWidth()/160));
  const int cell=(getWidth()-48)/columns;
  for(size_t i=0;i<controls_.size();++i){
   const int x=24+(static_cast<int>(i)%columns)*cell,y=102+(static_cast<int>(i)/columns)*130;
   controls_[i]->label.setBounds(x,y,cell-8,22);
   controls_[i]->slider.setBounds(x,y+23,cell-8,96);
  }
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff20303e));g.setColour(juce::Colour(0xffe9c190));
  g.setFont(juce::FontOptions(23.f));g.drawText("ANALOG EXCITER  /  Tube - FET - Tape",24,50,getWidth()-48,34,juce::Justification::left);
  g.setColour(juce::Colours::lightgrey);g.setFont(juce::FontOptions(14.f));
  g.drawText("Input RMS "+juce::String(reading_.inputRms,4)+"   Output RMS "+juce::String(reading_.outputRms,4)+"   Delta RMS "+juce::String(reading_.deltaRms,4),
      24,getHeight()-108,getWidth()-48,24,juce::Justification::left);
  g.drawText("Four-band harmonics plus one colour stage. Character is mutually exclusive; de-fizz is level dependent.",
      24,getHeight()-62,getWidth()-48,24,juce::Justification::left);
 }
};
