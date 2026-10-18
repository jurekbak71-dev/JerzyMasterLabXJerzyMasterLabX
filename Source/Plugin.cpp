#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
class Processor final: public juce::AudioProcessor {
public:
 juce::AudioProcessorValueTreeState state;
 masterlab::RoutedEngine engine;
 std::array<std::vector<float>,2> hostDryDelay;
 std::array<std::array<float,128>,2> hostDryScratch{};
 size_t hostDryPosition=0;float hostBypassMix=0,hostBypassStep=1.f/960.f;bool hostBypassTarget=false;
 std::array<std::atomic<float>*,52> parameters{};
 juce::AbstractFifo fifo{8192};std::array<float,8192> traceL{},traceR{};
 std::atomic<float> peak{0},rms{0},correlation{0};
 static auto layout(){juce::AudioProcessorValueTreeState::ParameterLayout p;
 p.add(std::make_unique<juce::AudioParameterFloat>("frequency","EQ frequency",juce::NormalisableRange<float>(20,20000,1,.3f),1000));
 p.add(std::make_unique<juce::AudioParameterFloat>("gain","EQ gain",-18,18,0));
 p.add(std::make_unique<juce::AudioParameterFloat>("q","EQ Q",.1f,10.f,1.f));
 p.add(std::make_unique<juce::AudioParameterFloat>("ceiling","Sample peak ceiling (dBFS)",-24,0,-1));
 p.add(std::make_unique<juce::AudioParameterBool>("eq","EQ enabled",true));
 p.add(std::make_unique<juce::AudioParameterBool>("lim","Limiter enabled",true));
 p.add(std::make_unique<juce::AudioParameterBool>("order","Limiter before EQ (ceiling applies before EQ)",false));
 for(int band=2;band<=16;++band){const auto id=juce::String(band);const auto title="Band "+id+" ";
 p.add(std::make_unique<juce::AudioParameterFloat>("frequency"+id,title+"frequency",juce::NormalisableRange<float>(20,20000,1,.3f),1000));
 p.add(std::make_unique<juce::AudioParameterFloat>("gain"+id,title+"gain",-18,18,0));
 p.add(std::make_unique<juce::AudioParameterFloat>("q"+id,title+"Q",.1f,10.f,1.f));}return p;}
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"MasterLab",layout()){state.state.setProperty("schemaVersion",1,nullptr);state.state.setProperty("eqInstanceId","eq-0001",nullptr);state.state.setProperty("limiterInstanceId","limiter-0001",nullptr);const char* ids[]={"frequency","gain","q","ceiling","eq","lim","order"};for(size_t i=0;i<7;++i)parameters[i]=state.getRawParameterValue(ids[i]);for(int k=2;k<=16;++k){int offset=7+(k-2)*3;const auto suffix=juce::String(k);parameters[offset]=state.getRawParameterValue("frequency"+suffix);parameters[offset+1]=state.getRawParameterValue("gain"+suffix);parameters[offset+2]=state.getRawParameterValue("q"+suffix);}}
 const juce::String getName()const override{return "JerzyMasterLabX";}
 bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return .1;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return "Default";}void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override{engine.parameters(parameters[0]->load(),parameters[1]->load(),parameters[2]->load(),parameters[3]->load(),parameters[4]->load()>.5f,parameters[5]->load()>.5f);engine.prepare(sr,parameters[6]->load()>.5f);setLatencySamples(engine.latency());for(auto& channel:hostDryDelay)channel.assign(static_cast<size_t>(engine.latency()+1),0);hostDryPosition=0;hostBypassMix=0;hostBypassTarget=false;hostBypassStep=static_cast<float>(1/(std::isfinite(sr)&&sr>=8000&&sr<=384000?sr*.02:960.));}
 void releaseResources()override{}void reset()override{engine.reset();for(auto& channel:hostDryDelay)std::fill(channel.begin(),channel.end(),0);hostDryPosition=0;hostBypassMix=hostBypassTarget?1.f:0.f;}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{renderAudio(b,false);}
 void processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{renderAudio(b,true);}
 void renderAudio(juce::AudioBuffer<float>& b,bool hostBypassed){
 juce::ScopedNoDenormals noDenormals;if(b.getNumChannels()<2)return;
 auto value=[&](size_t index){return parameters[index]->load();};
 engine.parameters(value(0),value(1),value(2),value(3),value(4)>.5f,value(5)>.5f);
 for(size_t k=1;k<16;++k){size_t offset=7+(k-1)*3;engine.setBand(k,value(offset),value(offset+1),value(offset+2));}
 hostBypassTarget=hostBypassed;
 if(hostDryDelay[0].empty()){b.clear();return;}
 for(int offset=0;offset<b.getNumSamples();){int length=std::min(128,b.getNumSamples()-offset);
  for(int i=0;i<length;++i){size_t read=(hostDryPosition+1)%hostDryDelay[0].size();for(int c=0;c<2;++c){hostDryDelay[c][hostDryPosition]=masterlab::clean(b.getSample(c,offset+i));hostDryScratch[c][i]=hostDryDelay[c][read];}hostDryPosition=read;}
  engine.process(b.getWritePointer(0,offset),b.getWritePointer(1,offset),length,value(6)>.5f);
  for(int i=0;i<length;++i){hostBypassMix=hostBypassed?std::min(1.f,hostBypassMix+hostBypassStep):std::max(0.f,hostBypassMix-hostBypassStep);for(int c=0;c<2;++c){float wet=b.getSample(c,offset+i);b.setSample(c,offset+i,wet+hostBypassMix*(hostDryScratch[c][i]-wet));}}
  offset+=length;
 }
 double ll=0,rr=0,lr=0;float pk=0;
 for(int i=0;i<b.getNumSamples();++i){float l=b.getSample(0,i),r=b.getSample(1,i);ll+=l*l;rr+=r*r;lr+=l*r;pk=std::max(pk,std::max(std::abs(l),std::abs(r)));}
 peak.store(pk);rms.store(static_cast<float>(std::sqrt((ll+rr)/std::max(1,2*b.getNumSamples()))));correlation.store(static_cast<float>(lr/std::sqrt(std::max(1e-20,ll*rr))));
 int s1,n1,s2,n2;fifo.prepareToWrite(b.getNumSamples(),s1,n1,s2,n2);
 for(int j=0;j<n1;++j){traceL[s1+j]=b.getSample(0,j);traceR[s1+j]=b.getSample(1,j);}for(int j=0;j<n2;++j){traceL[s2+j]=b.getSample(0,n1+j);traceR[s2+j]=b.getSample(1,n1+j);}fifo.finishedWrite(n1+n2);
 }
 void getStateInformation(juce::MemoryBlock& dest)override{auto xml=state.copyState().createXml();copyXmlToBinary(*xml,dest);}
 void setStateInformation(const void* data,int size)override{auto xml=getXmlFromBinary(data,size);if(xml&&xml->hasTagName("MasterLab"))state.replaceState(juce::ValueTree::fromXml(*xml));}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
};
class Editor final:public juce::AudioProcessorEditor,public juce::DragAndDropContainer,public juce::DragAndDropTarget,private juce::Timer {
 Processor& p;juce::GenericAudioProcessorEditor controls;
 juce::dsp::FFT fft{10};std::array<float,2048> spectrum{};std::array<float,1024> samples{};int head=0;
 std::array<juce::Point<float>,1024> scope{};
 void timerCallback()override{int s1,n1,s2,n2;p.fifo.prepareToRead(p.fifo.getNumReady(),s1,n1,s2,n2);
 auto consume=[&](int start,int count){for(int j=0;j<count;++j){float l=p.traceL[start+j],r=p.traceR[start+j];samples[head]=(l+r)*.5f;scope[head]={r-l,r+l};head=(head+1)%1024;}};consume(s1,n1);consume(s2,n2);p.fifo.finishedRead(n1+n2);
 for(int i=0;i<1024;++i)spectrum[i]=samples[(head+i)%1024]*(.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/1023));std::fill(spectrum.begin()+1024,spectrum.end(),0);fft.performFrequencyOnlyForwardTransform(spectrum.data());repaint();}
