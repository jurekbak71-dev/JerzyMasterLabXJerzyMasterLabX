#pragma once
class ProcessingModePanel final : public juce::Component {
 Processor& processor_;StudioTheme& theme_;
 juce::ComboBox mode_;
 juce::Label mixLabel_;
 juce::Slider mix_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach_;
public:
 ProcessingModePanel(Processor& p,StudioTheme& theme):processor_(p),theme_(theme){
  setComponentID("master-insert-panel");setLookAndFeel(&theme_);
  mode_.setComponentID("work-mode");mode_.addItem("MASTER",1);mode_.addItem("INSERT",2);
  addAndMakeVisible(mode_);
  modeAttach_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"workMode",mode_);
  mixLabel_.setText("INSERT processed blend (time aligned)",juce::dontSendNotification);
  mixLabel_.setColour(juce::Label::textColourId,juce::Colours::whitesmoke);
  addAndMakeVisible(mixLabel_);addAndMakeVisible(mix_);
  mix_.setComponentID("insert-blend");mix_.setSliderStyle(juce::Slider::LinearHorizontal);
  mix_.setTextBoxStyle(juce::Slider::TextBoxRight,false,76,28);
  mixAttach_=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,"insertBlend",mix_);
 }
 ~ProcessingModePanel()override{setLookAndFeel(nullptr);}
 void resized()override{
  mode_.setBounds(32,98,220,38);
  mixLabel_.setBounds(32,155,460,30);
  mix_.setBounds(32,188,std::min(620,getWidth()-64),42);
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff20303e));
  g.setColour(juce::Colours::white);g.setFont(juce::FontOptions(24.f));
  g.drawText("PROCESSING MODE  /  MASTER + INSERT",32,35,getWidth()-64,40,juce::Justification::left);
  g.setColour(juce::Colour(0xffaab9c9));g.setFont(juce::FontOptions(14.f));
  g.drawText("MASTER: fully processed chain. INSERT: smooth time-aligned mix with original input, preserving transient character.",
    32,265,getWidth()-64,54,juce::Justification::topLeft);
  const bool activeInsert=processor_.preparedInsertProfile.load(std::memory_order_acquire);
  const bool requestedInsert=processor_.state.getRawParameterValue("workMode")->load()>.5f;
  g.setColour(activeInsert?juce::Colour(0xff82dfcd):juce::Colour(0xffe7bb6a));
  g.drawText(juce::String("ACTIVE PROFILE: ")+(activeInsert?"INSERT / 176 samples at 48 kHz":"MASTER / 416 samples at 48 kHz")+(activeInsert==requestedInsert?"":"  |  requested change applies on audio restart"),
    32,322,getWidth()-64,28,juce::Justification::topLeft);
  g.setColour(juce::Colour(0xffaab9c9));
  g.drawText("The profile is latched only in prepareToPlay. Automation during audio cannot resize delay buffers or change host PDC.",
    32,360,getWidth()-64,60,juce::Justification::topLeft);
 }
};
