#pragma once
#include "StudioTheme.h"
// UI-only controls and plots. Receives coherent DSP snapshots from the editor FIFO.
class ModulePanel final : public juce::Component {
 Processor& processor;StudioTheme& theme;int kind;
 struct Control {juce::Label label;juce::Slider slider;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
 juce::Viewport controlsViewport;juce::Component controlsContent;
 std::vector<std::unique_ptr<Control>> controls;
 juce::ToggleButton present{"In chain"},enabled{"Active"},monoBass{"Mono bass"},mono{"Mono preview"},bandOn{"Band active"};
 juce::ComboBox mode,bandChoice;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> presenceAttachment,enabledAttachment,monoBassAttachment,monoAttachment,bandAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
 Processor::ResponseFrame frame;std::array<juce::Point<float>,1024> scope{};
 std::array<std::array<float,2>,240> history{};size_t historyHead=0,historyCount=0;int selectedBand=1;
 const juce::Colour accent{0xff82dfcd},gold{0xffe7bb6a},muted{0xffabb5c3};
 float value(const juce::String& id)const{return processor.state.getRawParameterValue(id)->load();}
 void add(const char* name,const juce::String& id,const char* suffix=""){
  auto control=std::make_unique<Control>();controlsContent.addAndMakeVisible(control->label);controlsContent.addAndMakeVisible(control->slider);control->label.setText(name,juce::dontSendNotification);control->label.setColour(juce::Label::textColourId,muted);
  control->slider.setSliderStyle(juce::Slider::LinearHorizontal);control->slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,110,24);control->slider.setTextValueSuffix(suffix);control->slider.setComponentID(id);
  control->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,control->slider);controls.push_back(std::move(control));
 }
 void bindBand(){
  static const std::array<const char*,7> ids{"deqFreq","deqGain","deqQ","deqThreshold","deqRange","deqAttack","deqRelease"};
  for(size_t i=0;i<controls.size();++i){controls[i]->attachment.reset();controls[i]->slider.setComponentID(juce::String(ids[i])+juce::String(selectedBand));controls[i]->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,juce::String(ids[i])+juce::String(selectedBand),controls[i]->slider);}
  bandAttachment.reset();bandAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"deqBandOn"+juce::String(selectedBand),bandOn);
 }
