#pragma once
#include <memory>
#include <vector>
// Fixed-latency post-chain safety stage. All host parameter attachments are real.
class FinalTruePeakPanel final : public juce::Component {
 Processor& processor_;
 StudioTheme& theme_;
 juce::ToggleButton included_{"In chain"},enabled_{"Active"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> includedAttachment_,enabledAttachment_;
 struct Knob {juce::Label name;juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
 std::vector<std::unique_ptr<Knob>> knobs_;
 masterlab::FinalTruePeakReading reading_{};
 void addKnob(const char* caption,const char* parameter){
  auto k=std::make_unique<Knob>();k->name.setText(caption,juce::dontSendNotification);
  k->name.setColour(juce::Label::textColourId,juce::Colour(0xffd4dee6));
  k->slider.setComponentID(parameter);
  k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,23);
  k->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffb3e2ce));
  addAndMakeVisible(k->name);addAndMakeVisible(k->slider);
  k->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,parameter,k->slider);
  knobs_.push_back(std::move(k));
 }
public:
 FinalTruePeakPanel(Processor& p,StudioTheme& t):processor_(p),theme_(t){
  setLookAndFeel(&theme_);setComponentID("final-true-peak-panel");
  included_.setComponentID("true-peak-present");
  enabled_.setComponentID("true-peak-enabled");
  addAndMakeVisible(included_);addAndMakeVisible(enabled_);
  includedAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakPresent",included_);
  enabledAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakOn",enabled_);
  addKnob("Ceiling dBTP (target)","truePeakCeiling");
  addKnob("Release ms","truePeakRelease");
  addKnob("Drive dB","truePeakDrive");
  included_.setTooltip("Adds final limiter after every other DSP stage. Host latency includes fixed 128 samples even when off.");
 }
 ~FinalTruePeakPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& response){reading_=response.truePeak;repaint();}
 void resized()override{
  included_.setBounds(24,16,120,28);enabled_.setBounds(154,16,110,28);
  int width=std::min(190,(getWidth()-60)/3);
  for(size_t i=0;i<knobs_.size();++i){
   const int x=24+static_cast<int>(i)*(width+12);
   knobs_[i]->name.setBounds(x,108,width,24);
   knobs_[i]->slider.setBounds(x,132,width,118);
  }
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff121c1f));
  g.setColour(juce::Colour(0xffb3e2ce));g.setFont(juce::Font(juce::FontOptions(23.f)));
  g.drawText("FINAL TRUE-PEAK",24,56,getWidth()-48,37,juce::Justification::left);
  // Live, mechanically styled gain reduction meter; sourced from DSP
  // telemetry rather than a decorative animation.
  if(getWidth()>=870){
   const float cx=static_cast<float>(getWidth()-172),cy=209.f,radius=92.f;
   const juce::Rectangle<float> bezel(cx-126.f,105.f,252.f,196.f);
   g.setColour(juce::Colour(0xff070b10));g.fillRoundedRectangle(bezel,16.f);
   g.setGradientFill(juce::ColourGradient(juce::Colour(0xff414c55),cx,105.f,
                                        juce::Colour(0xff111920),cx,301.f,false));
   g.fillRoundedRectangle(bezel.reduced(5.f),12.f);
   g.setColour(juce::Colour(0xffe6d5b2));
   g.fillRoundedRectangle(bezel.reduced(13.f),9.f);
   auto polar=[&](float angle,float r){
    return juce::Point<float>(cx+std::sin(angle)*r,cy-std::cos(angle)*r);
   };
   constexpr float a0=-1.13f,a1=1.13f;
   for(int tick=0;tick<=9;++tick){
    const float angle=a0+(a1-a0)*(tick/9.f);
    auto p1=polar(angle,radius*.70f),p2=polar(angle,radius*.84f);
    g.setColour(juce::Colour(0xff51463d));g.drawLine({p1,p2},tick%3==0?1.9f:1.f);
    if(tick%3==0){
     g.setFont(11.f);
     g.drawText(juce::String(tick*2),static_cast<int>(p2.x)-14,static_cast<int>(p2.y)-27,28,18,
                juce::Justification::centred);
    }
   }
   const float reduction=juce::jlimit(0.f,18.f,reading_.gainReductionDb);
   const float angle=a0+(a1-a0)*reduction/18.f;
   const auto tip=polar(angle,radius*.76f);
   g.setColour(juce::Colour(0xff2b333a));g.drawLine({juce::Point<float>(cx+1.f,cy+2.f),tip.translated(1.f,2.f)},3.f);
   g.setColour(juce::Colour(0xffab332b));g.drawLine({juce::Point<float>(cx,cy),tip},2.2f);
   g.setColour(juce::Colour(0xff3a302a));g.fillEllipse(cx-5.f,cy-5.f,10.f,10.f);
   g.setColour(juce::Colour(0xff554738));g.setFont(12.f);
   g.drawText("GAIN REDUCTION  /  dB",static_cast<int>(cx-108.f),266,216,24,
              juce::Justification::centred);
  }
  g.setColour(juce::Colour(0xffd4dee6));g.setFont(14.f);
  const int y=std::max(315,getHeight()-155);
  auto db=[](float x){return juce::String(20.f*std::log10(std::max(1e-9f,x)),2);};
  g.drawText("INPUT TP ESTIMATE  "+db(reading_.detectedTruePeak)+" dBFS"
             "     OUTPUT SAMPLE PEAK  "+db(reading_.outputSamplePeak)+" dBFS"
             "     GAIN REDUCTION  "+juce::String(reading_.gainReductionDb,2)+" dB",
             24,y,getWidth()-48,32,juce::Justification::left);
  g.setColour(juce::Colour(0xff29393a));
  g.fillRoundedRectangle(24.f,static_cast<float>(y+45),static_cast<float>(getWidth()-48),15.f,5.f);
  g.setColour(juce::Colour(0xffb3e2ce));
  g.fillRoundedRectangle(24.f,static_cast<float>(y+45),static_cast<float>(getWidth()-48)*juce::jlimit(0.f,1.f,reading_.gainReductionDb/18.f),15.f,5.f);
  g.setColour(juce::Colour(0xff9dacad));g.setFont(12.f);
  g.drawText("Experimental 4x detector, 128 samples fixed PDC. Reconstructed output ceiling is not yet formally certified.",
             24,y+83,getWidth()-48,30,juce::Justification::left);
 }
};
