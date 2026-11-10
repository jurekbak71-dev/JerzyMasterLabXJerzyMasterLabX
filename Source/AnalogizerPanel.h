#pragma once
// Unified Analog Exciter controls: four harmonic bands plus Tube/FET/Tape colour.
#include <memory>
#include <vector>
class AnalogizerPanel final : public juce::Component {
 class ExciteKnob final : public juce::Slider {
  public:
   ExciteKnob(){setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);setTextBoxStyle(juce::Slider::TextBoxBelow,false,100,26);}
   void paint(juce::Graphics& g)override {
    const float w=static_cast<float>(getWidth()),diameter=std::min(w,static_cast<float>(getHeight())-34.f);
    const float cx=w*.5f,cy=diameter*.5f+3.f,r=std::max(12.f,diameter*.46f);
    g.setColour(juce::Colour(0xff060c11));g.fillEllipse(cx-r-9,cy-r-9,2*r+18,2*r+18);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff697780),cx-r,cy-r,juce::Colour(0xff080d12),cx+r,cy+r,false));
    g.fillEllipse(cx-r,cy-r,2*r,2*r);
    const float value=juce::jlimit(0.f,1.f,static_cast<float>(getValue()));
    for(int i=0;i<61;++i){
     const float angle=(-2.35f+4.70f*i/60.f);
     const float inner=r*.81f,outer=r*.93f;
     const auto colour=i<=static_cast<int>(value*60.f)?
       juce::Colour(0xffffb969):juce::Colour(0xff495d67);
     g.setColour(colour);
     g.drawLine(cx+std::sin(angle)*inner,cy-std::cos(angle)*inner,
                cx+std::sin(angle)*outer,cy-std::cos(angle)*outer,1.6f);
    }
    const float disc=r*.73f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xffc7cbd0),cx-disc,cy-disc,
                                          juce::Colour(0xff222c34),cx+disc,cy+disc,false));
    g.fillEllipse(cx-disc,cy-disc,disc*2,disc*2);
    g.setColour(juce::Colour(0xff10171e).withAlpha(.4f));g.drawEllipse(cx-disc,cy-disc,disc*2,disc*2,3.f);
    const float angle=-2.35f+4.70f*value;
    g.setColour(juce::Colour(0xffffbc70));g.drawLine(cx+std::sin(angle)*disc*.54f,cy-std::cos(angle)*disc*.54f,cx+std::sin(angle)*disc*.86f,cy-std::cos(angle)*disc*.86f,4.f);
    g.setFont(juce::Font(juce::FontOptions(15.f).withStyle("bold")));
    g.setColour(juce::Colour(0xff151e26));g.drawText("EXCITE",static_cast<int>(cx-disc),static_cast<int>(cy-12),static_cast<int>(disc*2),24,juce::Justification::centred);
    g.setColour(juce::Colour(0xffffd59e));g.setFont(15.f);
    g.drawText(juce::String(static_cast<int>(value*100.f))+"%",0,static_cast<int>(diameter)+5,getWidth(),24,juce::Justification::centred);
   }
 };
 Processor& processor_;StudioTheme& theme_;
 ExciteKnob excite_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> exciteAttachment_;
 masterlab::HarmonicExciterReading harmonicReading_{};
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
  excite_.setComponentID("analog-excite");
  excite_.setTooltip("EXCITE is a real harmonic-intensity macro: drives four-band harmonics and Tube/FET/Tape saturation; 0 preserves old session settings.");
  addAndMakeVisible(excite_);
  exciteAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,"analogExcite",excite_);
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
 void update(const Processor::ResponseFrame& frame){reading_=frame.analogizer;harmonicReading_=frame.exciter;repaint();}
 void resized()override{
  present_.setBounds(24,12,130,30);enabled_.setBounds(162,12,110,30);
  harmonicsPresent_.setBounds(282,12,145,30);harmonicsEnabled_.setBounds(438,12,145,30);
  mode_.setBounds(596,12,170,30);
  const bool compact=getHeight()<440;
  const int centre=getWidth()/2;
  excite_.setBounds(centre-(compact?100:122),compact?88:101,compact?200:244,compact?205:253);
  const int sideWidth=std::max(76,std::min(132,(getWidth()-294)/3));
  for(size_t i=0;i<controls_.size();++i){
   int x=0,y=0,width=sideWidth;
   if(i<4){x=22;y=(compact?90:102)+static_cast<int>(i)*(compact?50:75);}
   else if(i<8){x=getWidth()-sideWidth-22;y=(compact?90:102)+static_cast<int>(i-4)*(compact?50:75);}
   else {width=std::max(70,(getWidth()-44)/7);x=22+static_cast<int>(i-8)*width;y=compact?getHeight()-64:getHeight()-105;}
   controls_[i]->label.setBounds(x,y,width-4,compact?15:18);
   controls_[i]->slider.setBounds(x,y+(compact?15:18),width-4,compact?39:54);
  }
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff20303e));g.setColour(juce::Colour(0xffe9c190));
  g.setFont(juce::FontOptions(23.f));g.drawText("ANALOG EXCITER  /  Tube - FET - Tape",24,50,getWidth()-48,34,juce::Justification::left);
  g.setColour(juce::Colours::lightgrey);g.setFont(juce::FontOptions(14.f));
  if(getHeight()>=440)g.drawText("Input RMS "+juce::String(reading_.inputRms,4)+"   Output RMS "+juce::String(reading_.outputRms,4)+"   Delta RMS "+juce::String(reading_.deltaRms,4),
      24,getHeight()-108,getWidth()-48,24,juce::Justification::left);
  const int meterY=std::min(getHeight()-61,363);
  g.setFont(11.f);
  if(getHeight()>=440)for(size_t band=0;band<4;++band){
   const float energy=harmonicReading_.generatedRms[band];
   const float normalized=juce::jlimit(0.f,1.f,(20.f*std::log10(std::max(1.e-8f,energy))+72.f)/48.f);
   const float width=std::max(32.f,static_cast<float>(getWidth()-328)/4.f);
   const float x=164.f+static_cast<float>(band)*width;
   g.setColour(juce::Colour(0xff1b252e));g.fillRoundedRectangle(x,meterY,width-9.f,9.f,4.f);
   g.setColour(juce::Colour(0xffffb969));g.fillRoundedRectangle(x,meterY,std::max(0.1f,(width-9.f)*normalized),9.f,4.f);
   g.setColour(juce::Colour(0xffcbd6e0));g.drawText(juce::String(std::array<const char*,4>{"LOW H","MID H","HIGH H","AIR H"}[band]),static_cast<int>(x),meterY+10,static_cast<int>(width-9),18,juce::Justification::left);
  }
  if(getHeight()>=440)g.drawText("Actual harmonic DSP RMS / Tube - FET - Tape / EXCITE macro",
      24,getHeight()-53,getWidth()-48,24,juce::Justification::left);
 }
};
