#pragma once
#include "Processor.h"
#include "StudioTheme.h"

// Hybrid Studio Phase 2A: controls and meters use APVTS plus snapshots copied by
// Processor on the audio thread. Painting never analyses or changes audio.
class ParallelPanel final : public juce::Component {
 using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
 Processor& processor;StudioTheme& theme;Processor::ResponseFrame frame;
 std::array<juce::Slider,6> trim,mix;
 std::array<juce::Label,6> trimLabel,mixLabel;
 std::array<std::unique_ptr<SliderAttachment>,6> trimAttachment,mixAttachment;
 juce::ToggleButton compensated{"Normalize branch weights"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compensatedAttachment;
 const juce::Colour panel{0xff191d25},muted{0xffabb5c3},accent{0xff82dfcd},warning{0xffe7bb6a};
 static constexpr std::array<const char*,6> names{"DYNAMIC EQ","DYNAMICS","STEREO","SPECTRAL EQ","LIMITER","STABILIZER"};
 static constexpr std::array<const char*,6> trimIds{"parallelTrimDynamicEq","parallelTrimDynamics","parallelTrimStereo","parallelTrimEq","parallelTrimLimiter","parallelTrimStabilizer"};
 static constexpr std::array<const char*,6> mixIds{"parallelMixDynamicEq","parallelMixDynamics","parallelMixStereo","parallelMixEq","parallelMixLimiter","parallelMixStabilizer"};
public:
 ParallelPanel(Processor& p,StudioTheme& t):processor(p),theme(t){
  setComponentID("parallel-panel");addAndMakeVisible(compensated);compensated.setComponentID("parallel-compensated");
  compensated.setTooltip("Off preserves legacy equal 1/N averaging. On divides by the live sum of branch gains so identical branches remain at unity while trims change their relative weight.");
  compensatedAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"parallelCompensated",compensated);
  for(size_t i=0;i<6;++i){
   for(auto* slider:{&trim[i],&mix[i]}){addAndMakeVisible(slider);slider->setSliderStyle(juce::Slider::LinearHorizontal);slider->setTextBoxStyle(juce::Slider::TextBoxRight,false,58,22);}
   addAndMakeVisible(trimLabel[i]);addAndMakeVisible(mixLabel[i]);trimLabel[i].setText("TRIM",juce::dontSendNotification);mixLabel[i].setText("WET",juce::dontSendNotification);
   for(auto* label:{&trimLabel[i],&mixLabel[i]}){label->setColour(juce::Label::textColourId,muted);label->setFont(theme.font(10));}
   trim[i].setRange(-24,12,.1);trim[i].setTextValueSuffix(" dB");trim[i].setComponentID(trimIds[i]);trim[i].setTooltip("Gain of this aligned branch before summing. Legacy mode keeps the denominator at the number of present branches.");
   mix[i].setRange(0,1,.01);mix[i].setComponentID(mixIds[i]);mix[i].setTooltip("Aligned dry/wet inside this branch. 0 is delayed dry; 1 is the module output.");
   trimAttachment[i]=std::make_unique<SliderAttachment>(processor.state,trimIds[i],trim[i]);mixAttachment[i]=std::make_unique<SliderAttachment>(processor.state,mixIds[i],mix[i]);
  }
 }
 void update(const Processor::ResponseFrame& next){frame=next;repaint();}
 void resized()override{
  compensated.setBounds(getWidth()-235,8,215,26);const int gap=10,columns=3,cardWidth=(getWidth()-40-gap*(columns-1))/columns;const int top=48,rowHeight=std::max(135,(getHeight()-top-18)/2);
  for(int i=0;i<6;++i){const int column=i%3,row=i/3,x=20+column*(cardWidth+gap),y=top+row*rowHeight;trimLabel[i].setBounds(x+12,y+60,42,20);trim[i].setBounds(x+54,y+59,cardWidth-66,23);mixLabel[i].setBounds(x+12,y+88,42,20);mix[i].setBounds(x+54,y+87,cardWidth-66,23);}
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff101318));g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(22));g.drawText("Parallel Branch Mixer",20,5,getWidth()-280,30,juce::Justification::left);
  g.setColour(muted);g.setFont(theme.font(11));g.drawText(frame.parallel.compensated?"NORMALIZED WEIGHTS  /  relative trims":"LEGACY 1/N  /  trims can change total level",20,31,getWidth()-40,18,juce::Justification::left);
  const int gap=10,columns=3,cardWidth=(getWidth()-40-gap*(columns-1))/columns;const int top=48,rowHeight=std::max(135,(getHeight()-top-18)/2);
  for(int i=0;i<6;++i){const int column=i%3,row=i/3,x=20+column*(cardWidth+gap),y=top+row*rowHeight;const bool present=frame.parallel.present[i],enabled=frame.parallel.enabled[i];g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y),static_cast<float>(cardWidth),static_cast<float>(rowHeight-10),12);g.setColour(present&&enabled?accent:muted.withAlpha(.45f));g.fillEllipse(static_cast<float>(x+12),static_cast<float>(y+14),7,7);g.setFont(theme.font(11));g.drawText(names[i],x+26,y+7,cardWidth-38,20,juce::Justification::left);
   const float db=juce::Decibels::gainToDecibels(frame.parallel.rms[i],-60.f),normal=juce::jlimit(0.f,1.f,(db+60)/60);g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(x+12.f,y+34.f,cardWidth-24.f,7,3);g.setColour(present&&enabled?accent:muted.withAlpha(.25f));g.fillRoundedRectangle(x+12.f,y+34.f,(cardWidth-24)*normal,7,3);g.setColour(muted);g.setFont(theme.font(9));const juce::String status=!present?"REMOVED":(!enabled?"BYPASSED":juce::String(db,1)+" dBFS");g.drawText(status,x+12,y+43,cardWidth-24,14,juce::Justification::right);
  }
  if(frame.parallel.limiterBypassed){g.setColour(warning);g.setFont(theme.font(11));g.drawText("LIMITER IS ONE BRANCH: other branches bypass its ceiling",20,getHeight()-22,getWidth()-40,18,juce::Justification::centred);}
  else{g.setColour(muted);g.setFont(theme.font(10));g.drawText("Meters show actual post trim branch RMS from bounded DSP snapshots",20,getHeight()-22,getWidth()-40,18,juce::Justification::centred);}
 }
};
