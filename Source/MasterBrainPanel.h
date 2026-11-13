#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MasterBrain.h"
#include "StudioTheme.h"

class MasterBrainPanel final : public juce::Component {
 StudioTheme& theme_;masterlab::BrainSnapshot snapshot_{};double progress_=0;
 juce::Label phase_,targetLufs_,targetPeak_,quality_,chainAnchor_;
 static juce::String phaseText(masterlab::BrainPhase phase){
  switch(phase){case masterlab::BrainPhase::Pending:return "WAITING FOR AUDIO";case masterlab::BrainPhase::Analysing:return "LISTENING";case masterlab::BrainPhase::Optimising:return "BUILDING MASTER";case masterlab::BrainPhase::Ready:return "READY TO AUDITION";case masterlab::BrainPhase::Applied:return "APPLIED";case masterlab::BrainPhase::Failed:return "CHECK INPUT";case masterlab::BrainPhase::Cancelled:return "CANCELLED";default:return "READY";}
 }
 static float proposed(const masterlab::BrainProposal& proposal,const char* id){for(const auto& change:proposal.changes)if(change.first==id)return change.second;return 0.f;}
public:
 explicit MasterBrainPanel(StudioTheme& theme):theme_(theme){setComponentID("master-brain-panel");setInterceptsMouseClicks(false,true);
  for(auto* label:{&phase_,&targetLufs_,&targetPeak_,&quality_,&chainAnchor_}){addAndMakeVisible(label);label->setFont(theme_.font(11));label->setColour(juce::Label::textColourId,juce::Colour(0xffd8e4ed));label->setJustificationType(juce::Justification::centred);}
  phase_.setComponentID("brain-phase-status");targetLufs_.setComponentID("brain-target-lufs");targetPeak_.setComponentID("brain-target-dbtp");quality_.setComponentID("brain-quality-score");chainAnchor_.setComponentID("brain-chain-cards");chainAnchor_.setText("CHAIN",juce::dontSendNotification);
 }
 void update(const masterlab::BrainSnapshot& snapshot,double progress){snapshot_=snapshot;progress_=std::clamp(progress,0.,1.);phase_.setText(phaseText(snapshot.phase),juce::dontSendNotification);
  const auto& proposal=snapshot.proposal;targetLufs_.setText("AUTO LOUDNESS\n"+(proposal.valid?juce::String(proposal.targetLufs,1)+" LUFS":"—"),juce::dontSendNotification);targetPeak_.setText("TRUE-PEAK CEILING\n"+(proposal.valid?juce::String(proposal.targetCeilingDbtp,1)+" dBTP":"—"),juce::dontSendNotification);
  quality_.setText("QUALITY SCORE\n"+(snapshot.score.valid?juce::String(snapshot.score.total,2):juce::String("—")),juce::dontSendNotification);repaint();
 }
 void resized()override{phase_.setBounds(getWidth()-210,18,188,34);auto metrics=juce::Rectangle<int>(22,98,getWidth()-44,58);const int metricWidth=std::max(100,metrics.getWidth()/3);targetLufs_.setBounds(metrics.removeFromLeft(metricWidth).reduced(4));targetPeak_.setBounds(metrics.removeFromLeft(metricWidth).reduced(4));quality_.setBounds(metrics.reduced(4));chainAnchor_.setBounds(22,getHeight()-104,getWidth()-44,58);}
 void paint(juce::Graphics& g)override{
  const auto bounds=getLocalBounds().toFloat();g.setColour(juce::Colour(0xff08111a));g.fillRoundedRectangle(bounds,16);g.setColour(juce::Colour(0xff243846));g.drawRoundedRectangle(bounds.reduced(.5f),16,1);
  g.setColour(juce::Colour(0xffe6f1f6));g.setFont(theme_.font(22));g.drawText("MASTER BRAIN",22,14,getWidth()/2,28,juce::Justification::left);g.setColour(juce::Colour(0xff6d8798));g.setFont(theme_.font(11));g.drawText("ANALYSIS-BY-SYNTHESIS  •  ONE VERIFIED CHAIN",22,40,getWidth()/2,18,juce::Justification::left);
  auto plot=juce::Rectangle<float>(22,164,getWidth()-44.f,std::max(90.f,getHeight()-286.f));g.setColour(juce::Colour(0xff061018));g.fillRoundedRectangle(plot,10);g.setColour(juce::Colour(0xff1c3441));g.drawRoundedRectangle(plot,10,1);
  for(int i=1;i<5;++i){g.setColour(juce::Colour(0xff172a35));g.drawHorizontalLine(static_cast<int>(plot.getY()+plot.getHeight()*i/5),plot.getX()+12,plot.getRight()-12);}
  const auto& p=snapshot_.proposal;const bool valid=p.valid&&p.targetShares[0]>0;juce::Path input,output;
  for(size_t i=0;i<5;++i){const float x=plot.getX()+18+(plot.getWidth()-36)*i/4.f;float inY=plot.getCentreY(),outY=inY;if(valid){const double target=std::max(1.e-20f,p.targetShares[i]);const auto y=[&](float share){return juce::jmap(static_cast<float>(std::clamp(10*std::log10(std::max(1.e-20f,share)/target),-12.,12.)),-12.f,12.f,plot.getBottom()-20,plot.getY()+20);};inY=y(p.shares[i]);outY=y(p.estimatedShares[i]);}if(i==0){input.startNewSubPath(x,inY);output.startNewSubPath(x,outY);}else{input.lineTo(x,inY);output.lineTo(x,outY);}}
  g.setColour(juce::Colour(0xff536b7a));g.strokePath(input,juce::PathStrokeType(1.5f));g.setColour(juce::Colour(0xff29d3d0).withAlpha(.15f));g.strokePath(output,juce::PathStrokeType(8.f));g.setColour(juce::Colour(0xff29d3d0));g.strokePath(output,juce::PathStrokeType(2.f));
  g.setFont(theme_.font(10));g.setColour(juce::Colour(0xff718b9b));g.drawText("INPUT",static_cast<int>(plot.getX()+12),static_cast<int>(plot.getY()+8),60,16,juce::Justification::left);g.setColour(juce::Colour(0xff29d3d0));g.drawText("MASTER",static_cast<int>(plot.getX()+72),static_cast<int>(plot.getY()+8),70,16,juce::Justification::left);
  auto chain=juce::Rectangle<float>(22,getHeight()-104.f,getWidth()-44.f,58.f);constexpr std::array<std::pair<const char*,const char*>,7> modules{{{"stabilizerPresent","STABILIZER"},{"dynamicEqPresent","DYNAMIC EQ"},{"dynamicsPresent","DYNAMICS"},{"stereoPresent","STEREO"},{"eqPresent","SPECTRAL EQ"},{"analogizerPresent","EXCITER"},{"truePeakOn","MAXIMIZER"}}};int active=0;for(auto module:modules)if(proposed(p,module.first)>.5f)++active;active=std::max(1,active);const float gap=7,card=(chain.getWidth()-gap*(active-1))/active;int cardIndex=0,number=1;for(auto module:modules)if(proposed(p,module.first)>.5f){auto box=juce::Rectangle<float>(chain.getX()+cardIndex*(card+gap),chain.getY(),card,chain.getHeight());g.setColour(juce::Colour(0xff10232e));g.fillRoundedRectangle(box,8);g.setColour(juce::Colour(0xff2b5362));g.drawRoundedRectangle(box,8,1);g.setColour(juce::Colour(0xff29d3d0));g.setFont(theme_.font(10));g.drawText(juce::String(number++).paddedLeft('0',2),box.toNearestInt().reduced(9).removeFromTop(16),juce::Justification::left);g.setColour(juce::Colour(0xffdbe8ee));g.drawFittedText(module.second,box.toNearestInt().reduced(9).withTrimmedTop(14),juce::Justification::centredLeft,1);++cardIndex;}
  if(!valid){g.setColour(juce::Colour(0xff6f8796));g.setFont(theme_.font(14));g.drawFittedText("Play a representative loud section. Brain analyses up to 20 seconds, then releases its heavy buffers.",plot.toNearestInt().reduced(40),juce::Justification::centred,2);}
  if(snapshot_.phase==masterlab::BrainPhase::Analysing){g.setColour(juce::Colour(0xff29d3d0).withAlpha(.6f));g.fillRoundedRectangle(22.f,128.f,(getWidth()-44.f)*static_cast<float>(progress_),4.f,2);}
 }
};
