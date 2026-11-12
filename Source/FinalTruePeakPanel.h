#pragma once
#include <array>
#include <memory>

// Fixed-latency final stage. Controls are host parameters; meters are DSP telemetry.
class FinalTruePeakPanel final : public juce::Component {
 Processor& processor_; StudioTheme& theme_;
 juce::ToggleButton included_{"FINAL SLOT"},enabled_{"ACTIVE"},autoRelease_{"AUTO RELEASE"},delta_{"DELTA"};
 juce::ComboBox character_,oversampling_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment_,autoReleaseAttachment_,deltaAttachment_;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment_,oversamplingAttachment_;
 struct Control { juce::Label name; juce::Slider slider; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment; };
 std::array<std::unique_ptr<Control>,10> controls_;
 masterlab::FinalTruePeakReading reading_{}; double scopeRate_=48000.;
 void addControl(size_t index,const char* caption,const char* parameter,juce::Slider::SliderStyle style){
  auto c=std::make_unique<Control>();c->name.setText(caption,juce::dontSendNotification);c->name.setJustificationType(juce::Justification::centred);c->name.setColour(juce::Label::textColourId,juce::Colour(0xffaebdca));
  c->slider.setComponentID(parameter);c->slider.setSliderStyle(style);c->slider.setTextBoxStyle(style==juce::Slider::LinearVertical?juce::Slider::TextBoxBelow:juce::Slider::TextBoxRight,false,74,22);
  addAndMakeVisible(c->name);addAndMakeVisible(c->slider);c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor_.state,parameter,c->slider);controls_[index]=std::move(c);
 }
 void layoutControl(size_t i,juce::Rectangle<int> bounds){controls_[i]->name.setBounds(bounds.removeFromTop(20));controls_[i]->slider.setBounds(bounds);}
 static juce::String db(float value){return juce::String(20.f*std::log10(std::max(value,1.e-9f)),1);}