public:
 Editor(Processor& proc):AudioProcessorEditor(proc),p(proc),controls(proc){addAndMakeVisible(controls);setResizable(true,true);setResizeLimits(750,550,2000,1400);setSize(1000,700);startTimerHz(30);}
 bool isInterestedInDragSource(const SourceDetails& d)override{return d.description=="eq"||d.description=="limiter";}
 void itemDropped(const SourceDetails& d)override{bool right=d.localPosition.x>getWidth()/2;bool limiterFirst=d.description=="eq"?right:!right;auto* param=p.state.getParameter("order");param->beginChangeGesture();param->setValueNotifyingHost(limiterFirst?1.f:0.f);param->endChangeGesture();repaint();}
 void mouseDrag(const juce::MouseEvent& event)override{if(event.getMouseDownY()<85||event.getMouseDownY()>130||isDragAndDropActive())return;bool right=event.getMouseDownX()>getWidth()/2;bool first=p.state.getRawParameterValue("order")->load()>.5f;startDragging((right?!first:first)?"limiter":"eq",this);}
 void resized()override{controls.setBounds(20,getHeight()-270,getWidth()-40,250);}
 void paint(juce::Graphics& g)override{
 g.fillAll(juce::Colour(0xff10151e));g.setColour(juce::Colours::whitesmoke);g.setFont(24);g.drawText("JERZY MASTERLAB X / Phase A",20,12,getWidth()-40,40,juce::Justification::left);
 g.setFont(14);g.drawText("Output peak: "+juce::String(juce::Decibels::gainToDecibels(p.peak.load()),1)+" dBFS   RMS: "+juce::String(juce::Decibels::gainToDecibels(p.rms.load()),1)+" dBFS   Correlation: "+juce::String(p.correlation.load(),3),20,55,getWidth()-40,30,juce::Justification::left);
 bool first=p.state.getRawParameterValue("order")->load()>.5f;g.setColour(juce::Colour(0xff243241));g.fillRoundedRectangle(20,88,getWidth()*.5f-25,40,5);g.fillRoundedRectangle(getWidth()*.5f+5,88,getWidth()*.5f-25,40,5);g.setColour(juce::Colours::whitesmoke);g.drawText(first?"1  LIMITER   (drag)":"1  SPECTRAL EQ / 16 bands   (drag)",30,88,getWidth()/2-40,40,juce::Justification::left);g.drawText(first?"2  SPECTRAL EQ / 16 bands":"2  LIMITER",getWidth()/2+15,88,getWidth()/2-40,40,juce::Justification::left);
 const float w=getWidth()*.65f,h=static_cast<float>(getHeight()-435);juce::Path path;
 g.setFont(11);for(float hz:{20.f,100.f,1000.f,10000.f}){float pos=static_cast<float>(std::log(hz/20.f)/std::log(std::max(21.,p.getSampleRate()*.5)/20));if(pos>1)continue;float x=20+w*pos;g.setColour(juce::Colour(0xff28313a));g.drawVerticalLine(static_cast<int>(x),145,145+h);g.setColour(juce::Colours::grey);g.drawText(juce::String(hz>=1000?hz/1000:hz,0)+(hz>=1000?"k":""),static_cast<int>(x)+3,static_cast<int>(145+h)-20,45,18,juce::Justification::left);}
 for(int i=0;i<512;++i){float hz=20*std::pow(static_cast<float>(p.getSampleRate()*.5/20),i/511.f);int bin=juce::jlimit(1,511,static_cast<int>(hz*1024/std::max(1.,p.getSampleRate())));float db=juce::Decibels::gainToDecibels(spectrum[bin]/256.f,-100.f);float x=20+w*i/511.f,y=145+h*(1-juce::jlimit(0.f,1.f,(db+100)/100));if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
 g.setColour(juce::Colour(0xff49d9c0));g.strokePath(path,juce::PathStrokeType(1.5f));
 const float cx=getWidth()*.84f,cy=145+h*.5f,scale=std::min(w*.15f,h*.4f);g.setColour(juce::Colours::grey);g.drawEllipse(cx-scale,cy-scale,scale*2,scale*2,1);g.setColour(juce::Colours::orange.withAlpha(.4f));for(auto v:scope)g.fillEllipse(cx+v.x*scale-1,cy-v.y*scale-1,2,2);
 g.setColour(juce::Colours::lightgrey);g.drawText("Output FFT (20 Hz to Nyquist) / Vectorscope",20,static_cast<int>(145+h),getWidth()-40,25,juce::Justification::left);
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
