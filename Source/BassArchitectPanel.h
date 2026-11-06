#pragma once
#include <memory>
#include <vector>
// Post-graph Bass Architect panel, separate from the six-family routing UI.
class BassArchitectPanel final : public juce::Component {
 Processor& processor_;
 StudioTheme& theme_;
 juce::ToggleButton present_{"In chain"},enabled_{"Active"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> presentAttachment_,enabledAttachment_;
 struct Knob {
  juce::Label label;
  juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 std::vector<std::unique_ptr<Knob>> knobs_;
 masterlab::BassArchitectReading reading_{};
 void knob(const char* title,const char* id){
  auto k=std::make_unique<Knob>();
  k->label.setText(title,juce::dontSendNotification);
  k->label.setColour(juce::Label::textColourId,juce::Colour(0xffc2dce8));
  k->slider.setComponentID(id);
  k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,84,22);
  k->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xff64c8ef));
  addAndMakeVisible(k->label);addAndMakeVisible(k->slider);
  k->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,id,k->slider);
  knobs_.push_back(std::move(k));
 }
public:
 BassArchitectPanel(Processor& processor,StudioTheme& theme):processor_(processor),theme_(theme){
  setComponentID("bass-architect-panel");
  setLookAndFeel(&theme_);
  present_.setComponentID("bass-present");enabled_.setComponentID("bass-enabled");
  addAndMakeVisible(present_);addAndMakeVisible(enabled_);
  presentAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"bassPresent",present_);
  enabledAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"bassOn",enabled_);
  knob("Sub level","bassSub");knob("Bass body","bassBody");
  knob("Harmonics","bassHarmonics");knob("Mono sub","bassMono");
  knob("Dry / Wet","bassMix");knob("Output dB","bassOutput");
  present_.setTooltip("Optional final post-chain Bass Architect processor. Off in legacy presets.");
  enabled_.setTooltip("Enables the bass processing without changing the six-module routing order.");
 }
 ~BassArchitectPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& r){reading_=r.bass;repaint();}
 void resized()override{
  present_.setBounds(24,12,130,34);enabled_.setBounds(162,12,116,34);
  const int cols=std::max(3,std::min(6,getWidth()/150));
  const int width=(getWidth()-48)/cols;
  for(size_t i=0;i<knobs_.size();++i){
   const int x=24+static_cast<int>(i%cols)*width,y=110+static_cast<int>(i/cols)*138;
   knobs_[i]->label.setBounds(x,y,width-8,22);
   knobs_[i]->slider.setBounds(x,y+24,width-8,110);
  }
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xffedf0f2));
  // Concept C powder-coated sheet-metal side rails and inset dark instrument panel.
  const float panelWidth=juce::jmax(1.f,static_cast<float>(getWidth())-20.f);
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff344553),10.f,0.f,
                                        juce::Colour(0xff111b25),10.f,static_cast<float>(getHeight()),false));
  g.fillRect(10.f,0.f,panelWidth,static_cast<float>(getHeight()));
  g.setColour(juce::Colour(0xff96a7b6));
  g.drawVerticalLine(10,0.f,static_cast<float>(getHeight()));
  g.drawVerticalLine(juce::jmax(10,getWidth()-11),0.f,static_cast<float>(getHeight()));
  g.setColour(juce::Colour(0xff72d1ef));g.setFont(juce::Font(juce::FontOptions(23.f)));
  g.drawText("BASS ARCHITECT",24,56,getWidth()-48,35,juce::Justification::left);
  g.setColour(juce::Colour(0xffd0dfeb));g.setFont(13.f);
  const int y=std::max(330,getHeight()-126);
  g.drawText("DSP RMS  IN "+juce::String(reading_.inputRms,4)+"   OUT "+juce::String(reading_.outputRms,4)+
     "   DELTA "+juce::String(reading_.deltaRms,4)+"   LOW "+juce::String(reading_.lowRms,4),
     24,y,getWidth()-48,28,juce::Justification::left);
  g.setColour(juce::Colour(0xff2b4254));g.fillRoundedRectangle(24.f,static_cast<float>(y+33),static_cast<float>(getWidth()-48),12.f,4.f);
  g.setColour(juce::Colour(0xff72d1ef));
  g.fillRoundedRectangle(24.f,static_cast<float>(y+33),static_cast<float>(getWidth()-48)*juce::jlimit(0.f,1.f,reading_.deltaRms*5.f),12.f,4.f);
  g.setColour(juce::Colour(0xff8da7b9));g.setFont(12.f);
  g.drawText("Post-limiter processing can exceed the ceiling. Sub stereo-to-mono may reduce anti-phase bass.",
   24,y+57,getWidth()-48,26,juce::Justification::left);
 }
};