public:
 FinalTruePeakPanel(Processor& p,StudioTheme& t):processor_(p),theme_(t){
  setLookAndFeel(&theme_);setComponentID("final-true-peak-panel");included_.setComponentID("true-peak-present");included_.setEnabled(false);included_.setToggleState(true,juce::dontSendNotification);
  enabled_.setComponentID("true-peak-enabled");autoRelease_.setComponentID("true-peak-auto-release");delta_.setComponentID("true-peak-delta");for(auto* button:{&included_,&enabled_,&autoRelease_,&delta_})addAndMakeVisible(*button);
  character_.setComponentID("true-peak-character");for(const char* text:{"CLEAN","PUNCH","DENSE"})character_.addItem(text,character_.getNumItems()+1);
  oversampling_.setComponentID("true-peak-oversampling");for(const char* text:{"1x","2x","4x"})oversampling_.addItem(text,oversampling_.getNumItems()+1);addAndMakeVisible(character_);addAndMakeVisible(oversampling_);
  enabledAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakOn",enabled_);autoReleaseAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakAutoRelease",autoRelease_);deltaAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"truePeakDelta",delta_);
  characterAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"truePeakCharacter",character_);oversamplingAttachment_=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"truePeakOversampling",oversampling_);
  addControl(0,"INPUT GAIN","truePeakDrive",juce::Slider::LinearVertical);addControl(1,"DENSITY","truePeakDensity",juce::Slider::LinearVertical);addControl(2,"CEILING dBTP","truePeakCeiling",juce::Slider::LinearVertical);
  addControl(3,"RELEASE","truePeakRelease",juce::Slider::LinearHorizontal);addControl(4,"COMPRESS","truePeakCompressorMix",juce::Slider::LinearHorizontal);addControl(5,"SOFT CLIP","truePeakSoftClip",juce::Slider::LinearHorizontal);addControl(6,"TRANSIENT","truePeakTransientEmphasis",juce::Slider::LinearHorizontal);addControl(7,"STEREO LINK","truePeakStereoLink",juce::Slider::LinearHorizontal);addControl(8,"ATTACK","truePeakAttack",juce::Slider::LinearHorizontal);addControl(9,"LOOKAHEAD","truePeakLookahead",juce::Slider::LinearHorizontal);
  included_.setTooltip("The final stage occupies the fixed last slot; Active bypasses processing without changing host PDC.");
 }
 ~FinalTruePeakPanel()override{setLookAndFeel(nullptr);}void update(const Processor::ResponseFrame& response){reading_=response.truePeak;scopeRate_=response.rate;repaint();}
 void resized()override{
  auto area=getLocalBounds().reduced(22,12);auto top=area.removeFromTop(36);included_.setBounds(top.removeFromLeft(125));top.removeFromLeft(8);enabled_.setBounds(top.removeFromLeft(105));top.removeFromLeft(15);character_.setBounds(top.removeFromLeft(132));top.removeFromLeft(8);oversampling_.setBounds(top.removeFromLeft(76));top.removeFromLeft(8);autoRelease_.setBounds(top.removeFromLeft(132));top.removeFromLeft(8);delta_.setBounds(top.removeFromLeft(92));
  area.removeFromTop(46);const int bottomH=juce::jlimit(142,190,getHeight()/3);auto bottom=area.removeFromBottom(bottomH);auto left=area.removeFromLeft(190);auto right=area.removeFromRight(220);layoutControl(0,left.removeFromLeft(91).reduced(8,0));layoutControl(1,left.removeFromLeft(91).reduced(8,0));layoutControl(2,right.removeFromRight(92).reduced(8,0));
  const int columns=4,cellW=juce::jmax(120,bottom.getWidth()/columns);for(size_t i=3;i<controls_.size();++i){const int n=(int)i-3,row=n/columns,col=n%columns;layoutControl(i,{bottom.getX()+col*cellW,bottom.getY()+row*(bottomH/2),cellW-8,bottomH/2-5});}
 }
 void paint(juce::Graphics& g)override{
  g.setGradientFill({juce::Colour(0xff142535),0.f,0.f,juce::Colour(0xff08111b),0.f,(float)getHeight(),false});g.fillAll();g.setColour(juce::Colour(0xff51d8ff));g.setFont(theme_.font(19.f));g.drawText("FINAL MAXIMIZER",22,52,260,30,juce::Justification::left);g.setColour(juce::Colour(0xff718597));g.setFont(theme_.font(10.f));g.drawText("MODERN METERING CONSOLE  /  FIXED 128-SAMPLE PDC",286,57,getWidth()-308,24,juce::Justification::left);
  const auto graph=juce::Rectangle<float>(222.f,96.f,juce::jmax(120.f,(float)getWidth()-472.f),juce::jmax(120.f,(float)getHeight()-292.f));g.setColour(juce::Colour(0xff050c13));g.fillRoundedRectangle(graph,8.f);g.setColour(juce::Colour(0xff263b4b));g.drawRoundedRectangle(graph,8.f,1.f);for(int i=1;i<5;++i){g.setColour(juce::Colour(0xff284557).withAlpha(.45f));g.drawHorizontalLine((int)(graph.getY()+graph.getHeight()*i/5),graph.getX(),graph.getRight());}
  auto trace=[&](const auto& wave,juce::Colour colour,bool envelope){juce::Path path;for(size_t i=0;i<wave.size();++i){const float x=graph.getX()+graph.getWidth()*i/(wave.size()-1.f),reduction=-20.f*std::log10(std::max(wave[i],1.e-9f));const float y=envelope?graph.getY()+graph.getHeight()*.08f+graph.getHeight()*.84f*std::clamp(reduction/18.f,0.f,1.f):graph.getCentreY()-graph.getHeight()*.42f*std::clamp(wave[i],-1.f,1.f);if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}g.setColour(colour);g.strokePath(path,juce::PathStrokeType(envelope?2.f:1.2f));};
  {juce::Graphics::ScopedSaveState clip(g);g.reduceClipRegion(graph.toNearestInt());trace(reading_.inputWave,juce::Colour(0xff24586d),false);trace(reading_.outputWave,juce::Colour(0xff26c8f2),false);trace(reading_.gainEnvelope,juce::Colour(0xff6de9ff),true);}
  auto meter=[&](float x,const char* label,float value,juce::Colour colour){const auto rail=juce::Rectangle<float>(x,116.f,18.f,graph.getHeight()-42.f);g.setColour(juce::Colour(0xff03080d));g.fillRoundedRectangle(rail,3.f);const float level=juce::jlimit(0.f,1.f,(db(value).getFloatValue()+60.f)/60.f);g.setColour(colour);g.fillRoundedRectangle(rail.withTop(rail.getBottom()-rail.getHeight()*level),3.f);g.setFont(theme_.font(9.f));g.drawText(label,(int)x-8,96,34,18,juce::Justification::centred);g.drawText(db(value),(int)x-13,(int)rail.getBottom()+5,44,18,juce::Justification::centred);};
  g.setColour(juce::Colour(0xff668195));g.setFont(theme_.font(8.f));g.drawText("IN",getWidth()-228,81,58,14,juce::Justification::centred);g.setColour(juce::Colour(0xff21c5eb));g.drawText("OUT",getWidth()-174,81,58,14,juce::Justification::centred);
  meter((float)getWidth()-220.f,"L",reading_.inputPeak[0],juce::Colour(0xff3d7185));meter((float)getWidth()-197.f,"R",reading_.inputPeak[1],juce::Colour(0xff3d7185));meter((float)getWidth()-166.f,"L",reading_.outputPeak[0],juce::Colour(0xff20c8ed));meter((float)getWidth()-143.f,"R",reading_.outputPeak[1],juce::Colour(0xff20c8ed));
  const float gr=juce::jlimit(0.f,18.f,reading_.gainReductionDb);g.setColour(juce::Colour(0xfff36e73));g.fillRect(graph.getX(),graph.getY(),graph.getWidth()*gr/18.f,3.f);g.setColour(juce::Colour(0xffa8b9c6));g.setFont(theme_.font(10.f));g.drawText("GR  "+juce::String(reading_.gainReductionDb,1)+" dB     DENSITY  "+juce::String(reading_.densityGainDb,1)+" dB     TRANSIENT  "+juce::String(reading_.transientActivity,2),graph.toNearestInt().removeFromBottom(25),juce::Justification::centredLeft);
  const bool clip=reading_.outputSamplePeak>1.f;theme_.paintStatusLed(g,getWidth()-38,57,18,clip);g.setColour(clip?juce::Colour(0xffff6670):juce::Colour(0xff62e59a));g.drawText(clip?"CLIP":"TP SAFE",getWidth()-105,56,62,22,juce::Justification::right);g.setColour(juce::Colour(0xff617687));g.setFont(theme_.font(9.f));g.drawText("1x / 2x / 4x TRUE-PEAK DETECTOR  |  LIVE DSP TELEMETRY  |  "+juce::String(511000./scopeRate_,1)+" ms HISTORY",22,getHeight()-18,getWidth()-44,15,juce::Justification::left);
 }
};