public:
 ModulePanel(Processor& p,StudioTheme& t,int type):processor(p),theme(t),kind(type){
  setLookAndFeel(&theme);addAndMakeVisible(controlsViewport);controlsViewport.setViewedComponent(&controlsContent,false);controlsViewport.setScrollBarsShown(true,false);const char* presence=kind==0?"dynamicEqPresent":(kind==1?"dynamicsPresent":"stereoPresent");const char* on=kind==0?"dynamicEqOn":(kind==1?"dynamicsOn":"stereoOn");
  addAndMakeVisible(present);addAndMakeVisible(enabled);present.setComponentID(juce::String(presence));enabled.setComponentID(on);
  presenceAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,presence,present);enabledAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,on,enabled);
  present.setTooltip("Add or remove this processor from the signal chain. Settings are retained; changes crossfade.");enabled.setTooltip("Bypass this module independently without removing it. No added latency.");
  if(kind==0){add("Frequency","deqFreq1"," Hz");add("Static gain","deqGain1"," dB");add("Q","deqQ1");add("Threshold","deqThreshold1"," dBFS");add("Dynamic range","deqRange1"," dB");add("Attack","deqAttack1"," ms");add("Release","deqRelease1"," ms");addAndMakeVisible(bandChoice);addAndMakeVisible(bandOn);for(int i=1;i<=3;++i)bandChoice.addItem("Band "+juce::String(i),i);bandChoice.setSelectedId(1);bandChoice.onChange=[this]{selectedBand=bandChoice.getSelectedId();bindBand();};bindBand();}
  if(kind==1){add("Threshold","dynThreshold"," dBFS");add("Ratio","dynRatio",":1");add("Upward maximum","upwardAmount"," dB");add("Upward threshold","upwardThreshold"," dBFS");add("Attack","dynAttack"," ms");add("Release","dynRelease"," ms");add("Knee","dynKnee"," dB");add("Makeup","dynMakeup"," dB");add("Dry / wet","dynMix");addAndMakeVisible(mode);mode.addItem("Digital",1);mode.addItem("Analog",2);modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"dynamicsMode",mode);mode.setTooltip("Digital: clean gain processing. Analog: slower detector, wider knee and mild arctangent colour. Original DSP, no hardware emulation or oversampling.");}
  if(kind==2){add("Low width","stereoLowWidth"," x");add("Mid width","stereoMidWidth"," x");add("High width","stereoHighWidth"," x");add("Air width","stereoAirWidth"," x");add("Low split","stereoLowSplit"," Hz");add("Mid split","stereoHighSplit"," Hz");add("Top split","stereoTopSplit"," Hz");add("Balance","stereoBalance");addAndMakeVisible(monoBass);addAndMakeVisible(mono);monoBassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stereoMonoBass",monoBass);monoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stereoMono",mono);}
 }
 ~ModulePanel()override{controlsViewport.setViewedComponent(nullptr,false);setLookAndFeel(nullptr);}
 void updateScope(const std::array<juce::Point<float>,1024>& points){scope=points;}
 void update(const Processor::ResponseFrame& f){frame=f;history[historyHead]={f.dynamics.reduction,f.dynamics.upward};historyHead=(historyHead+1)%history.size();historyCount=std::min(history.size(),historyCount+1);repaint();}
 void resized()override{
  present.setBounds(getWidth()-232,12,112,28);enabled.setBounds(getWidth()-114,12,104,28);
  const int side=std::min(340,getWidth()/2-24),left=getWidth()-side-24;
  const int row=54;controlsViewport.setBounds(left+12,94,side-24,getHeight()-154);controlsContent.setSize(side-40,static_cast<int>(controls.size())*row);
  for(size_t i=0;i<controls.size();++i){int y=static_cast<int>(i)*row;controls[i]->label.setBounds(0,y,side-40,18);controls[i]->slider.setBounds(0,y+20,side-40,30);}
  mode.setBounds(left+12,53,side-24,28);bandChoice.setBounds(left+12,53,side/2-12,28);bandOn.setBounds(left+side/2+6,53,side/2-18,28);
  monoBass.setBounds(left+12,getHeight()-48,side/2-12,28);mono.setBounds(left+side/2+6,getHeight()-48,side/2-18,28);
 }
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff101318));g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(23));g.drawText(kind==0?"Dynamic EQ":(kind==1?"Smart Dynamics":"Stereo Architect"),24,6,getWidth()-280,36,juce::Justification::left);
  const int side=std::min(340,getWidth()/2-24);const float right=getWidth()-side-24.f,plotWidth=right-48.f;
  g.setColour(juce::Colour(0xff191d25));g.fillRoundedRectangle(24,54,plotWidth,getHeight()-94.f,18);g.fillRoundedRectangle(right,48,side,getHeight()-72.f,18);
  g.setColour(muted);g.setFont(theme.font(11));g.drawText("Warm DSP / 20 ms bypass / input before the main EQ-limiter pair",24,getHeight()-32,getWidth()-48,22,juce::Justification::left);
  if(kind==1){
   const float x=52,y=96,w=plotWidth-56,h=std::max(50.f,(getHeight()-190.f)*.58f);
   auto px=[&](float db){return x+w*(db+60)/60;};auto py=[&](float db){return y+h-h*(std::clamp(db,-60.f,12.f)+60)/72;};
   g.setColour(muted);g.drawText("Transfer / dBFS   (steady-state target, before colour)",42,65,static_cast<int>(plotWidth)-24,22,juce::Justification::left);
   for(int db=-60;db<=0;db+=12){g.setColour(muted.withAlpha(.18f));g.drawLine(px(db),y,px(db),y+h);g.drawLine(x,py(static_cast<float>(db)),x+w,py(static_cast<float>(db)));}
   g.setColour(muted);g.setFont(theme.font(10));for(int db=-60;db<=0;db+=12){g.drawText(juce::String(db),static_cast<int>(px(db))-16,static_cast<int>(y+h)+3,32,16,juce::Justification::centred);}
   juce::Path path;for(int i=0;i<=120;++i){float in=-60+i*.5f,out=in-masterlab::SmartCompressor::reduction(in,value("dynThreshold"),value("dynRatio"),value("dynKnee")+(value("dynamicsMode")>.5f?3:0))+masterlab::SmartCompressor::upward(in,value("upwardThreshold"),value("upwardAmount"))+value("dynMakeup");float outMixed=in+20*std::log10(std::max(1e-9f,1-value("dynMix")+value("dynMix")*masterlab::dbGain(out-in)));if(i==0)path.startNewSubPath(px(in),py(outMixed));else path.lineTo(px(in),py(outMixed));}
   g.setColour(accent);g.strokePath(path,juce::PathStrokeType(2));g.setColour(gold);float input=std::clamp(frame.dynamics.inputDb,-60.f,0.f);g.drawVerticalLine(static_cast<int>(px(input)),y,y+h);
   const float historyY=y+h+42,historyH=std::max(20.f,getHeight()-historyY-74);g.setColour(muted);g.drawText("Gain history / GUI snapshots",42,static_cast<int>(historyY)-24,static_cast<int>(plotWidth)-24,20,juce::Justification::left);
   for(int curve=0;curve<2;++curve){juce::Path line;for(size_t i=0;i<historyCount;++i){auto item=history[(historyHead+history.size()-historyCount+i)%history.size()];float xx=x+w*i/std::max<size_t>(1,historyCount-1),yy=historyY+historyH-std::clamp(item[curve]/18.f,0.f,1.f)*historyH;if(i==0)line.startNewSubPath(xx,yy);else line.lineTo(xx,yy);}g.setColour(curve==0?gold:accent);g.strokePath(line,juce::PathStrokeType(1.5f));}
   g.setColour(gold);g.drawText("Down "+juce::String(frame.dynamics.reduction,1)+" dB",42,getHeight()-66,145,20,juce::Justification::left);g.setColour(accent);g.drawText("Up "+juce::String(frame.dynamics.upward,1)+" dB",195,getHeight()-66,145,20,juce::Justification::left);
  }
  if(kind==2){
   g.setColour(muted);g.drawText("Per-band stereo / 400 ms smoothing",42,66,static_cast<int>(plotWidth)-24,22,juce::Justification::left);
   constexpr std::array<const char*,4> names{"Low","Mid","High","Air"};for(int i=0;i<4;++i){const bool compact=getHeight()<470;int y=104+i*(compact?56:std::max(66,(getHeight()-190)/4));g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(16));g.drawText(names[i],42,y,90,24,juce::Justification::left);g.setFont(theme.font(11));g.setColour(muted);g.drawText("Corr "+juce::String(frame.stereo.correlation[i],3)+"   S/M "+juce::String(frame.stereo.width[i],2),42,y+(compact?22:28),static_cast<int>(plotWidth)-36,22,juce::Justification::left);
    float x=44,w=plotWidth>450?plotWidth*.49f:plotWidth-40;g.setColour(muted.withAlpha(.2f));g.fillRoundedRectangle(x,y+(compact?46.f:56.f),w,6,3);g.setColour(frame.stereo.correlation[i]<0?gold:accent);float c=frame.stereo.correlation[i];g.fillRect(x+w*.5f+std::min(0.f,c)*w*.5f,y+(compact?46.f:56.f),std::max(1.f,std::abs(c)*w*.5f),6.f);}
  }
  if(kind==2&&plotWidth>450){
   const float cx=24+plotWidth-118,cy=214,radius=82;g.setColour(muted.withAlpha(.3f));g.drawEllipse(cx-radius,cy-radius,radius*2,radius*2,1);g.drawLine(cx-radius,cy,cx+radius,cy);g.drawLine(cx,cy-radius,cx,cy+radius);g.setColour(accent.withAlpha(.55f));
   for(auto point:scope)g.fillEllipse(cx+std::clamp(point.x,-1.f,1.f)*radius-1,cy-std::clamp(point.y,-1.f,1.f)*radius-1,2,2);
   g.setColour(muted);g.drawText("Full-chain output vectorscope",static_cast<int>(cx)-104,static_cast<int>(cy+radius)+14,208,40,juce::Justification::centred);
  }
  if(kind==0){
   g.setColour(muted);g.drawText("Live filter response / gold: selected band",42,66,static_cast<int>(plotWidth)-24,22,juce::Justification::left);
   const float x=44,y=106,w=plotWidth-40,h=std::max(50.f,getHeight()-240.f);
   for(int db=-12;db<=12;db+=6){float yy=y+h*.5f-db*h/24;g.setColour(muted.withAlpha(.2f));g.drawLine(x,yy,x+w,yy);g.setColour(muted);g.drawText(juce::String(db),27,static_cast<int>(yy)-7,16,14,juce::Justification::right);}
   g.setColour(muted);g.drawText("20 Hz",44,static_cast<int>(y+h)+2,60,16,juce::Justification::left);g.drawText("Nyquist",static_cast<int>(x+w)-65,static_cast<int>(y+h)+2,65,16,juce::Justification::right);
   for(int band=0;band<3;++band){juce::Path path;for(int i=0;i<256;++i){double hz=20*std::pow(frame.rate*.5/20,i/255.);float db=static_cast<float>(20*std::log10(std::max(1e-9,std::abs((1.-frame.dynamicBlends[band])+static_cast<double>(frame.dynamicBlends[band])*frame.dynamicBands[band].response(frame.rate,hz)))));float yy=y+h*.5f-std::clamp(db,-12.f,12.f)*h/24;if(i==0)path.startNewSubPath(x,yy);else path.lineTo(x+w*i/255.f,yy);}g.setColour(juce::Colour(std::array<juce::uint32,3>{0xffe7bb6a,0xff82dfcd,0xffb6a0ff}[static_cast<size_t>(band)]).withAlpha(band==selectedBand-1?1.f:.7f));g.strokePath(path,juce::PathStrokeType(2));g.drawText("Band "+juce::String(band+1)+"  "+juce::String(frame.dynamicGain[band],1)+" dB dynamic",42,static_cast<int>(y+h)+20+band*22,static_cast<int>(plotWidth)-24,22,juce::Justification::left);}
  }
 }
};
