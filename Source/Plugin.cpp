#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
class Processor final: public juce::AudioProcessor {
public:
 juce::AudioProcessorValueTreeState state;
 masterlab::Engine engine;
 std::array<std::atomic<float>*,7> parameters{};
 juce::AbstractFifo fifo{8192};std::array<float,8192> traceL{},traceR{};
 std::atomic<float> peak{0},rms{0},correlation{0};
 static auto layout(){juce::AudioProcessorValueTreeState::ParameterLayout p;
 p.add(std::make_unique<juce::AudioParameterFloat>("frequency","EQ frequency",juce::NormalisableRange<float>(20,20000,0,.3f),1000));
 p.add(std::make_unique<juce::AudioParameterFloat>("gain","EQ gain",-18,18,0));
 p.add(std::make_unique<juce::AudioParameterFloat>("q","EQ Q",.1f,10.f,1.f));
 p.add(std::make_unique<juce::AudioParameterFloat>("ceiling","Sample peak ceiling (dBFS)",-24,0,-1));
 p.add(std::make_unique<juce::AudioParameterBool>("eq","EQ enabled",true));
 p.add(std::make_unique<juce::AudioParameterBool>("lim","Limiter enabled",true));
 p.add(std::make_unique<juce::AudioParameterBool>("order","Limiter before EQ (ceiling applies before EQ)",false));return p;}
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"MasterLab",layout()){const char* ids[]={"frequency","gain","q","ceiling","eq","lim","order"};for(size_t i=0;i<parameters.size();++i)parameters[i]=state.getRawParameterValue(ids[i]);}
 const juce::String getName()const override{return "JerzyMasterLabX";}
 bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return .1;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return "Default";}void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override{engine.prepare(sr);setLatencySamples(engine.latency());}
 void releaseResources()override{}void reset()override{engine.reset();}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{
 juce::ScopedNoDenormals noDenormals;if(b.getNumChannels()<2)return;
 auto value=[&](size_t index){return parameters[index]->load();};
 engine.parameters(value(0),value(1),value(2),value(3),value(4)>.5f,value(5)>.5f);
 engine.process(b.getWritePointer(0),b.getWritePointer(1),b.getNumSamples(),value(6)>.5f);
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
class Editor final:public juce::AudioProcessorEditor,private juce::Timer {
 Processor& p;juce::GenericAudioProcessorEditor controls;
 juce::dsp::FFT fft{10};std::array<float,2048> spectrum{};std::array<float,1024> samples{};int head=0;
 std::array<juce::Point<float>,1024> scope{};
 void timerCallback()override{int s1,n1,s2,n2;p.fifo.prepareToRead(p.fifo.getNumReady(),s1,n1,s2,n2);
 auto consume=[&](int start,int count){for(int j=0;j<count;++j){float l=p.traceL[start+j],r=p.traceR[start+j];samples[head]=(l+r)*.5f;scope[head]={r-l,r+l};head=(head+1)%1024;}};consume(s1,n1);consume(s2,n2);p.fifo.finishedRead(n1+n2);
 for(int i=0;i<1024;++i)spectrum[i]=samples[(head+i)%1024]*(.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/1023));std::fill(spectrum.begin()+1024,spectrum.end(),0);fft.performFrequencyOnlyForwardTransform(spectrum.data());repaint();}
public:
 Editor(Processor& proc):AudioProcessorEditor(proc),p(proc),controls(proc){addAndMakeVisible(controls);setResizable(true,true);setResizeLimits(750,550,2000,1400);setSize(1000,700);startTimerHz(30);}
 void resized()override{controls.setBounds(20,getHeight()-270,getWidth()-40,250);}
 void paint(juce::Graphics& g)override{
 g.fillAll(juce::Colour(0xff10151e));g.setColour(juce::Colours::whitesmoke);g.setFont(24);g.drawText("JERZY MASTERLAB X / Phase A",20,12,getWidth()-40,40,juce::Justification::left);
 g.setFont(14);g.drawText("Output peak: "+juce::String(juce::Decibels::gainToDecibels(p.peak.load()),1)+" dBFS   RMS: "+juce::String(juce::Decibels::gainToDecibels(p.rms.load()),1)+" dBFS   Correlation: "+juce::String(p.correlation.load(),3),20,55,getWidth()-40,30,juce::Justification::left);
 const float w=getWidth()*.65f,h=static_cast<float>(getHeight()-390);juce::Path path;
 for(int i=0;i<512;++i){float hz=20*std::pow(static_cast<float>(p.getSampleRate()*.5/20),i/511.f);int bin=juce::jlimit(1,511,static_cast<int>(hz*1024/std::max(1.,p.getSampleRate())));float db=juce::Decibels::gainToDecibels(spectrum[bin]/256.f,-100.f);float x=20+w*i/511.f,y=100+h*(1-juce::jlimit(0.f,1.f,(db+100)/100));if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
 g.setColour(juce::Colour(0xff49d9c0));g.strokePath(path,juce::PathStrokeType(1.5f));
 const float cx=getWidth()*.84f,cy=100+h*.5f,scale=std::min(w*.15f,h*.4f);g.setColour(juce::Colours::grey);g.drawEllipse(cx-scale,cy-scale,scale*2,scale*2,1);g.setColour(juce::Colours::orange.withAlpha(.4f));for(auto v:scope)g.fillEllipse(cx+v.x*scale-1,cy-v.y*scale-1,2,2);
 g.setColour(juce::Colours::lightgrey);g.drawText("Output FFT (20 Hz–Nyquist) / Vectorscope",20,static_cast<int>(100+h),getWidth()-40,25,juce::Justification::left);
 }
};
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
