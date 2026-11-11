#pragma once
#include <memory>
#include <vector>
class TransientSculptorPanel final : public juce::Component {
 Processor& processor_;
 StudioTheme& theme_;
 juce::ToggleButton present_{"In chain"},enabled_{"Active"},delta_{"DELTA"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> presentAttachment_,enabledAttachment_,deltaAttachment_;
 struct Knob {juce::Label label;juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
 std::vector<std::unique_ptr<Knob>> knobs_;
 double scopeRate_=48000.;
 masterlab::TransientSculptorReading reading_{};
 void addKnob(const char* label,const char* parameter){
  auto k=std::make_unique<Knob>();
  k->label.setText(label,juce::dontSendNotification);
  k->label.setColour(juce::Label::textColourId,juce::Colour(0xffe5c7ac));
  k->slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  k->slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,88,22);
  k->slider.setComponentID(parameter);
  k->slider.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffffa15a));
  addAndMakeVisible(k->label);addAndMakeVisible(k->slider);
  k->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,parameter,k->slider);
  knobs_.push_back(std::move(k));
 }
public:
 TransientSculptorPanel(Processor& p,StudioTheme& t):processor_(p),theme_(t){
  setComponentID("transient-sculptor-panel");setLookAndFeel(&theme_);
  present_.setComponentID("transient-present");enabled_.setComponentID("transient-enabled");
  addAndMakeVisible(present_);addAndMakeVisible(enabled_);addAndMakeVisible(delta_);delta_.setComponentID("transient-delta");
  deltaAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"transientDelta",delta_);
  presentAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"transientPresent",present_);
  enabledAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"transientOn",enabled_);
  for(const auto& item:std::array<std::pair<const char*,const char*>,8>{{
   {"Low attack","transientAttackLow"},{"Mid attack","transientAttackMid"},
   {"High attack","transientAttackHigh"},{"Low sustain","transientSustainLow"},
   {"Mid sustain","transientSustainMid"},{"High sustain","transientSustainHigh"},
   {"Dry / Wet","transientMix"},{"Output dB","transientOutput"}
  }})addKnob(item.first,item.second);
 }
 ~TransientSculptorPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& f){reading_=f.transient;scopeRate_=f.rate;repaint();}
 void resized()override{
  present_.setBounds(24,12,130,34);enabled_.setBounds(162,12,116,34);delta_.setBounds(290,12,116,34);
  const bool compact=getHeight()<440;
  const int columns=compact?8:4;
  const int width=(getWidth()-48)/columns;
  for(size_t i=0;i<knobs_.size();++i){
   const int x=24+static_cast<int>(i%columns)*width,y=252+static_cast<int>(i/columns)*85;
   knobs_[i]->label.setBounds(x,y,width-8,24);
   knobs_[i]->slider.setBounds(x,y+23,width-8,compact?53:61);
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
  g.setColour(juce::Colour(0xffffae6e));g.setFont(juce::Font(juce::FontOptions(23.f)));
  g.drawText("TRANSIENT SCULPTOR",24,55,getWidth()-48,37,juce::Justification::left);
  g.setColour(juce::Colour(0xffe5d1c0));g.setFont(13.f);
  const float scopeX=36.f,scopeY=112.f,scopeW=std::max(180.f,static_cast<float>(getWidth()-72)),scopeH=119.f;
  g.setColour(juce::Colour(0xff131e27));g.fillRoundedRectangle(scopeX,scopeY,scopeW,scopeH,10.f);
  for(int grid=1;grid<4;++grid){g.setColour(juce::Colour(0xff6d808e).withAlpha(.2f));g.drawHorizontalLine(static_cast<int>(scopeY+grid*scopeH/4.f),scopeX,scopeX+scopeW);}
  for(int channel=0;channel<2;++channel){
   const auto& samples=channel==0?reading_.inputWave:reading_.outputWave;
   juce::Path waveform;
   for(size_t i=0;i<samples.size();++i){
    const float x=scopeX+scopeW*static_cast<float>(i)/(samples.size()-1);
    const float y=scopeY+scopeH*.5f-juce::jlimit(-1.f,1.f,samples[i])*scopeH*.43f;
    if(i==0)waveform.startNewSubPath(x,y);else waveform.lineTo(x,y);
   }
   g.setColour(channel==0?juce::Colour(0xff53cedb):juce::Colour(0xffffb16f));
   juce::Graphics::ScopedSaveState scopeClip(g);
   g.reduceClipRegion(juce::Rectangle<float>(scopeX+1,scopeY+1,scopeW-2,scopeH-2).toNearestInt());
   g.strokePath(waveform,juce::PathStrokeType(1.5f));
  }
  g.setFont(11.f);g.setColour(juce::Colour(0xffe5d1c0));
  g.drawText("IN cyan / OUT amber | +/-1 FS | 0-"+juce::String(511000./scopeRate_,2)+" ms | rising trigger",36,92,getWidth()-72,19,juce::Justification::left);
  const int y=getHeight()<440?330:std::max(412,getHeight()-91);
  g.drawText("ACTUAL DSP  /  Input "+juce::String(reading_.inputRms,4)+
    "    Output "+juce::String(reading_.outputRms,4)+"    Delta "+juce::String(reading_.deltaRms,4),
    24,y,getWidth()-48,28,juce::Justification::left);
  if(getHeight()>=440)for(int k=0;k<3;++k){
   const int colWidth=(getWidth()-60)/3,x=24+k*colWidth;
   const float strength=juce::jlimit(0.f,1.f,reading_.modulationRms[k]*6.f);
   g.setColour(juce::Colour(0xff4c342d));
   g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y+37),static_cast<float>(colWidth-12),10.f,4.f);
   g.setColour(juce::Colour(0xffffae6e));
   g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y+37),static_cast<float>(colWidth-12)*strength,10.f,4.f);
   g.drawText(juce::String(std::array<const char*,3>{"LOW","MID","HIGH"}[k]),
       x,y+51,colWidth-12,20,juce::Justification::left);
  }
  g.setColour(juce::Colour(0xffb5a39b));g.setFont(12.f);
  if(getHeight()>=440)g.drawText("Measured transient activity per Low / Mid / High band",
    24,y+77,getWidth()-48,19,juce::Justification::left);
 }
};
