#pragma once
// Unified Analog Exciter controls: four harmonic bands plus Tube/FET/Tape colour.
#include <memory>
#include <vector>
class AnalogizerPanel final : public juce::Component {
 // Uses the actual pre-rendered Concept C metal and LED atlases. The
 // 101 frames map directly to the APVTS-backed analogExcite parameter.
 class ExciteKnob final : public juce::Slider {
  static constexpr int frameSize=160;
  juce::Image metal_{juce::ImageCache::getFromMemory(
      MasterLabGuiAssets::knob_160_atlas_png,MasterLabGuiAssets::knob_160_atlas_pngSize)};
  juce::Image ring_{juce::ImageCache::getFromMemory(
      MasterLabGuiAssets::led_wreath_160_atlas_png,MasterLabGuiAssets::led_wreath_160_atlas_pngSize)};
 public:
  ExciteKnob(){
   setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
   setTextBoxStyle(juce::Slider::TextBoxBelow,false,100,26);
  }
  bool renderedAssetsReady()const noexcept{
   return !metal_.isNull()&&!ring_.isNull()&&
          metal_.getWidth()>=11*frameSize&&metal_.getHeight()>=10*frameSize&&
          ring_.getWidth()>=11*frameSize&&ring_.getHeight()>=10*frameSize;
  }
  void paint(juce::Graphics& g)override{
   const float w=static_cast<float>(getWidth());
   const float diameter=std::max(1.f,std::min(w,static_cast<float>(getHeight())-34.f));
   const float cy=diameter*.5f+3.f,cx=w*.5f;
   const float value=juce::jlimit(0.f,1.f,static_cast<float>(getNormalisableRange().convertTo0to1(getValue())));
   const int frame=juce::jlimit(0,100,juce::roundToInt(value*100.f));
   const int srcX=(frame%11)*frameSize,srcY=(frame/11)*frameSize;
   const int ringFrame=isEnabled()?frame:0;
   const int ringX=(ringFrame%11)*frameSize,ringY=(ringFrame/11)*frameSize;
   const float shown=diameter*.91f;
   const int dx=juce::roundToInt(cx-shown*.5f);
   const int dy=juce::roundToInt(cy-shown*.5f);
   const int size=juce::roundToInt(shown);
   if(renderedAssetsReady()){
    g.setOpacity(isEnabled()?1.f:.42f);
    g.drawImage(metal_,dx,dy,size,size,srcX,srcY,frameSize,frameSize);
    g.drawImage(ring_,dx,dy,size,size,ringX,ringY,frameSize,frameSize);
    g.setOpacity(1.f);
   }else{
    // A missing render is a packaging error, never a substitute fake meter.
    g.setColour(juce::Colours::darkred);
    g.drawEllipse(dx,dy,size,size,2.f);
    g.setColour(juce::Colours::lightgrey);
    g.drawFittedText("MISSING GUI ASSETS",getLocalBounds().reduced(10),juce::Justification::centred,2);
    return;
   }
   // Legible engraving over the metal centre; no independent animation.
   g.setFont(juce::Font(juce::FontOptions(14.f).withStyle("bold")));
   g.setColour(juce::Colour(0xff141d27));
   g.drawText("EXCITE",juce::roundToInt(cx-shown*.27f),juce::roundToInt(cy-12.f),
              juce::roundToInt(shown*.54f),24,juce::Justification::centred);
   if(hasKeyboardFocus(true)){
    g.setColour(juce::Colour(0xff087bff).withAlpha(.85f));
    g.drawEllipse(juce::Rectangle<float>(dx,dy,size,size).expanded(1.f),1.5f);
   }
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
 void refreshStatus(){
  auto reflect=[](juce::ToggleButton& present,juce::ToggleButton& enabled){
   const bool inChain=present.getToggleState();
   enabled.setButtonText(!inChain?"NOT IN CHAIN":
                         (enabled.getToggleState()?"ACTIVE":"OFF"));
   enabled.setEnabled(inChain);
  };
  reflect(present_,enabled_);reflect(harmonicsPresent_,harmonicsEnabled_);
 }
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
  present_.onClick=[this]{refreshStatus();};enabled_.onClick=[this]{refreshStatus();};
  harmonicsPresent_.onClick=[this]{refreshStatus();};harmonicsEnabled_.onClick=[this]{refreshStatus();};
  refreshStatus();
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
 bool has3DExciteAssets()const noexcept{return excite_.renderedAssetsReady();}
 void update(const Processor::ResponseFrame& frame){reading_=frame.analogizer;harmonicReading_=frame.exciter;refreshStatus();repaint();}
 void resized()override{
  const bool compactWidth=getWidth()<850;
  present_.setBounds(24,12,compactWidth?112:130,30);
  enabled_.setBounds(compactWidth?140:162,12,compactWidth?98:110,30);
  harmonicsPresent_.setBounds(compactWidth?244:282,12,compactWidth?119:145,30);
  harmonicsEnabled_.setBounds(compactWidth?367:438,12,compactWidth?110:145,30);
  mode_.setBounds(compactWidth?481:596,12,compactWidth?170:170,30);
  const bool compact=getHeight()<440;
  if(compact){
   // Minimum host viewport: reserve a real 150 px EXCITE instrument and use a
   // three-by-five control grid. The old 39 px sliders left only 15 px for
   // the rotary artwork after the value box, effectively making them vanish.
   excite_.setBounds(18,91,150,158);
   const int gridX=184,gridWidth=getWidth()-gridX-18;
   const int cellWidth=gridWidth/5,rowHeight=(getHeight()-96)/3;
   for(size_t i=0;i<controls_.size();++i){
    const int column=static_cast<int>(i%5),row=static_cast<int>(i/5);
    const int x=gridX+column*cellWidth,y=91+row*rowHeight;
    controls_[i]->label.setBounds(x,y,cellWidth-5,15);
    controls_[i]->slider.setBounds(x,y+15,cellWidth-5,rowHeight-17);
   }
   return;
  }
  const int centre=getWidth()/2;
  const int exciteSize=getHeight()>=680?300:244;
  excite_.setBounds(centre-exciteSize/2,101,exciteSize,exciteSize+9);
  const int sideWidth=std::clamp((getWidth()-exciteSize-110)/2,112,170);
  const int sideRow=getHeight()>=680?105:75;
  for(size_t i=0;i<controls_.size();++i){
   int x=0,y=0,width=sideWidth;
   if(i<4){x=24;y=102+static_cast<int>(i)*sideRow;}
   else if(i<8){x=getWidth()-sideWidth-24;y=102+static_cast<int>(i-4)*sideRow;}
   else {width=std::max(82,(getWidth()-48)/7);x=24+static_cast<int>(i-8)*width;y=getHeight()-(getHeight()>=680?142:105);}
   controls_[i]->label.setBounds(x,y,width-5,18);
   const int sliderHeight=(i<8?sideRow-19:(getHeight()>=680?112:82));
   controls_[i]->slider.setBounds(x,y+18,width-5,sliderHeight);
  }
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff20303e));g.setColour(juce::Colour(0xffe9c190));
  g.setFont(juce::FontOptions(23.f));g.drawText("ANALOG EXCITER  /  Tube - FET - Tape",24,50,getWidth()-48,34,juce::Justification::left);
  g.setColour(juce::Colours::lightgrey);g.setFont(juce::FontOptions(14.f));
  if(getHeight()>=440)g.drawText("Input RMS "+juce::String(reading_.inputRms,4)+"   Output RMS "+juce::String(reading_.outputRms,4)+"   Delta RMS "+juce::String(reading_.deltaRms,4),
      24,81,getWidth()-48,20,juce::Justification::left);
  const int meterY=getHeight()>=680?445:363;
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
      24,82,getWidth()-48,18,juce::Justification::right);
 }
};
