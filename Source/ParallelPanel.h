#pragma once
#include "Processor.h"
#include "StudioTheme.h"

// Hybrid Studio Phase 2A: controls and meters use APVTS plus snapshots copied by
// Processor on the audio thread. Painting never analyses or changes audio.
class ParallelPanel final : public juce::Component {
 using SliderAttachment=juce::AudioProcessorValueTreeState::SliderAttachment;
 Processor& processor;StudioTheme& theme;Processor::ResponseFrame frame;
 std::array<juce::Slider,masterlab::fullParallelBranchCount> trim,mix;
 std::array<juce::Label,masterlab::fullParallelBranchCount> trimLabel,mixLabel;
 std::array<std::unique_ptr<SliderAttachment>,masterlab::fullParallelBranchCount> trimAttachment,mixAttachment;
 juce::ToggleButton compensated{"Normalize branch weights"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> compensatedAttachment;
 const juce::Colour panel{0xff191d25},muted{0xffabb5c3},accent{0xff82dfcd},warning{0xffe7bb6a};
 static constexpr std::array<const char*,masterlab::fullParallelBranchCount> names{"DYNAMIC EQ","DYNAMICS","STEREO","SPECTRAL EQ","STABILIZER","ANALOG EXCITER","BASS ARCHITECT","TRANSIENT","RESTORATION"};
 static constexpr std::array<const char*,masterlab::fullParallelBranchCount> trimIds{"parallelTrimDynamicEq","parallelTrimDynamics","parallelTrimStereo","parallelTrimEq","parallelTrimStabilizer","parallelTrimAnalog","parallelTrimBass","parallelTrimTransient","parallelTrimRestoration"};
 static constexpr std::array<const char*,masterlab::fullParallelBranchCount> mixIds{"parallelMixDynamicEq","parallelMixDynamics","parallelMixStereo","parallelMixEq","parallelMixStabilizer","parallelMixAnalog","parallelMixBass","parallelMixTransient","parallelMixRestoration"};
public:
 ParallelPanel(Processor& p,StudioTheme& t):processor(p),theme(t){
  setComponentID("parallel-panel");addAndMakeVisible(compensated);compensated.setComponentID("parallel-compensated");
  compensated.setTooltip("Off preserves legacy equal 1/N averaging. On divides by the live sum of branch gains so identical branches remain at unity while trims change their relative weight.");
  compensatedAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"parallelCompensated",compensated);
  for(size_t i=0;i<masterlab::fullParallelBranchCount;++i){
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
  compensated.setBounds(getWidth()-235,8,215,26);const int gap=8,columns=3,cardWidth=(getWidth()-40-gap*(columns-1))/columns;const int top=48,rowHeight=std::max(102,(getHeight()-top-18)/3);
  for(int i=0;i<static_cast<int>(masterlab::fullParallelBranchCount);++i){const int column=i%3,row=i/3,x=20+column*(cardWidth+gap),y=top+row*rowHeight;trimLabel[i].setBounds(x+10,y+49,40,18);trim[i].setBounds(x+50,y+47,cardWidth-60,22);mixLabel[i].setBounds(x+10,y+72,40,18);mix[i].setBounds(x+50,y+70,cardWidth-60,22);}
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
  g.drawVerticalLine(juce::jmax(10,getWidth()-11),0.f,static_cast<float>(getHeight()));g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(22));g.drawText("Parallel Branch Mixer",20,5,getWidth()-280,30,juce::Justification::left);
  g.setColour(muted);g.setFont(theme.font(11));g.drawText(frame.parallel.compensated?"NORMALIZED WEIGHTS  /  relative trims":"LEGACY 1/N  /  trims can change total level",20,31,getWidth()-40,18,juce::Justification::left);
  const int gap=8,columns=3,cardWidth=(getWidth()-40-gap*(columns-1))/columns;const int top=48,rowHeight=std::max(102,(getHeight()-top-18)/3);
  for(int i=0;i<static_cast<int>(masterlab::fullParallelBranchCount);++i){const int column=i%3,row=i/3,x=20+column*(cardWidth+gap),y=top+row*rowHeight;const bool present=frame.parallel.present[i],enabled=frame.parallel.enabled[i];g.setColour(panel);g.fillRoundedRectangle(static_cast<float>(x),static_cast<float>(y),static_cast<float>(cardWidth),static_cast<float>(rowHeight-6),10);g.setColour(present&&enabled?accent:muted.withAlpha(.45f));g.fillEllipse(static_cast<float>(x+10),static_cast<float>(y+11),7,7);g.setFont(theme.font(10));g.drawText(names[i],x+23,y+4,cardWidth-32,18,juce::Justification::left);
   const float db=juce::Decibels::gainToDecibels(frame.parallel.rms[i],-60.f),normal=juce::jlimit(0.f,1.f,(db+60)/60);g.setColour(juce::Colour(0xff0a1119));g.fillRoundedRectangle(x+12.f,y+34.f,cardWidth-24.f,7,3);g.setColour(present&&enabled?accent:muted.withAlpha(.25f));g.fillRoundedRectangle(x+12.f,y+34.f,(cardWidth-24)*normal,7,3);g.setColour(muted);g.setFont(theme.font(9));const juce::String status=!present?"REMOVED":(!enabled?"BYPASSED":juce::String(db,1)+" dBFS");g.drawText(status,x+12,y+43,cardWidth-24,14,juce::Justification::right);
  }
  g.setColour(muted);g.setFont(theme.font(10));g.drawText("Nine latency-aligned branches; Final Limiter remains the single fixed output stage",20,getHeight()-22,getWidth()-40,18,juce::Justification::centred);
 }
};
