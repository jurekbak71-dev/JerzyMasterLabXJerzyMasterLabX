#pragma once
#include <memory>
#include <vector>
// Fixed-latency post-chain safety stage. All host parameter attachments are real.
class FinalTruePeakPanel final : public juce::Component {
 Processor& processor_;
 StudioTheme& theme_;
 juce::ToggleButton included_{"Fixed slot"},enabled_{"Active"},autoRelease_{"Auto release"};
 juce::ComboBox character_,oversampling_;
 juce::TextButton advancedButton_{"ADVANCED"};
 bool advancedView_=false;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> includedAttachment_,enabledAttachment_,autoReleaseAttachment_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment_,oversamplingAttachment_;
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
  addAndMakeVisible(included_);addAndMakeVisible(enabled_);included_.setEnabled(false);
  addAndMakeVisible(character_);addAndMakeVisible(autoRelease_);
  addAndMakeVisible(advancedButton_);
  advancedButton_.setComponentID("maximizer-advanced");
  advancedButton_.onClick=[this]{advancedView_=!advancedView_;advancedButton_.setButtonText(advancedView_?"CORE":"ADVANCED");resized();repaint();};
  addAndMakeVisible(oversampling_);
  oversampling_.setComponentID("true-peak-oversampling");
  for(const char* title:{"1x (sample)","2x TP","4x TP","8x TP"})oversampling_.addItem(title,oversampling_.getNumItems()+1);
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
  addKnob("Lookahead ms","truePeakLookahead");
  addKnob("Attack ms","truePeakAttack");
  addKnob("Transient Emphasis","truePeakTransient");
  included_.setTooltip("Final limiter occupies the last slot; Active switches it OFF without changing host PDC.");
  oversampling_.setTooltip("True-peak interpolation/detection factor. Not oversampling of the entire soft-clip distortion path.");
  advancedButton_.setTooltip("Switch between core Maximizer controls and advanced lookahead, attack, transient and TP detection settings.");
 }
 ~FinalTruePeakPanel()override{setLookAndFeel(nullptr);}
 void update(const Processor::ResponseFrame& response){reading_=response.truePeak;repaint();}
 void resized()override{
  const int w=getWidth(),h=getHeight();
  const bool compact=h<430;
  const bool showMeter=w>=870&&!compact;
  const int meterReserve=showMeter?270:0;
  // The meter owns a dedicated column: controls can never overlap it.
  included_.setBounds(24,12,124,32);
  enabled_.setBounds(154,12,115,32);
  character_.setBounds(278,12,168,32);
  autoRelease_.setBounds(454,12,130,32);
  advancedButton_.setBounds(w-157,12,125,32);
  const int areaWidth=std::max(200,w-48-meterReserve);
  const int columns=advancedView_?4:5;
  const int gap=12;
  const int knobWidth=std::clamp((areaWidth-gap*(columns-1))/columns,72,157);
  const int rowWidth=columns*knobWidth+(columns-1)*gap;
  const int xStart=24+std::max(0,(areaWidth-rowWidth)/2);
  const int rowTop=compact?109:(h>=680?133:121);
  const int knobTop=rowTop+23;
  const int knobHeight=compact?91:(h>=680?155:129);
  for(size_t i=0;i<knobs_.size();++i){
   const bool visible=advancedView_?i>=5:i<5;
   auto& knob=*knobs_[i];
   knob.name.setVisible(visible);
   knob.slider.setVisible(visible);
   if(!visible)continue;
   const int column=static_cast<int>(advancedView_?i-5:i);
   const int x=xStart+column*(knobWidth+gap);
   knob.name.setBounds(x,rowTop,knobWidth,21);
   knob.slider.setBounds(x,knobTop,knobWidth,knobHeight);
  }
  oversampling_.setVisible(advancedView_);
  if(advancedView_)
   oversampling_.setBounds(xStart+3*(knobWidth+gap),knobTop+std::min(38,knobHeight/3),
                           knobWidth,32);
 }
 void paint(juce::Graphics& g)override{
  const int w=getWidth(),h=getHeight();
  if(w<120||h<150)return;
  const bool compact=h<430;
  const bool showMeter=w>=870&&!compact;
  // Concept C: powder-coated light chassis around a recessed, dark DSP surface.
  g.fillAll(juce::Colour(0xffd6dfe4));
  const auto frame=juce::Rectangle<float>(9.f,0.f,static_cast<float>(w-18),static_cast<float>(h));
  g.setGradientFill(juce::ColourGradient(juce::Colour(0xff324555),12.f,0.f,
                                        juce::Colour(0xff111d28),12.f,static_cast<float>(h),false));
  g.fillRoundedRectangle(frame,11.f);
  g.setColour(juce::Colour(0xff7a8e9e));g.drawRoundedRectangle(frame.reduced(.5f),10.f,1.f);
  g.setColour(juce::Colour(0xff1c9bc8).withAlpha(.35f));
  g.drawHorizontalLine(99,24.f,static_cast<float>(w-24));
  g.setColour(juce::Colour(0xffc3eadf));g.setFont(theme_.font(compact?18.f:20.f));
  g.drawText("FINAL MAXIMIZER",24,53,w-260,32,juce::Justification::centredLeft);
  g.setColour(juce::Colour(0xff8faebf));g.setFont(theme_.font(11.f));
  g.drawText("STEREO LINKED  /  TRUE PEAK",24,80,w-260,18,juce::Justification::left);
  const bool clipping=reading_.active&&
      reading_.outputTruePeak>reading_.ceilingLinear*1.002f;
  const juce::String status=!reading_.active?"WAITING / BYPASSED":
       (clipping?"TRUE PEAK OVER":"TRUE PEAK EST OK");
  theme_.paintStatusLed(g,w-191,63,21,clipping);
  g.setFont(theme_.font(11.f));
  g.setColour(clipping?juce::Colour(0xffff7384):juce::Colour(0xffb3e2ce));
  g.drawText(status,w-163,60,133,26,juce::Justification::centredLeft);

  const int areaWidth=w-48-(showMeter?270:0);
  const int columns=advancedView_?4:5;
  const int gap=12;
  const int knobWidth=std::clamp((std::max(200,areaWidth)-gap*(columns-1))/columns,72,157);
  const int rowWidth=columns*knobWidth+(columns-1)*gap;
  const int xStart=24+std::max(0,(areaWidth-rowWidth)/2);
  const int rowTop=compact?109:(h>=680?133:121);
  const int knobTop=rowTop+23;
  const int knobHeight=compact?91:(h>=680?155:129);

  // Label the fourth advanced control accurately; it is a real true-peak
  // reconstruction setting, not nonlinear oversampling of soft clipping.
  if(advancedView_){
   g.setColour(juce::Colour(0xffc5d6e2));g.setFont(theme_.font(11.f));
   g.drawFittedText("TP DETECTION",xStart+3*(knobWidth+gap),rowTop,
                    knobWidth,20,juce::Justification::centred,1);
  }

  const int rowBottom=knobTop+knobHeight;
  if(showMeter){
   // Physically separate VU window; a knob is never drawn in this column.
   const int mx=w-255,my=109,mw=226,mh=h>=680?211:180;
   const juce::Rectangle<float> meterRect(static_cast<float>(mx),
       static_cast<float>(my),static_cast<float>(mw),static_cast<float>(mh));
   theme_.drawAnalogGrBezel(g,meterRect);
   const float cx=mx+mw*.5f,cy=my+mh*.58f,rad=std::min(82.f,mh*.40f);
   auto polar=[&](float a,float d){
    return juce::Point<float>(cx+std::sin(a)*d,cy-std::cos(a)*d);
   };
   constexpr float first=-1.10f,last=1.10f;
   g.setColour(juce::Colour(0xff4b4036));
   for(int t=0;t<=9;++t){
    const float a=first+(last-first)*(t/9.f);
    g.drawLine({polar(a,rad*.74f),polar(a,rad*.92f)},t%3==0?1.7f:1.f);
    if(t%3==0){
     const auto p=polar(a,rad*1.02f);
     g.setFont(theme_.font(9.f));g.drawText(juce::String(t*2),
       static_cast<int>(p.x)-13,static_cast<int>(p.y)-12,26,15,
       juce::Justification::centred);
    }
   }
   const float gr=juce::jlimit(0.f,18.f,reading_.gainReductionDb);
   const auto needle=polar(first+(last-first)*(gr/18.f),rad*.82f);
   g.setColour(juce::Colour(0xffc83832));
   g.drawLine({juce::Point<float>(cx,cy),needle},2.5f);
   g.setColour(juce::Colour(0xff282c30));g.fillEllipse(cx-5.f,cy-5.f,10.f,10.f);
   g.setColour(juce::Colour(0xff4d423b));g.setFont(theme_.font(10.f));
   g.drawText("GAIN REDUCTION / dB",mx+12,my+mh-32,mw-24,20,
              juce::Justification::centred);
  }

  const int scopeTop=compact?232:std::max(rowBottom+20,
                                             showMeter?(h>=680?344:305):rowBottom+20);
  const int footerHeight=compact?61:(h>=680?103:91);
  const int scopeHeight=std::max(43,h-scopeTop-footerHeight);
  const auto scope=juce::Rectangle<float>(26.f,static_cast<float>(scopeTop),
                                         static_cast<float>(w-52),
                                         static_cast<float>(scopeHeight));
  g.setColour(juce::Colour(0xff091722));g.fillRoundedRectangle(scope,9.f);
  g.setColour(juce::Colour(0xff2c5364));g.drawRoundedRectangle(scope,9.f,1.f);
  g.saveState();g.reduceClipRegion(scope.toNearestInt().reduced(2));
  for(int line=1;line<4;++line){
   const float gy=scope.getY()+scope.getHeight()*line/4.f;
   g.setColour(juce::Colour(0xff61788d).withAlpha(.22f));
   g.drawHorizontalLine(juce::roundToInt(gy),scope.getX()+4.f,scope.getRight()-4.f);
  }
  for(int lane=0;lane<3;++lane){
   const auto& points=lane==0?reading_.inputWave:
       (lane==1?reading_.outputWave:reading_.gainEnvelope);
   juce::Path line;
   for(size_t i=0;i<points.size();++i){
    const float x=scope.getX()+6.f+
      (scope.getWidth()-12.f)*static_cast<float>(i)/
      static_cast<float>(points.size()-1);
    const float norm=lane==2?
        juce::jlimit(0.f,1.f,1.f-points[i]):
        juce::jlimit(-1.f,1.f,points[i]);
    const float y=scope.getCentreY()-norm*scope.getHeight()*(lane==2?.40f:.43f);
    if(i==0)line.startNewSubPath(x,y);else line.lineTo(x,y);
   }
   g.setColour(lane==0?juce::Colour(0xff76d6ef):
      (lane==1?juce::Colour(0xffffb46b):juce::Colour(0xffff6e82)));
   g.strokePath(line,juce::PathStrokeType(lane==2?2.2f:1.6f));
  }
  g.restoreState();
  // Legend and values occupy their own footer, never overlap a slider.
  const int footerTop=scopeTop+scopeHeight+6;
  g.setColour(juce::Colour(0xffb8c9d3));g.setFont(theme_.font(10.5f));
  g.drawFittedText("INPUT / CYAN     OUTPUT / AMBER     GAIN REDUCTION / RED",
                   30,footerTop,w-60,19,juce::Justification::left,1);
  auto db=[](float peak){return juce::String(20.f*std::log10(std::max(1e-9f,peak)),2);};
  g.setFont(theme_.font(compact?10.f:12.f));
  g.setColour(juce::Colour(0xffe5f1f5));
  const juce::String tpName=reading_.oversamplingFactor>1?"OUTPUT TP EST ":"OUTPUT SAMPLE ";
  const juce::String metrics=reading_.active?
       ("INPUT "+db(reading_.detectedTruePeak)+" dBFS     "+
        tpName+db(reading_.outputTruePeak)+" dBFS     GR "+
        juce::String(reading_.gainReductionDb,2)+" dB"):
       "AWAITING AUDIO — METERS SHOW LAST PROCESSED BLOCK";
  g.drawFittedText(metrics,30,footerTop+20,w-60,25,juce::Justification::left,1);
  if(!compact){
   // The UI settings update immediately; DSP telemetry updates only after
   // a real audio block. Do not present stale measurements as new settings.
   const juce::String selected="SELECTED: "+
        juce::String(1<<std::clamp(oversampling_.getSelectedItemIndex(),0,3))+
        "x TP / "+
        juce::String(knobs_[5]->slider.getValue(),2)+" ms LOOKAHEAD";
   const juce::String processed=reading_.active?
      ("    LAST PROCESSED: "+juce::String(reading_.oversamplingFactor)+
       "x / "+juce::String(reading_.lookaheadMs,2)+" ms"):
      "    LAST PROCESSED: NO AUDIO";
   g.setColour(juce::Colour(0xff90bcc5));g.setFont(theme_.font(10.f));
   g.drawFittedText(selected+processed,30,footerTop+50,w-60,22,
                    juce::Justification::left,1);
  }
 }

};
