#pragma once
#include "StudioTheme.h"
// UI-only controls and plots. Receives coherent DSP snapshots from the editor FIFO.
class ModulePanel final : public juce::Component {
 Processor& processor;StudioTheme& theme;int kind;
 struct Control {juce::Label label;juce::Slider slider;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
 juce::Viewport controlsViewport;juce::Component controlsContent;
 std::vector<std::unique_ptr<Control>> controls;
 juce::ToggleButton present{"In chain"},enabled{"Active"},monoBass{"Mono bass"},mono{"Mono preview"},bandOn{"Band active"};
 juce::ComboBox mode,bandChoice,profile,intensity;juce::ToggleButton tame{"Tame transients"},stabilizerDelta{"Delta"},dynamicsDelta{"Delta"},dynamicEqDelta{"DELTA"};std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tameAttachment,stabilizerDeltaAttachment,dynamicsDeltaAttachment,dynamicEqDeltaAttachment;std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> profileAttachment,intensityAttachment;
 std::unique_ptr<juce::ParameterAttachment> presenceAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment,monoBassAttachment,monoAttachment,bandAttachment;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
 Processor::ResponseFrame frame;std::array<juce::Point<float>,1024> scope{};
 std::array<std::array<float,3>,240> history{};
 std::array<std::array<float,masterlab::dynamicEqBandCount>,240> dynamicHeat{};
 std::array<std::array<float,2>,240> compressorEnergy{};
 size_t historyHead=0,historyCount=0;int selectedBand=1;
 std::array<float,512> stereoSpectrum{};int draggedSplit=-1;
 std::array<juce::TextButton,4> bandPower,bandSolo;juce::ToggleButton deltaMonitor{"Delta"};std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> deltaAttachment;std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,4> bandPowerAttachments,bandSoloAttachments;
 std::array<juce::Colour,4> bandColours{juce::Colour(0xff5fcce8),juce::Colour(0xffd67bea),juce::Colour(0xffef907d),juce::Colour(0xffa7dfcb)};
 juce::Rectangle<float> stereoPlot()const{return {24,86,getWidth()-48.f,std::max(82.f,(getHeight()-166.f)*.43f)};}
 float stereoX(float hz)const{auto p=stereoPlot();return p.getX()+p.getWidth()*std::log(std::max(20.f,hz)/20.f)/std::log(static_cast<float>(frame.rate*.5)/20.f);}
 std::array<const char*,3> splitIds{"stereoLowSplit","stereoHighSplit","stereoTopSplit"};
 const juce::Colour accent{0xff82dfcd},gold{0xffe7bb6a},muted{0xffabb5c3};
 float value(const juce::String& id)const{return processor.state.getRawParameterValue(id)->load();}
 void add(const char* name,const juce::String& id,const char* suffix=""){
  auto control=std::make_unique<Control>();controlsContent.addAndMakeVisible(control->label);controlsContent.addAndMakeVisible(control->slider);control->label.setText(name,juce::dontSendNotification);control->label.setColour(juce::Label::textColourId,muted);
  control->slider.setSliderStyle(juce::Slider::LinearHorizontal);control->slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,110,24);control->slider.setTextValueSuffix(suffix);control->slider.setComponentID(id);
  if(kind==0)control->slider.onDragStart=[this]{auto* p=processor.state.getParameter("deqBandOn"+juce::String(selectedBand));if(p->getValue()<.5f)p->setValueNotifyingHost(1);};
  control->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,control->slider);controls.push_back(std::move(control));
 }
 void bindBand(){
  static const std::array<const char*,7> ids{"deqFreq","deqGain","deqQ","deqThreshold","deqRange","deqAttack","deqRelease"};
  for(size_t i=0;i<controls.size();++i){controls[i]->attachment.reset();controls[i]->slider.setComponentID(juce::String(ids[i])+juce::String(selectedBand));controls[i]->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,juce::String(ids[i])+juce::String(selectedBand),controls[i]->slider);}
  bandAttachment.reset();bandAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"deqBandOn"+juce::String(selectedBand),bandOn);
 }
