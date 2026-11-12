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
 juce::ToggleButton delta_{"DELTA"};juce::TextButton details_{"Details"};bool showingDetails_=false;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaAttachment_;
 double scopeRate_=48000.;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> presentAttachment_,enabledAttachment_,harmonicsPresentAttachment_,harmonicsEnabledAttachment_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment_;
 struct Control {
  juce::Label label;juce::Slider slider;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 std::vector<std::unique_ptr<Control>> controls_;
 masterlab::AnalogExciterReading reading_{};
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
  addAndMakeVisible(delta_);addAndMakeVisible(details_);
  delta_.setComponentID("analog-delta");details_.setComponentID("analog-details");
  deltaAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor_.state,"analogDelta",delta_);
  delta_.setTooltip("Monitor input minus the complete processed Analog Exciter, after combined Mix.");
  details_.onClick=[this]{showingDetails_=!showingDetails_;details_.setButtonText(showingDetails_?"Main":"Details");resized();repaint();};
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
  add("Dynamic de-fizz","analogizerDeFizz");add("Colour mix","analogizerMix");
  add("Colour output dB","analogizerOutput");
  add("Low harmonics","exciterAmountLow");add("Mid harmonics","exciterAmountMid");
  add("High harmonics","exciterAmountHigh");add("Air harmonics","exciterAmountAir");
  add("Even H2","exciterEven");add("Odd H3","exciterOdd");
  add("Bias","analogBias");add("Tone","analogTone");
  add("Dynamics","analogDynamics");add("Impact","analogImpact");
  add("Combined mix","analogGlobalMix");
 }
 ~AnalogizerPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& frame){reading_=frame.analogExciter;harmonicReading_=frame.exciter;scopeRate_=frame.rate;repaint();}
 void resized()override{
  const int rowWidth=(getWidth()-48)/6;
  present_.setBounds(24,35,rowWidth-5,28);enabled_.setBounds(24+rowWidth,35,rowWidth-5,28);
  harmonicsPresent_.setBounds(24+2*rowWidth,35,rowWidth-5,28);harmonicsEnabled_.setBounds(24+3*rowWidth,35,rowWidth-5,28);
  mode_.setBounds(24+4*rowWidth,35,rowWidth-5,28);delta_.setBounds(24+5*rowWidth,35,rowWidth-5,28);
  details_.setBounds(getWidth()-126,4,102,26);
  const bool compact=getHeight()<440;
  const int body=getHeight()-88;
  const int diameter=std::min(static_cast<int>(getWidth()*.29f),static_cast<int>(body*.64f));
  const int outputHeight=compact?78:110;
  const int knobTop=76+std::max(0,(body-diameter-outputHeight-14)/2);
  excite_.setVisible(!showingDetails_);excite_.setBounds((getWidth()-diameter)/2,knobTop,diameter,diameter);
  const int sideWidth=(getWidth()-diameter-88)/2;
  const int scopeHeight=juce::jlimit(62,150,getHeight()/5);
  int detailIndex=0;
  for(size_t i=0;i<controls_.size();++i){
   const bool primary=i==0||i==1||i==4||i==12||i==15;
   auto& c=*controls_[i];const bool visible=showingDetails_?!primary:primary;
   c.label.setVisible(visible);c.slider.setVisible(visible);
   int x=0,y=0,width=0,height=compact?60:92;
   if(primary){
    if(i==4){width=std::min(130,diameter);x=(getWidth()-width)/2;y=knobTop+diameter+8;}
    else {width=sideWidth/2;x=(i==0||i==1?24:getWidth()-24-sideWidth)+(i==1||i==15?width:0);y=76+scopeHeight+12;}
   }else{
    width=(getWidth()-48)/6;x=24+(detailIndex%6)*width;y=82+(detailIndex/6)*(compact?116:154);++detailIndex;
   }
   c.label.setBounds(x,y,width-5,18);c.slider.setBounds(x,y+18,width-5,height);
  }
 }
 void drawScope(juce::Graphics& g,juce::Rectangle<float> box,const masterlab::ScopeCapture::Wave& wave,const juce::String& title,juce::Colour colour){
  g.setColour(juce::Colour(0xff080f13));g.fillRoundedRectangle(box,8.f);
  g.setColour(juce::Colour(0xff5e665d));g.drawRoundedRectangle(box,8.f,1.f);
  g.setFont(11.f);g.setColour(juce::Colour(0xffd6dcca));g.drawText(title,box.removeFromTop(18),juce::Justification::centred);
  auto plot=box.reduced(7,4);juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(plot.toNearestInt());
  g.setColour(juce::Colour(0xff24413a));g.drawHorizontalLine(static_cast<int>(plot.getCentreY()),plot.getX(),plot.getRight());
  juce::Path path;for(size_t i=0;i<wave.size();++i){const float x=plot.getX()+plot.getWidth()*i/(wave.size()-1);
   const float y=plot.getCentreY()-std::clamp(wave[i],-1.f,1.f)*plot.getHeight()*.45f;
   if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);
  }
  g.setColour(colour);g.strokePath(path,juce::PathStrokeType(1.3f));
 }
 void paint(juce::Graphics& g)override{
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff323a3c),0,0,juce::Colour(0xff0b1115),0,static_cast<float>(getHeight()),false));g.fillAll();
  g.setColour(juce::Colour(0xffa28b63));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(6),12.f,1.f);
  g.setColour(juce::Colour(0xffe9c190));g.setFont(juce::FontOptions(21.f));
  g.drawText(showingDetails_?"ANALOG EXCITER / DETAILS":"ANALOG EXCITER",24,2,getWidth()-164,30,juce::Justification::left);
  if(showingDetails_){
   g.setColour(juce::Colour(0xffccd3ce));g.setFont(12.f);
   g.drawText("Colour mix controls saturation only. Combined mix and DELTA cover the entire module.",24,getHeight()-35,getWidth()-48,24,juce::Justification::left);return;
  }
  const int body=getHeight()-88;
  const int diameter=std::min(static_cast<int>(getWidth()*.29f),static_cast<int>(body*.64f));
  const float sideWidth=static_cast<float>((getWidth()-diameter-88)/2);
  const float scopeHeight=static_cast<float>(juce::jlimit(62,150,getHeight()/5));
  drawScope(g,{24.f,76.f,sideWidth,scopeHeight},reading_.inputWave,"INPUT / MID WAVE",juce::Colour(0xff64dfca));
  drawScope(g,{getWidth()-24.f-sideWidth,76.f,sideWidth,scopeHeight},reading_.outputWave,"OUTPUT / MID WAVE",juce::Colour(0xffffc878));
  const float y=76.f+scopeHeight+12.f+18.f+(getHeight()<440?60.f:92.f)+10.f;
  g.setFont(11.f);g.setColour(juce::Colour(0xffcbd6ce));
  g.drawText("+/-1 FS | "+juce::String(511000./scopeRate_,2)+" ms | rising/auto",24,static_cast<int>(y),static_cast<int>(sideWidth),18,juce::Justification::left);
  g.drawText("INPUT "+juce::String(juce::Decibels::gainToDecibels(reading_.inputRms,-100.f),1)+" dBFS RMS",24,static_cast<int>(y)+22,static_cast<int>(sideWidth),18,juce::Justification::left);
  const int right=static_cast<int>(getWidth()-24.f-sideWidth);
  g.drawText("OUT "+juce::String(juce::Decibels::gainToDecibels(reading_.outputRms,-100.f),1)+" dBFS RMS",right,static_cast<int>(y),static_cast<int>(sideWidth),18,juce::Justification::left);
  g.drawText("DELTA "+juce::String(juce::Decibels::gainToDecibels(reading_.deltaRms,-100.f),1)+" dBFS RMS",right,static_cast<int>(y)+22,static_cast<int>(sideWidth),18,juce::Justification::left);
  for(size_t band=0;band<4;++band){
   const float level=juce::jlimit(0.f,1.f,(juce::Decibels::gainToDecibels(harmonicReading_.generatedRms[band],-72.f)+72.f)/72.f);
   const float width=sideWidth/4.f,x=24.f+band*width,barY=y+48.f;
   g.setColour(juce::Colour(0xff142724));g.fillRect(x,barY,width-5,6.f);
   g.setColour(juce::Colour(0xff7edcc4));g.fillRect(x,barY,(width-5)*level,6.f);
   g.setColour(juce::Colour(0xffbfcac4));g.drawText(std::array<const char*,4>{"Low H","Mid H","High H","Air H"}[band],static_cast<int>(x),static_cast<int>(barY)+8,static_cast<int>(width),16,juce::Justification::left);
  }
 }
};
