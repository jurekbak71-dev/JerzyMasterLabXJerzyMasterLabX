#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
#include "LimiterMeter.h"
#include "StateValidation.h"
class Processor final: public juce::AudioProcessor {
public:
 juce::AudioProcessorValueTreeState state;
 masterlab::RoutedEngine engine;
 masterlab::LimiterMeter limiterMeter;
 juce::AbstractFifo limiterFifo{2048};std::array<masterlab::LimiterFrame,2048> limiterFrames{};
 std::array<float,128> limiterGainTrace{};
 std::array<std::vector<float>,2> hostDryDelay;
 std::array<std::array<float,128>,2> hostDryScratch{};
 size_t hostDryPosition=0;float hostBypassMix=0,hostBypassStep=1.f/960.f;bool hostBypassTarget=false;
 std::array<std::atomic<float>*,86> parameters{};
 juce::AbstractFifo fifo{8192};std::array<float,8192> traceL{},traceR{};
 struct ResponseFrame {std::array<masterlab::Biquad,16> bands;std::array<float,16> blends{};double rate=48000;float eqMix=1;};
 juce::AbstractFifo responseFifo{4};std::array<ResponseFrame,4> responses{};
 std::atomic<double> displayRate{48000};
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
 p.add(std::make_unique<juce::AudioParameterFloat>("q"+id,title+"Q",.1f,10.f,1.f));}
 p.add(std::make_unique<juce::AudioParameterFloat>("limiterDrive","Limiter drive (dB)",0.f,24.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>("limiterRelease","Limiter release (ms)",juce::NormalisableRange<float>(10.f,1000.f,.1f,.4f),80.f));
 for(int band=1;band<=16;++band){auto suffix=juce::String(band);
 p.add(std::make_unique<juce::AudioParameterChoice>("bandType"+suffix,"Band "+suffix+" filter type",juce::StringArray{"Bell","Low Shelf","High Shelf","High Pass 12 dB/oct","Low Pass 12 dB/oct","Notch"},0));
 p.add(std::make_unique<juce::AudioParameterBool>("bandEnabled"+suffix,"Band "+suffix+" enabled",true));}return p;}
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"MasterLab",layout()){state.state.setProperty("schemaVersion",3,nullptr);state.state.setProperty("eqInstanceId","eq-0001",nullptr);state.state.setProperty("limiterInstanceId","limiter-0001",nullptr);const char* ids[]={"frequency","gain","q","ceiling","eq","lim","order"};for(size_t i=0;i<7;++i)parameters[i]=state.getRawParameterValue(ids[i]);for(int k=2;k<=16;++k){int offset=7+(k-2)*3;const auto suffix=juce::String(k);parameters[offset]=state.getRawParameterValue("frequency"+suffix);parameters[offset+1]=state.getRawParameterValue("gain"+suffix);parameters[offset+2]=state.getRawParameterValue("q"+suffix);}
 parameters[52]=state.getRawParameterValue("limiterDrive");parameters[53]=state.getRawParameterValue("limiterRelease");
 for(int k=1;k<=16;++k){parameters[54+(k-1)*2]=state.getRawParameterValue("bandType"+juce::String(k));parameters[55+(k-1)*2]=state.getRawParameterValue("bandEnabled"+juce::String(k));}}
 const juce::String getName()const override{return "JerzyMasterLabX";}
 bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return .1;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return "Default";}void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override{engine.parameters(parameters[0]->load(),parameters[1]->load(),parameters[2]->load(),parameters[3]->load(),parameters[4]->load()>.5f,parameters[5]->load()>.5f);engine.limiterParameters(parameters[52]->load(),parameters[53]->load());for(size_t k=0;k<16;++k)engine.setBandMode(k,static_cast<masterlab::FilterType>(static_cast<int>(parameters[54+k*2]->load())),parameters[55+k*2]->load()>.5f);engine.prepare(sr,parameters[6]->load()>.5f);displayRate.store(std::isfinite(sr)&&sr>=8000&&sr<=384000?sr:48000);limiterMeter.prepare(displayRate.load());setLatencySamples(engine.latency());for(auto& channel:hostDryDelay)channel.assign(static_cast<size_t>(engine.latency()+1),0);hostDryPosition=0;hostBypassMix=0;hostBypassTarget=false;hostBypassStep=static_cast<float>(1/(std::isfinite(sr)&&sr>=8000&&sr<=384000?sr*.02:960.));}
 void releaseResources()override{}void reset()override{engine.reset();limiterMeter.prepare(displayRate.load());for(auto& channel:hostDryDelay)std::fill(channel.begin(),channel.end(),0);hostDryPosition=0;hostBypassMix=hostBypassTarget?1.f:0.f;}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{renderAudio(b,false);}
 void processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{renderAudio(b,true);}
 void renderAudio(juce::AudioBuffer<float>& b,bool hostBypassed){
 juce::ScopedNoDenormals noDenormals;if(b.getNumChannels()<2)return;
 auto value=[&](size_t index){return parameters[index]->load();};
 engine.limiterParameters(value(52),value(53));
 engine.parameters(value(0),value(1),value(2),value(3),value(4)>.5f,value(5)>.5f);
 for(size_t k=1;k<16;++k){size_t offset=7+(k-1)*3;engine.setBand(k,value(offset),value(offset+1),value(offset+2));}
 for(size_t k=0;k<16;++k)engine.setBandMode(k,static_cast<masterlab::FilterType>(static_cast<int>(value(54+k*2))),value(55+k*2)>.5f);
 hostBypassTarget=hostBypassed;
 if(hostDryDelay[0].empty()){b.clear();return;}
 for(int offset=0;offset<b.getNumSamples();){int length=std::min(128,b.getNumSamples()-offset);
  for(int i=0;i<length;++i){size_t read=(hostDryPosition+1)%hostDryDelay[0].size();for(int c=0;c<2;++c){hostDryDelay[c][hostDryPosition]=masterlab::clean(b.getSample(c,offset+i));hostDryScratch[c][i]=hostDryDelay[c][read];}hostDryPosition=read;}
  engine.process(b.getWritePointer(0,offset),b.getWritePointer(1,offset),length,value(6)>.5f,limiterGainTrace.data());
  for(int i=0;i<length;++i){hostBypassMix=hostBypassed?std::min(1.f,hostBypassMix+hostBypassStep):std::max(0.f,hostBypassMix-hostBypassStep);limiterGainTrace[i]+=hostBypassMix*(1-limiterGainTrace[i]);for(int c=0;c<2;++c){float wet=b.getSample(c,offset+i);b.setSample(c,offset+i,wet+hostBypassMix*(hostDryScratch[c][i]-wet));}}
  for(int i=0;i<length;++i){
   masterlab::LimiterFrame frame;
   // The reported envelope includes module and host bypass and route crossfades.
   if(limiterMeter.push(limiterGainTrace[i],b.getSample(0,offset+i),b.getSample(1,offset+i),frame)){
    int start,n,start2,n2;limiterFifo.prepareToWrite(1,start,n,start2,n2);
    if(n+n2)limiterFrames[n?start:start2]=frame;
    limiterFifo.finishedWrite(n+n2);
   }
  }
  offset+=length;
 }
 int responseStart,responseCount,responseStart2,responseCount2;
 responseFifo.prepareToWrite(1,responseStart,responseCount,responseStart2,responseCount2);
 if(responseCount+responseCount2){int index=responseCount?responseStart:responseStart2;engine.copyEqCoefficients(responses[index].bands);engine.copyBandBlends(responses[index].blends);responses[index].rate=displayRate.load();responses[index].eqMix=engine.eqBlend();}
 responseFifo.finishedWrite(responseCount+responseCount2);
 double ll=0,rr=0,lr=0;float pk=0;
 for(int i=0;i<b.getNumSamples();++i){float l=b.getSample(0,i),r=b.getSample(1,i);ll+=l*l;rr+=r*r;lr+=l*r;pk=std::max(pk,std::max(std::abs(l),std::abs(r)));}
 peak.store(pk);rms.store(static_cast<float>(std::sqrt((ll+rr)/std::max(1,2*b.getNumSamples()))));correlation.store(static_cast<float>(lr/std::sqrt(std::max(1e-20,ll*rr))));
 int s1,n1,s2,n2;fifo.prepareToWrite(b.getNumSamples(),s1,n1,s2,n2);
 for(int j=0;j<n1;++j){traceL[s1+j]=b.getSample(0,j);traceR[s1+j]=b.getSample(1,j);}for(int j=0;j<n2;++j){traceL[s2+j]=b.getSample(0,n1+j);traceR[s2+j]=b.getSample(1,n1+j);}fifo.finishedWrite(n1+n2);
 }
 void getStateInformation(juce::MemoryBlock& dest)override{auto xml=state.copyState().createXml();copyXmlToBinary(*xml,dest);}
 void setStateInformation(const void* data,int size)override{
  if(data==nullptr||size<=0||size>1024*1024)return;
  auto xml=getXmlFromBinary(data,size);if(!xml)return;
  auto validated=masterlab::validateState(juce::ValueTree::fromXml(*xml),state,getParameters());
  if(validated.isValid())state.replaceState(validated);
 }
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
};
#include "Editor.h"
juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