public:
 juce::Slider* controlForParameter(const juce::String& id){for(auto& control:controls)if(control->slider.getComponentID()==id)return &control->slider;return nullptr;}
 ModulePanel(Processor& p,StudioTheme& t,int type):processor(p),theme(t),kind(type){
  setLookAndFeel(&theme);addAndMakeVisible(controlsViewport);controlsViewport.setViewedComponent(&controlsContent,false);controlsViewport.setScrollBarsShown(true,false);const char* presence=kind==3?"stabilizerPresent":kind==0?"dynamicEqPresent":(kind==1?"dynamicsPresent":"stereoPresent");const char* on=kind==3?"stabilizerOn":kind==0?"dynamicEqOn":(kind==1?"dynamicsOn":"stereoOn");
  addAndMakeVisible(present);addAndMakeVisible(enabled);present.setComponentID(juce::String(presence));enabled.setComponentID(on);
  presenceAttachment=std::make_unique<juce::ParameterAttachment>(*processor.state.getParameter(presence),[this](float v){present.setToggleState(v>.5f,juce::dontSendNotification);},nullptr);presenceAttachment->sendInitialUpdate();enabledAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,on,enabled);
  present.onClick=[this]{processor.setModulePresentUI(kind==3?5:kind,present.getToggleState());};
  enabled.onClick=[this]{if(enabled.getToggleState()&&!present.getToggleState())processor.setModulePresentUI(kind==3?5:kind,true);};
  present.setTooltip("Add or remove this processor from the signal chain. Settings are retained; changes crossfade.");enabled.setTooltip("Bypass this module independently without removing it. No added latency.");
  if(kind==0){addAndMakeVisible(dynamicEqDelta);dynamicEqDelta.setComponentID("dynamic-eq-delta");dynamicEqDeltaAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"dynamicEqDelta",dynamicEqDelta);dynamicEqDelta.setTooltip("Listen to the time-aligned exact input-minus-output Dynamic EQ signal.");add("Frequency","deqFreq1"," Hz");add("Static gain","deqGain1"," dB");add("Q","deqQ1");add("Threshold","deqThreshold1"," dBFS");add("Dynamic range","deqRange1"," dB");add("Attack","deqAttack1"," ms");add("Release","deqRelease1"," ms");addAndMakeVisible(bandChoice);addAndMakeVisible(bandOn);for(int i=1;i<=8;++i)bandChoice.addItem("Band "+juce::String(i),i);bandChoice.setSelectedId(1);bandChoice.onChange=[this]{selectedBand=bandChoice.getSelectedId();bindBand();};bindBand();}
  if(kind==1){add("Threshold","dynThreshold"," dBFS");add("Ratio","dynRatio",":1");add("Upward maximum","upwardAmount"," dB");add("Upward threshold","upwardThreshold"," dBFS");add("Attack","dynAttack"," ms");add("Release","dynRelease"," ms");add("Knee","dynKnee"," dB");add("Makeup","dynMakeup"," dB");add("Dry / wet","dynMix");add("Punch","dynPunch"," %");addAndMakeVisible(mode);mode.addItem("Digital",1);mode.addItem("Analog",2);modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"dynamicsMode",mode);mode.setTooltip("Digital: clean gain processing. Analog: slower detector, wider knee and the existing mild arctangent colour. No hardware-emulation or oversampling claim.");addAndMakeVisible(intensity);intensity.setComponentID("dynamics-intensity");for(const char* name:{"Legacy","Clean","Strong","Extreme"})intensity.addItem(name,intensity.getNumItems()+1);intensityAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"dynamicsIntensity",intensity);intensity.setTooltip("Explicit gain-reduction profile. Legacy preserves old behavior; Strong and Extreme increase measured reduction without hidden makeup gain.");addAndMakeVisible(dynamicsDelta);dynamicsDelta.setComponentID("dynamics-delta");dynamicsDeltaAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"dynDelta",dynamicsDelta);dynamicsDelta.setTooltip("Listen to the exact processed-minus-input contribution after Dry/Wet.");}
  if(kind==3){add("Amount","stabilizerAmount"," %");add("Speed (slow at 100)","stabilizerSpeed"," %");add("Smoothing","stabilizerSmoothing"," %");add("Low","stabilizerLow"," %");add("Mid","stabilizerMid"," %");add("High","stabilizerHigh"," %");addAndMakeVisible(mode);mode.addItem("Shape",1);mode.addItem("Cut",2);modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"stabilizerMode",mode);addAndMakeVisible(profile);profile.addItem("Neutral",1);profile.addItem("Electronic",2);profile.addItem("Acoustic",3);profileAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"stabilizerProfile",profile);addAndMakeVisible(intensity);intensity.setComponentID("stabilizer-intensity");for(const char* name:{"Legacy","Clean","Strong","Extreme"})intensity.addItem(name,intensity.getNumItems()+1);intensityAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,"stabilizerIntensity",intensity);intensity.setTooltip("Bounded correction authority. Legacy preserves old presets; Clean, Strong and Extreme add spectral-support and continuous transient protection.");addAndMakeVisible(tame);addAndMakeVisible(stabilizerDelta);tameAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stabilizerTame",tame);stabilizerDeltaAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stabilizerDelta",stabilizerDelta);}
  if(kind==2){add("Low width","stereoLowWidth"," x");add("Mid width","stereoMidWidth"," x");add("High width","stereoHighWidth"," x");add("Air width","stereoAirWidth"," x");add("Low split","stereoLowSplit"," Hz");add("Mid split","stereoHighSplit"," Hz");add("Top split","stereoTopSplit"," Hz");add("Balance","stereoBalance");add("Amount","stereoAmount");add("Stereoize","stereoizeAmount");controls.back()->slider.setTooltip("Optional bass-protected Mid-to-Side phase rotation. 0 preserves legacy width. Added Side cancels in mono; stereo level and peaks can rise. Use Mono preview to compare.");addAndMakeVisible(monoBass);addAndMakeVisible(mono);monoBassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stereoMonoBass",monoBass);monoAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stereoMono",mono);addAndMakeVisible(deltaMonitor);deltaAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stereoDelta",deltaMonitor);
   for(size_t i=0;i<4;++i){auto suffix=juce::String(static_cast<int>(i)+1);for(auto* b:{&bandPower[i],&bandSolo[i]}){controlsContent.addAndMakeVisible(b);b->setClickingTogglesState(true);}bandPower[i].setButtonText("On");bandSolo[i].setButtonText("S");bandPower[i].setTooltip("Band processing on/off; disabled width returns to unity.");bandSolo[i].setTooltip("Audition this frequency band. Multiple solos may be combined.");bandPowerAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stereoBandOn"+suffix,bandPower[i]);bandSoloAttachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"stereoSolo"+suffix,bandSolo[i]);}
}
 }
 ~ModulePanel()override{controlsViewport.setViewedComponent(nullptr,false);setLookAndFeel(nullptr);}
 void updateSpectrum(const std::array<float,512>& values){stereoSpectrum=values;}
 void mouseDown(const juce::MouseEvent& event)override{
  if(kind!=2)return;auto p=stereoPlot();if(!p.expanded(0,14).contains(event.position))return;
  for(int i=0;i<3;++i)if(std::abs(event.position.x-stereoX(value(splitIds[i])))<14){draggedSplit=i;processor.beginUiTransaction("Stereo crossover");processor.state.getParameter(splitIds[i])->beginChangeGesture();break;}
 }
 void mouseDrag(const juce::MouseEvent& event)override{
  if(draggedSplit<0)return;auto p=stereoPlot();auto* parameter=processor.state.getParameter(splitIds[draggedSplit]);float hz=20*std::pow(static_cast<float>(frame.rate*.5)/20,std::clamp((event.position.x-p.getX())/p.getWidth(),0.f,1.f));auto range=parameter->getNormalisableRange();hz=std::clamp(hz,range.start,range.end);parameter->setValueNotifyingHost(parameter->convertTo0to1(hz));repaint();
 }
 void mouseUp(const juce::MouseEvent&)override{if(draggedSplit>=0){processor.state.getParameter(splitIds[draggedSplit])->endChangeGesture();processor.finishUiTransaction();draggedSplit=-1;}}
 void updateScope(const std::array<juce::Point<float>,1024>& points){scope=points;}
 void update(const Processor::ResponseFrame& f){
  frame=f;
  history[historyHead]={f.dynamics.reduction,f.dynamics.upward,f.dynamics.punch};
  dynamicHeat[historyHead]=f.dynamicGain;
  compressorEnergy[historyHead]={f.dynamics.measuredInputDb,f.dynamics.measuredOutputDb};
  historyHead=(historyHead+1)%history.size();
  historyCount=std::min(history.size(),historyCount+1);repaint();
 }
 void resized()override{
  present.setBounds(getWidth()-264,10,130,34);enabled.setBounds(getWidth()-128,10,118,34);
  const int side=std::min(340,getWidth()/2-24),left=getWidth()-side-24;
  const int row=54;controlsViewport.setBounds(left+12,94,side-24,getHeight()-154);controlsContent.setSize(side-40,static_cast<int>(controls.size())*row);
  for(size_t i=0;i<controls.size();++i){int y=static_cast<int>(i)*row;controls[i]->label.setBounds(0,y,side-40,18);controls[i]->slider.setBounds(0,y+20,side-40,30);}
  if(kind==2){
   controlsViewport.setBounds(24,static_cast<int>(stereoPlot().getBottom())+34,getWidth()*.43f,getHeight()-static_cast<int>(stereoPlot().getBottom())-94);
   controlsContent.setSize(controlsViewport.getWidth()-16,controlsViewport.getHeight());
   const int column=controlsContent.getWidth()/4;
   for(size_t i=0;i<controls.size();++i){auto& c=*controls[i];c.label.setVisible(i<4);c.slider.setVisible(i<4);if(i<4){c.label.setBounds(static_cast<int>(i)*column,0,column,22);c.slider.setSliderStyle(juce::Slider::LinearVertical);c.slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,column-8,24);c.slider.setColour(juce::Slider::trackColourId,bandColours[i]);c.slider.setColour(juce::Slider::thumbColourId,bandColours[i]);c.slider.setBounds(static_cast<int>(i)*column+4,56,column-8,controlsContent.getHeight()-58);bandPower[i].setBounds(static_cast<int>(i)*column+3,25,column/2-4,23);bandSolo[i].setBounds(static_cast<int>(i)*column+column/2+1,25,column/2-4,23);}}
   for(int i:{7,8,9}){addAndMakeVisible(controls[i]->label);addAndMakeVisible(controls[i]->slider);}controls[7]->label.setBounds(getWidth()-285,51,70,24);controls[7]->slider.setBounds(getWidth()-213,51,185,24);controls[8]->label.setBounds(320,getHeight()-50,60,28);controls[8]->slider.setBounds(380,getHeight()-50,(getWidth()-428)/2-30,28);controls[8]->slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,50,24);controls[9]->slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,50,24);controls[9]->label.setBounds(getWidth()/2+170,getHeight()-50,65,28);controls[9]->slider.setBounds(getWidth()/2+235,getHeight()-50,getWidth()/2-259,28);deltaMonitor.setBounds(300,10,104,34);
   monoBass.setBounds(24,getHeight()-52,140,34);mono.setBounds(170,getHeight()-52,150,34);
  }
  mode.setBounds(left+12,53,(kind==3?(side-36)/3:kind==1?(side-30)/2:side-24),28);if(kind==0)dynamicEqDelta.setBounds(24,getHeight()-51,112,34);if(kind==1){intensity.setBounds(left+side/2,53,side/2-12,28);dynamicsDelta.setBounds(getWidth()-116,getHeight()-51,108,34);}if(kind==3){const int column=(side-36)/3;profile.setBounds(left+18+column,53,column,28);intensity.setBounds(left+24+column*2,53,column,28);tame.setBounds(left+12,getHeight()-51,side-126,34);stabilizerDelta.setBounds(getWidth()-116,getHeight()-51,108,34);}bandChoice.setBounds(left+12,53,side/2-12,28);bandOn.setBounds(left+side/2+6,53,side/2-18,28);
  if(kind!=2){monoBass.setBounds(left+12,getHeight()-48,side/2-12,28);mono.setBounds(left+side/2+6,getHeight()-48,side/2-18,28);}
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
  g.drawVerticalLine(juce::jmax(10,getWidth()-11),0.f,static_cast<float>(getHeight()));g.setColour(juce::Colours::whitesmoke);g.setFont(theme.font(23));g.drawText(kind==3?"Stabilizer":kind==0?"Dynamic EQ":(kind==1?"Smart Dynamics":"Stereo Architect"),24,6,getWidth()-280,36,juce::Justification::left);
  const int side=std::min(340,getWidth()/2-24);const float right=getWidth()-side-24.f,plotWidth=right-48.f;
  g.setColour(juce::Colour(0xff191d25));g.fillRoundedRectangle(24,54,plotWidth,getHeight()-94.f,18);g.fillRoundedRectangle(right,48,side,getHeight()-72.f,18);
  g.setColour(muted);g.setFont(theme.font(11));g.drawText("Live DSP / 20 ms bypass / drag chain cards to change processing order",24,getHeight()-32,getWidth()-48,22,juce::Justification::left);
  if(kind==3){
   const float x=48,y=116,w=plotWidth-48,h=getHeight()-222.f;g.setColour(muted);g.drawText("Actual adaptive response / original octave-band DSP",40,68,static_cast<int>(plotWidth)-24,26,juce::Justification::left);g.setFont(theme.font(10));for(int db=-12;db<=12;db+=3){float yy=y+h*.5f-db*h/24;g.setColour(muted.withAlpha(db==0?.4f:.12f));g.drawLine(x,yy,x+w,yy);g.setColour(muted);g.drawText(juce::String(db),25,static_cast<int>(yy)-7,22,16,juce::Justification::right);}
   for(float hz:{40.f,100.f,300.f,1000.f,3000.f,10000.f}){float xx=x+w*std::log(hz/20)/std::log(static_cast<float>(frame.rate*.5)/20);g.setColour(muted.withAlpha(.12f));g.drawLine(xx,y,xx,y+h);g.setColour(muted);g.drawText(hz>=1000?juce::String(hz/1000,0)+"k":juce::String(hz,0),static_cast<int>(xx)-20,static_cast<int>(y+h)+4,40,20,juce::Justification::centred);}
   juce::Path path;for(int i=0;i<400;++i){double hz=20*std::pow(frame.rate*.5/20,i/399.);std::complex<double> transfer{1,0};for(const auto& filter:frame.stabilizerBands)transfer*=filter.response(frame.rate,hz);transfer=(1.-frame.stabilizerMix)+static_cast<double>(frame.stabilizerMix)*transfer;float db=static_cast<float>(20*std::log10(std::max(1e-12,std::abs(transfer)))),xx=x+w*i/399,yy=y+h*.5f-std::clamp(db,-12.f,12.f)*h/24;if(i==0)path.startNewSubPath(xx,yy);else path.lineTo(xx,yy);}g.setColour(accent);g.strokePath(path,juce::PathStrokeType(2));g.setColour(muted);g.drawText("Shape: bounded cuts/boosts    Cut: attenuation only",40,getHeight()-82,static_cast<int>(plotWidth)-24,24,juce::Justification::left);
  }
  if(kind==1){
   const float x=52,y=96,w=plotWidth-56,h=std::max(50.f,(getHeight()-190.f)*.58f);
   auto px=[&](float db){return x+w*(db+60)/60;};auto py=[&](float db){return y+h-h*(std::clamp(db,-60.f,12.f)+60)/72;};
   g.setColour(muted);g.drawText("Transfer / dBFS   (steady-state target, before colour)",42,65,static_cast<int>(plotWidth)-24,22,juce::Justification::left);
   for(int db=-60;db<=0;db+=12){g.setColour(muted.withAlpha(.18f));g.drawLine(px(db),y,px(db),y+h);g.drawLine(x,py(static_cast<float>(db)),x+w,py(static_cast<float>(db)));}
   g.setColour(muted);g.setFont(theme.font(10));for(int db=-60;db<=0;db+=12){g.drawText(juce::String(db),static_cast<int>(px(db))-16,static_cast<int>(y+h)+3,32,16,juce::Justification::centred);}
   const int intensityMode=std::clamp(static_cast<int>(value("dynamicsIntensity")),0,3);const float grScale=std::array<float,4>{1.f,.8f,1.4f,1.9f}[intensityMode],upScale=std::array<float,4>{1.f,.85f,1.2f,1.45f}[intensityMode];juce::Path path;for(int i=0;i<=120;++i){float in=-60+i*.5f,out=in-masterlab::SmartCompressor::reduction(in,value("dynThreshold"),value("dynRatio"),value("dynKnee")+(value("dynamicsMode")>.5f?3:0))*grScale+masterlab::SmartCompressor::upward(in,value("upwardThreshold"),value("upwardAmount"))*upScale+value("dynMakeup");float outMixed=in+20*std::log10(std::max(1e-9f,1-value("dynMix")+value("dynMix")*masterlab::dbGain(out-in)));if(i==0)path.startNewSubPath(px(in),py(outMixed));else path.lineTo(px(in),py(outMixed));}
   g.setColour(accent);g.strokePath(path,juce::PathStrokeType(2));g.setColour(gold);float input=std::clamp(frame.dynamics.inputDb,-60.f,0.f);g.drawVerticalLine(static_cast<int>(px(input)),y,y+h);
   const float historyY=y+h+42,historyH=std::max(20.f,getHeight()-historyY-74);g.setColour(muted);g.drawText("Gain history / GUI snapshots",42,static_cast<int>(historyY)-24,static_cast<int>(plotWidth)-24,20,juce::Justification::left);
   // Measured full-band input and output energies, not synthetic curves.
   const float cellW=w/static_cast<float>(history.size());
   const float cellH=historyH*.5f;
   for(size_t ix=0;ix<historyCount;++ix){
    const auto row=(historyHead+history.size()-historyCount+ix)%history.size();
    const auto levels=compressorEnergy[row];
    for(int lane=0;lane<2;++lane){
     const float level=std::clamp((levels[static_cast<size_t>(lane)]+72.f)/72.f,0.f,1.f);
     const auto colour=lane==0?juce::Colour(0xffdca25d):juce::Colour(0xff55c3e4);
     g.setColour(colour.withAlpha(.08f+.36f*level));
     g.fillRect(x+ix*cellW,historyY+lane*cellH,cellW+1.f,cellH);
    }
   }
   for(int curve=0;curve<3;++curve){juce::Path line;for(size_t i=0;i<historyCount;++i){auto item=history[(historyHead+history.size()-historyCount+i)%history.size()];float xx=x+w*i/std::max<size_t>(1,historyCount-1),yy=historyY+historyH-std::clamp(item[curve]/18.f,0.f,1.f)*historyH;if(i==0)line.startNewSubPath(xx,yy);else line.lineTo(xx,yy);}g.setColour(curve==0?gold:(curve==1?accent:juce::Colour(0xff6cb5ff)));g.strokePath(line,juce::PathStrokeType(1.5f));}
   g.setColour(gold);g.drawText("Down "+juce::String(frame.dynamics.reduction,1)+" dB",42,getHeight()-66,145,20,juce::Justification::left);g.setColour(accent);g.drawText("Up "+juce::String(frame.dynamics.upward,1)+" dB",195,getHeight()-66,145,20,juce::Justification::left);
  }
  if(kind==2){
   g.setColour(juce::Colour(0xff101318));g.fillRect(0,48,getWidth(),getHeight()-48);
   const auto p=stereoPlot();g.setColour(juce::Colour(0xff171e26));g.fillRoundedRectangle(p,12);
   std::array<float,5> edges{p.getX(),stereoX(value(splitIds[0])),stereoX(value(splitIds[1])),stereoX(value(splitIds[2])),p.getRight()};
   for(int band=0;band<4;++band){g.setColour(bandColours[band].withAlpha(.045f));g.fillRect(edges[band],p.getY(),edges[band+1]-edges[band],p.getHeight());}
   g.setFont(theme.font(10));for(float hz:{20.f,60.f,100.f,300.f,1000.f,3000.f,10000.f,20000.f}){if(hz>frame.rate*.5)continue;float x=stereoX(hz);g.setColour(muted.withAlpha(.13f));g.drawVerticalLine(static_cast<int>(x),p.getY(),p.getBottom());g.setColour(muted);g.drawText(hz>=1000?juce::String(hz/1000,0)+"k":juce::String(hz,0),static_cast<int>(x)-20,static_cast<int>(p.getBottom())+3,40,18,juce::Justification::centred);}
   for(int db=-90;db<=0;db+=30){g.setColour(muted.withAlpha(.1f));float y=p.getBottom()-p.getHeight()*(db+100)/100;g.drawHorizontalLine(static_cast<int>(y),p.getX(),p.getRight());}
   juce::Path spectrum;for(size_t i=0;i<stereoSpectrum.size();++i){float x=p.getX()+p.getWidth()*i/(stereoSpectrum.size()-1),db=masterlab::levelDb(stereoSpectrum[i]),y=p.getBottom()-p.getHeight()*std::clamp((db+100)/100.f,0.f,1.f);if(i==0)spectrum.startNewSubPath(x,y);else spectrum.lineTo(x,y);}
   auto fill=spectrum;fill.lineTo(p.getRight(),p.getBottom());fill.lineTo(p.getX(),p.getBottom());fill.closeSubPath();g.setColour(accent.withAlpha(.1f));g.fillPath(fill);g.setColour(accent.withAlpha(.65f));g.strokePath(spectrum,juce::PathStrokeType(1.2f));
   for(int i=0;i<3;++i){float x=edges[i+1];g.setColour(bandColours[i]);g.drawVerticalLine(static_cast<int>(x),p.getY(),p.getBottom());g.fillRoundedRectangle(x-5,p.getY()-8,10,24,5);g.setColour(juce::Colours::whitesmoke);g.drawText(juce::String(value(splitIds[i]),0)+" Hz",static_cast<int>(x)-44,static_cast<int>(p.getY())+20,88,18,juce::Justification::centred);}
   g.setColour(muted);g.setFont(theme.font(11));g.drawText((value("stereoPresent")<.5f?"REMOVED FROM CHAIN":value("stereoOn")<.5f?"BYPASSED":value("routingMode")>.5f?"ACTIVE / PARALLEL: branch averaged":"ACTIVE / SERIAL"),28,52,getWidth()-342,24,juce::Justification::left);
   const float sx=getWidth()*.49f,sy=p.getBottom()+36,sw=getWidth()-sx-40,sh=getHeight()-sy-58,cx=sx+sw*.5f,cy=sy+sh-34,radius=std::max(20.f,std::min(sw*.43f,sh-60));
   g.setColour(juce::Colour(0xff171e26));g.fillRoundedRectangle(sx,sy,sw,sh,16);g.setColour(muted);g.drawText("OUTPUT VECTORSCOPE",static_cast<int>(sx)+16,static_cast<int>(sy)+8,static_cast<int>(sw)-32,22,juce::Justification::left);
   juce::Path arc;for(int i=0;i<=100;++i){float a=juce::MathConstants<float>::pi*i/100,x=cx-radius*std::cos(a),y=cy-radius*std::sin(a);if(i==0)arc.startNewSubPath(x,y);else arc.lineTo(x,y);}arc.closeSubPath();g.setColour(muted.withAlpha(.25f));g.strokePath(arc,juce::PathStrokeType(1));g.drawLine(cx,cy,cx,cy-radius);g.drawLine(cx-radius*.7071f,cy-radius*.7071f,cx,cy);g.drawLine(cx+radius*.7071f,cy-radius*.7071f,cx,cy);
   g.setColour(accent.withAlpha(.45f));for(auto point:scope){float side=std::clamp(-point.x*.5f,-1.f,1.f),mid=std::clamp(std::abs(point.y)*.5f,0.f,1.f);if(side*side+mid*mid>1){float norm=1/std::sqrt(side*side+mid*mid);side*=norm;mid*=norm;}g.fillEllipse(cx+side*radius-1,cy-mid*radius-1,2,2);}
   const float corr=std::clamp(frame.outputReading.correlation,-1.f,1.f);g.setColour(muted.withAlpha(.25f));g.fillRoundedRectangle(sx+18,sy+sh-18,sw-36,5,2);g.setColour(corr<0?gold:accent);g.fillEllipse(sx+18+(sw-36)*(corr+1)*.5f-4,sy+sh-20,9,9);g.setColour(muted);g.drawText((corr<0?"CHECK MONO / C ":"Correlation ")+juce::String(corr,3),static_cast<int>(sx)+16,static_cast<int>(sy+sh)-43,static_cast<int>(sw)-32,20,juce::Justification::left);
   for(int band=0;band<4;++band){g.setColour(bandColours[band]);g.setFont(theme.font(10));float x=24+band*(getWidth()*.43f-16)/4;g.drawFittedText("W "+juce::String(frame.stereo.width[band],2)+" / C "+juce::String(frame.stereo.correlation[band],2),static_cast<int>(x),static_cast<int>(p.getBottom())+18,static_cast<int>((getWidth()*.43f-16)/4)-2,18,juce::Justification::centred,1);}
  }
  if(kind==0){
   g.setColour(muted);g.drawText("Live filter response / gold: selected band",42,66,static_cast<int>(plotWidth)-24,22,juce::Justification::left);
   const float x=44,y=106,w=plotWidth-40,h=std::max(50.f,getHeight()-240.f);
   for(int db=-12;db<=12;db+=6){float yy=y+h*.5f-db*h/24;g.setColour(muted.withAlpha(.2f));g.drawLine(x,yy,x+w,yy);g.setColour(muted);g.drawText(juce::String(db),27,static_cast<int>(yy)-7,16,14,juce::Justification::right);}
   g.setColour(muted);g.drawText("20 Hz",44,static_cast<int>(y+h)+2,60,16,juce::Justification::left);g.drawText("Nyquist",static_cast<int>(x+w)-65,static_cast<int>(y+h)+2,65,16,juce::Justification::right);
   for(int band=0;band<8;++band){juce::Path path;for(int i=0;i<256;++i){double hz=20*std::pow(frame.rate*.5/20,i/255.);float db=static_cast<float>(20*std::log10(std::max(1e-9,std::abs((1.-frame.dynamicBlends[band])+static_cast<double>(frame.dynamicBlends[band])*frame.dynamicBands[band].response(frame.rate,hz)))));float yy=y+h*.5f-std::clamp(db,-12.f,12.f)*h/24;if(i==0)path.startNewSubPath(x,yy);else path.lineTo(x+w*i/255.f,yy);}g.setColour(juce::Colour(std::array<juce::uint32,8>{0xffe7bb6a,0xff82dfcd,0xffb6a0ff,0xfff48ea1,0xff73c9f4,0xffc6df77,0xffdf9ded,0xffffa77d}[static_cast<size_t>(band)]).withAlpha(band==selectedBand-1?1.f:.7f));g.strokePath(path,juce::PathStrokeType(band==selectedBand-1?2.f:1.f));}
   g.setColour(muted);g.drawText("Dynamic EQ  /  measured gain action in each of eight bands",42,static_cast<int>(y+h)+18,static_cast<int>(plotWidth)-24,20,juce::Justification::left);
   const float heatTop=y+h+42.f,heatBottom=static_cast<float>(getHeight()-44);
   const float cellHeight=std::max(2.f,(heatBottom-heatTop)/8.f);
   const float dx=w/static_cast<float>(history.size());
   for(size_t ix=0;ix<historyCount;++ix){
    const auto row=(historyHead+history.size()-historyCount+ix)%history.size();
    for(size_t band=0;band<masterlab::dynamicEqBandCount;++band){
     const float reduction=std::clamp(dynamicHeat[row][band]/12.f,-1.f,1.f);
     const float intensity=std::abs(reduction);
     g.setColour((reduction<0?juce::Colour(0xff4bafff):juce::Colour(0xffffad6a)).withAlpha(.07f+.85f*intensity));
     g.fillRect(x+ix*dx,heatTop+band*cellHeight,dx+1.f,cellHeight);
    }
   }
   g.setColour(muted);g.setFont(theme.font(10));
   g.drawText("Selected band "+juce::String(selectedBand)+" action "+juce::String(frame.dynamicGain[static_cast<size_t>(selectedBand-1)],2)+" dB",42,getHeight()-32,static_cast<int>(plotWidth)-24,20,juce::Justification::left);
  }
 }
};
