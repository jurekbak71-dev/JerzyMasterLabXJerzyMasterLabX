#pragma once
#include <memory>
#include <vector>
// Fixed-latency post-chain safety stage. All host parameter attachments are real.
class FinalTruePeakPanel final : public juce::Component {
 Processor& processor_;
 StudioTheme& theme_;
 juce::ToggleButton included_{"Fixed slot"},enabled_{"Active"},autoRelease_{"Auto release"};
 juce::ComboBox character_,oversampling_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> includedAttachment_,enabledAttachment_,autoReleaseAttachment_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment_,oversamplingAttachment_;
 struct Knob {juce::Label name;juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
 std::vector<std::unique_ptr<Knob>> knobs_;
 double scopeRate_=48000.;
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
  addAndMakeVisible(included_);addAndMakeVisible(enabled_);included_.setEnabled(false);
  addAndMakeVisible(character_);addAndMakeVisible(autoRelease_);
  addAndMakeVisible(oversampling_);oversampling_.setComponentID("true-peak-oversampling");
  for(const char* label:{"1x","2x","4x"})oversampling_.addItem(label,oversampling_.getNumItems()+1);
  oversamplingAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"truePeakOversampling",oversampling_);
  character_.setComponentID("true-peak-character");
  for(const char* label:{"Transparent","Punch","Aggressive"})character_.addItem(label,character_.getNumItems()+1);
  characterAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"truePeakCharacter",character_);
  autoRelease_.setComponentID("true-peak-auto-release");
  autoReleaseAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakAutoRelease",autoRelease_);
  includedAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakPresent",included_);
  enabledAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakOn",enabled_);
  addKnob("Ceiling dBTP (target)","truePeakCeiling");
  addKnob("Release ms","truePeakRelease");
  addKnob("Drive dB","truePeakDrive");
  addKnob("Output trim dB","truePeakOutput");
  addKnob("Soft Clip","truePeakSoftClip");
  addKnob("Attack ms","truePeakAttack");
  addKnob("Lookahead smp","truePeakLookahead");
  addKnob("Transient","truePeakTransientEmphasis");
  included_.setTooltip("Final limiter occupies the last slot; Active switches it OFF without changing host PDC.");
 }
 ~FinalTruePeakPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& response){reading_=response.truePeak;scopeRate_=response.rate;repaint();}
 void resized()override{
  included_.setBounds(24,12,130,34);enabled_.setBounds(162,12,116,34);
  character_.setBounds(295,12,150,32);oversampling_.setBounds(457,12,82,32);autoRelease_.setBounds(551,12,132,32);
  int width=std::min(116,(getWidth()-132)/static_cast<int>(knobs_.size()));
  for(size_t i=0;i<knobs_.size();++i){
   const int x=24+static_cast<int>(i)*(width+12);
   const bool compact=getHeight()<440;
   knobs_[i]->name.setBounds(x,compact?92:108,width,22);
   knobs_[i]->slider.setBounds(x,compact?115:132,width,compact?78:118);
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
  g.setColour(juce::Colour(0xffb3e2ce));g.setFont(juce::Font(juce::FontOptions(23.f)));
  g.drawText("FINAL MAXIMIZER / TRUE-PEAK LIMITER",24,56,getWidth()-48,37,juce::Justification::left);
  // A red warning light is driven strictly by a measured sample clipping condition.
  // Inter-sample conformity remains separately audited, not inferred here.
  const bool sampleClip=reading_.outputSamplePeak>1.f;
  theme_.paintStatusLed(g,juce::jmax(24,getWidth()-195),56,25,sampleClip);
  g.setColour(juce::Colour(sampleClip?0xffff5969:0xff63e28d));g.setFont(12.f);
  g.drawText(sampleClip?"SAMPLE CLIP":"OUTPUT OK",juce::jmax(24,getWidth()-166),56,130,25,
             juce::Justification::left);
  // Live, mechanically styled gain reduction meter; sourced from DSP
  // telemetry rather than a decorative animation.
  if(getWidth()>=1200){
   const float cx=static_cast<float>(getWidth()-172),cy=209.f,radius=92.f;
   const juce::Rectangle<float> bezel(cx-126.f,105.f,252.f,196.f);
   theme_.drawAnalogGrBezel(g,bezel);
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
  const bool compact=getHeight()<440;
  const float scopeY=compact?216.f:juce::jmax(270.f,static_cast<float>(getHeight()-245)),scopeX=32.f,scopeW=static_cast<float>(getWidth()-64-(getWidth()>=1200?270:0)),scopeH=compact?78.f:95.f;
  g.setColour(juce::Colour(0xff101d27));g.fillRoundedRectangle(scopeX,scopeY,scopeW,scopeH,10.f);
  for(int i=1;i<4;++i){g.setColour(juce::Colour(0xff536c79).withAlpha(.23f));g.drawHorizontalLine(static_cast<int>(scopeY+scopeH*i/4),scopeX,scopeX+scopeW);}
  for(int lane=0;lane<3;++lane){
   const auto& wave=lane==0?reading_.inputWave:(lane==1?reading_.outputWave:reading_.gainEnvelope);
   juce::Path line;
   for(size_t i=0;i<wave.size();++i){
    const float x=scopeX+scopeW*i/(wave.size()-1.f);
    const float reduction=-20.f*std::log10(std::max(1.e-9f,wave[i]));
    const float y=lane==2?scopeY+scopeH*(.1f+.8f*std::clamp(reduction/18.f,0.f,1.f)):
       scopeY+scopeH*.5f-std::clamp(wave[i],-1.f,1.f)*scopeH*.4f;
    if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);
   }
   g.setColour(lane==0?juce::Colour(0xff6fcde2):(lane==1?juce::Colour(0xffffb46b):juce::Colour(0xffff6b76)));
   juce::Graphics::ScopedSaveState scopeClip(g);
   g.reduceClipRegion(juce::Rectangle<float>(scopeX+1,scopeY+1,scopeW-2,scopeH-2).toNearestInt());
   g.strokePath(line,juce::PathStrokeType(lane==2?2.f:1.2f));
  }
  g.setColour(juce::Colour(0xffd4dee6));g.setFont(11.f);
  g.drawText("IN cyan / OUT amber +/-1 FS | GR red 0-18 dB | 0-"+juce::String(511000./scopeRate_,2)+" ms",34,static_cast<int>(scopeY)-18,getWidth()-68,18,juce::Justification::left);
  const int y=compact?299:std::max(375,getHeight()-117);
  auto db=[](float x){return juce::String(20.f*std::log10(std::max(1e-9f,x)),2);};
  g.drawText("INPUT TP ESTIMATE  "+db(reading_.detectedTruePeak)+" dBFS"
             "     OUTPUT SAMPLE PEAK  "+db(reading_.outputSamplePeak)+" dBFS"
             "     GAIN REDUCTION  "+juce::String(reading_.gainReductionDb,2)+" dB",
             24,y,getWidth()-48,32,juce::Justification::left);
  if(!compact){g.setColour(juce::Colour(0xff29393a));
  g.fillRoundedRectangle(24.f,static_cast<float>(y+45),static_cast<float>(getWidth()-48),15.f,5.f);
  g.setColour(juce::Colour(0xffb3e2ce));
  g.fillRoundedRectangle(24.f,static_cast<float>(y+45),static_cast<float>(getWidth()-48)*juce::jlimit(0.f,1.f,reading_.gainReductionDb/18.f),15.f,5.f);
  g.setColour(juce::Colour(0xff9dacad));g.setFont(12.f);
  g.drawText("Selectable 1x/2x/4x detector / fixed 128 samples PDC. Independent 8x stress audit active; formal certification remains external.",
             24,y+83,getWidth()-48,30,juce::Justification::left);}
 }
};
