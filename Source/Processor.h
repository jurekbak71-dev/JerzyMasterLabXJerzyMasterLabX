#include "GraphConfiguration.h"
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
#include "SerialGraph.h"
#include "ModuleParameters.h"
#include "LimiterMeter.h"
#include "WindowMeter.h"
#include "LoudnessMeter.h"
#include "MasterBrain.h"
#include "StemRemix.h"
#include "StateValidation.h"
class Processor final: public juce::AudioProcessor {
public:
 juce::UndoManager undoManager{1000000,100};
 juce::AudioProcessorValueTreeState state;
 masterlab::MasterBrain brain;
 masterlab::StemRemix stemRemix;
 std::vector<float> brainBaseline,brainAuditionValues;
 int brainAuditionVariant=-1;
 juce::String brainActionMessage{"No Brain proposal applied yet."};
 masterlab::SerialGraph engine;
 masterlab::LimiterMeter limiterMeter;
 masterlab::WindowMeter inputMeter,outputMeter;
 masterlab::LoudnessMeter inputLoudness,outputLoudness;
 std::atomic<bool> resetMeterRequest{false};
 std::array<std::array<float,128>,2> inputScratch{};
 juce::AbstractFifo limiterFifo{2048};std::array<masterlab::LimiterFrame,2048> limiterFrames{};
 std::array<float,128> limiterGainTrace{};
 std::array<std::vector<float>,2> hostDryDelay;
 std::array<std::array<float,128>,2> hostDryScratch{};
 size_t hostDryPosition=0;float hostBypassMix=0,hostBypassStep=1.f/960.f;bool hostBypassTarget=false;
 std::array<std::atomic<float>*,masterlab::parameterCount> parameters{};
 juce::AbstractFifo fifo{8192};std::array<float,8192> traceL{},traceR{};
 struct ResponseFrame {std::array<masterlab::Biquad,16> bands;std::array<float,16> blends{};double rate=48000;std::array<float,12> stabilizerGain{};std::array<masterlab::Biquad,12> stabilizerBands{};float stabilizerMix=0;float eqMix=1;masterlab::StereoReading stereo;masterlab::DynamicsReading dynamics;masterlab::ParallelReading parallel;masterlab::HarmonicExciterReading exciter;masterlab::AnalogColorReading analog;masterlab::BassArchitectReading bass;masterlab::TransientSculptorReading transient;masterlab::FinalTruePeakReading truePeak;std::array<float,3> dynamicGain{},dynamicBlends{};std::array<masterlab::Biquad,3> dynamicBands{};masterlab::MeterReading inputReading,outputReading;masterlab::LoudnessReading inputLufs,outputLufs;};
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
 p.add(std::make_unique<juce::AudioParameterBool>("bandEnabled"+suffix,"Band "+suffix+" enabled",true));}
 addModuleParameters(p);addStemParameters(p);return p;}
 static void addStemParameters(juce::AudioProcessorValueTreeState::ParameterLayout& p){
  p.add(std::make_unique<juce::AudioParameterChoice>("stemMode","Stem Brain source",
      juce::StringArray{"Original DAW mix","Separated stems only","DAW mix plus stems"},0));
  constexpr std::array<const char*,4> names{"Drums","Bass","Other","Vocals"};
  for(const auto* name:names){const auto n=juce::String(name);const auto stem="stem"+n;
   p.add(std::make_unique<juce::AudioParameterFloat>(stem+"Gain",n+" gain dB",-24.f,12.f,0.f));
   p.add(std::make_unique<juce::AudioParameterFloat>(stem+"Tone",n+" tonal tilt",-1.f,1.f,0.f));
   p.add(std::make_unique<juce::AudioParameterFloat>(stem+"Width",n+" stereo width",0.f,2.f,1.f));
   p.add(std::make_unique<juce::AudioParameterBool>(stem+"Mute",n+" mute",false));
   p.add(std::make_unique<juce::AudioParameterBool>(stem+"Solo",n+" solo",false));
  }
 }
 static void addModuleParameters(juce::AudioProcessorValueTreeState::ParameterLayout& p){
  auto boolean=[&](const char* id,const char* name,bool initial){p.add(std::make_unique<juce::AudioParameterBool>(id,name,initial));};
  auto number=[&](const char* id,const char* name,float lo,float hi,float initial){p.add(std::make_unique<juce::AudioParameterFloat>(id,name,lo,hi,initial));};
  boolean("eqPresent","EQ in chain",true);boolean("limPresent","Limiter in chain",true);
  boolean("stereoPresent","Stereo in chain",false);boolean("stereoOn","Stereo enabled",true);
  number("stereoLowWidth","Stereo low width",0,2,1);number("stereoMidWidth","Stereo mid width",0,2,1);number("stereoHighWidth","Stereo high width",0,2,1);
  number("stereoLowSplit","Stereo low crossover",40,500,140);number("stereoHighSplit","Stereo mid crossover",500,4000,2500);
  boolean("stereoMonoBass","Stereo mono bass",false);boolean("stereoMono","Stereo mono preview",false);number("stereoBalance","Stereo balance",-1,1,0);
  boolean("dynamicsPresent","Compressor in chain",false);boolean("dynamicsOn","Compressor enabled",true);
  p.add(std::make_unique<juce::AudioParameterChoice>("dynamicsMode","Compressor character",juce::StringArray{"Digital","Analog"},0));
  number("dynThreshold","Compressor threshold",-60,0,-18);number("dynRatio","Compressor ratio",1,12,2);number("dynAttack","Compressor attack",.1f,100,15);number("dynRelease","Compressor release",10,1000,150);number("dynKnee","Compressor knee",0,18,6);number("dynMakeup","Compressor makeup",-12,12,0);number("dynMix","Compressor dry wet",0,1,1);
  number("upwardAmount","Upward maximum gain",0,18,0);number("upwardThreshold","Upward threshold",-60,-6,-36);
  boolean("dynamicEqPresent","Dynamic EQ in chain",false);boolean("dynamicEqOn","Dynamic EQ enabled",true);
  for(int i=0;i<3;++i){auto suffix=juce::String(i+1);auto title="Dynamic EQ "+suffix+" ";
   p.add(std::make_unique<juce::AudioParameterFloat>("deqFreq"+suffix,title+"frequency",juce::NormalisableRange<float>(20,20000,1,.3f),std::array<float,3>{150,2500,7500}[i]));
   p.add(std::make_unique<juce::AudioParameterFloat>("deqGain"+suffix,title+"static gain",-12,12,0));p.add(std::make_unique<juce::AudioParameterFloat>("deqQ"+suffix,title+"Q",.2f,6,1));
   p.add(std::make_unique<juce::AudioParameterFloat>("deqThreshold"+suffix,title+"threshold",-60,0,i==2?-30.f:-24.f));p.add(std::make_unique<juce::AudioParameterFloat>("deqRange"+suffix,title+"dynamic range",-12,12,-3));
   p.add(std::make_unique<juce::AudioParameterFloat>("deqAttack"+suffix,title+"attack",.1f,100,std::array<float,3>{25,10,3}[i]));p.add(std::make_unique<juce::AudioParameterFloat>("deqRelease"+suffix,title+"release",10,1000,std::array<float,3>{180,120,80}[i]));p.add(std::make_unique<juce::AudioParameterBool>("deqBandOn"+suffix,title+"enabled",true));
  }
  number("stereoAirWidth","Stereo air width",0,2,1);number("stereoTopSplit","Stereo top crossover",5000,16000,8000);
  p.add(std::make_unique<juce::AudioParameterInt>("chainOrder","Module chain order",0,719,0));
  for(int i=1;i<=4;++i){auto suffix=juce::String(i);p.add(std::make_unique<juce::AudioParameterBool>("stereoBandOn"+suffix,"Stereo band "+suffix+" enabled",true));p.add(std::make_unique<juce::AudioParameterBool>("stereoSolo"+suffix,"Stereo band "+suffix+" solo",false));}
  number("stereoAmount","Stereo amount",0,1,1);boolean("stereoDelta","Stereo delta monitor",false);
  boolean("stabilizerPresent","Stabilizer in chain",false);boolean("stabilizerOn","Stabilizer enabled",true);p.add(std::make_unique<juce::AudioParameterChoice>("stabilizerMode","Stabilizer mode",juce::StringArray{"Shape","Cut"},0));p.add(std::make_unique<juce::AudioParameterChoice>("stabilizerProfile","Stabilizer profile",juce::StringArray{"Neutral","Electronic","Acoustic"},0));number("stabilizerAmount","Stabilizer amount",0,100,25);number("stabilizerSpeed","Stabilizer response time",0,100,50);number("stabilizerSmoothing","Stabilizer spectral smoothing",0,100,50);number("stabilizerLow","Stabilizer low amount",0,100,100);number("stabilizerMid","Stabilizer mid amount",0,100,100);number("stabilizerHigh","Stabilizer high amount",0,100,100);boolean("stabilizerTame","Stabilizer tame transients",true);boolean("stabilizerDelta","Stabilizer delta monitor",false);
  p.add(std::make_unique<juce::AudioParameterChoice>("routingMode","Module routing",juce::StringArray{"Serial","Parallel"},0));
  number("stereoizeAmount","Stereoize (bass protected)",0,1,0);
  boolean("parallelCompensated","Parallel normalize branch weights",false);
  constexpr std::array<const char*,6> trimIds{"parallelTrimDynamicEq","parallelTrimDynamics","parallelTrimStereo","parallelTrimEq","parallelTrimLimiter","parallelTrimStabilizer"};
  constexpr std::array<const char*,6> mixIds{"parallelMixDynamicEq","parallelMixDynamics","parallelMixStereo","parallelMixEq","parallelMixLimiter","parallelMixStabilizer"};
  constexpr std::array<const char*,6> names{"Dynamic EQ","Dynamics","Stereo","Spectral EQ","Limiter","Stabilizer"};
  for(size_t i=0;i<names.size();++i){const auto trimName=juce::String(names[i])+" parallel trim";const auto mixName=juce::String(names[i])+" parallel wet";p.add(std::make_unique<juce::AudioParameterFloat>(trimIds[i],trimName,-24,12,0));p.add(std::make_unique<juce::AudioParameterFloat>(mixIds[i],mixName,0,1,1));}
  p.add(std::make_unique<juce::AudioParameterChoice>("dynamicsIntensity","Dynamics intensity",juce::StringArray{"Legacy","Clean","Strong","Extreme"},0));number("dynPunch","Dynamics transient punch",0,100,0);boolean("dynDelta","Dynamics delta monitor",false);
  p.add(std::make_unique<juce::AudioParameterChoice>("stabilizerIntensity","Stabilizer intensity",juce::StringArray{"Legacy","Clean","Strong","Extreme"},0));
  boolean("exciterPresent","Harmonic Exciter post chain",false);boolean("exciterOn","Harmonic Exciter active",true);
  for(const auto* id:{"exciterAmountLow","exciterAmountMid","exciterAmountHigh","exciterAmountAir"})number(id,id,0,1,0);
  number("exciterEven","Harmonic Exciter even",0,1,.5f);number("exciterOdd","Harmonic Exciter odd",0,1,.5f);
  number("exciterDrive","Harmonic Exciter drive dB",0,24,0);number("exciterMix","Harmonic Exciter wet",0,1,1);number("exciterOutput","Harmonic Exciter output dB",-18,6,0);
  boolean("analogPresent","Analog Color post chain",false);boolean("analogOn","Analog Color enabled",true);
  p.add(std::make_unique<juce::AudioParameterChoice>("analogMode","Analog Color model",juce::StringArray{"Tube","Tape","Transformer","Solid State"},0));
  number("analogDrive","Analog Color drive dB",0,30,0);number("analogBias","Analog Color bias",-1,1,0);number("analogTone","Analog Color tone",-1,1,0);
  number("analogDynamics","Analog Color compression",0,1,0);number("analogMix","Analog Color wet",0,1,1);number("analogOutput","Analog Color output dB",-18,12,0);boolean("analogAutoGain","Analog Color gain compensation",true);
  number("exciterImpact","Harmonic Exciter impact",0,1,0);number("analogImpact","Analog Color impact",0,1,0);
  boolean("bassPresent","Bass Architect post chain",false);boolean("bassOn","Bass Architect enabled",true);
  number("bassSub","Bass Architect sub level",-1,1,0);number("bassBody","Bass Architect body level",-1,1,0);
  number("bassHarmonics","Bass Architect harmonics",0,1,0);number("bassMono","Bass Architect mono sub",0,1,0);
  number("bassMix","Bass Architect mix",0,1,1);number("bassOutput","Bass Architect output dB",-12,12,0);
  boolean("transientPresent","Transient Sculptor post chain",false);boolean("transientOn","Transient Sculptor active",true);
  for(const char* id:{"transientAttackLow","transientAttackMid","transientAttackHigh","transientSustainLow","transientSustainMid","transientSustainHigh"})number(id,id,-1,1,0);
  number("transientMix","Transient Sculptor mix",0,1,1);number("transientOutput","Transient Sculptor output",-12,12,0);
  boolean("truePeakPresent","Final True-Peak post-chain",false);boolean("truePeakOn","Final True-Peak enabled",true);
  number("truePeakCeiling","Final True-Peak ceiling dB",-12,-.1f,-1);
  number("truePeakRelease","Final True-Peak release ms",10,1000,120);
  number("truePeakDrive","Final True-Peak drive dB",0,18,0);
 }
 void updateModules(){
  masterlab::configureModules(engine,[&](size_t i){return parameters[i]->load();});
 }
 Processor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,&undoManager,"MasterLab",layout()){state.state.setProperty("schemaVersion",17,nullptr);state.state.setProperty("eqInstanceId","eq-0001",nullptr);state.state.setProperty("limiterInstanceId","limiter-0001",nullptr);state.state.setProperty("dynamicEqInstanceId","dynamic-eq-0001",nullptr);state.state.setProperty("dynamicsInstanceId","dynamics-0001",nullptr);state.state.setProperty("stereoInstanceId","stereo-0001",nullptr);state.state.setProperty("stabilizerInstanceId","stabilizer-0001",nullptr);const char* ids[]={"frequency","gain","q","ceiling","eq","lim","order"};for(size_t i=0;i<7;++i)parameters[i]=state.getRawParameterValue(ids[i]);for(int k=2;k<=16;++k){int offset=7+(k-2)*3;const auto suffix=juce::String(k);parameters[offset]=state.getRawParameterValue("frequency"+suffix);parameters[offset+1]=state.getRawParameterValue("gain"+suffix);parameters[offset+2]=state.getRawParameterValue("q"+suffix);}
 parameters[52]=state.getRawParameterValue("limiterDrive");parameters[53]=state.getRawParameterValue("limiterRelease");
 for(int k=1;k<=16;++k){parameters[54+(k-1)*2]=state.getRawParameterValue("bandType"+juce::String(k));parameters[55+(k-1)*2]=state.getRawParameterValue("bandEnabled"+juce::String(k));}
 for(size_t i=0;i<masterlab::moduleParameterIds.size();++i)parameters[86+i]=state.getRawParameterValue(masterlab::moduleParameterIds[i]);
 for(size_t i=0;i<masterlab::stemParameterIds.size();++i)parameters[226+i]=state.getRawParameterValue(masterlab::stemParameterIds[i]);}
 const juce::String getName()const override{return "JerzyMasterLabX";}
 bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return .1;}
 int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return "Default";}void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override{masterlab::configureGraph(engine,[&](size_t i){return parameters[i]->load();});engine.prepare(sr,parameters[6]->load()>.5f,static_cast<int>(parameters[138]->load()));displayRate.store(std::isfinite(sr)&&sr>=8000&&sr<=384000?sr:48000);limiterMeter.prepare(displayRate.load());inputMeter.prepare(displayRate.load());outputMeter.prepare(displayRate.load());inputLoudness.prepare(displayRate.load());outputLoudness.prepare(displayRate.load());brain.prepare(displayRate.load());stemRemix.reset();updateModules();setLatencySamples(engine.latency());for(auto& channel:hostDryDelay)channel.assign(static_cast<size_t>(engine.latency()+1),0);hostDryPosition=0;hostBypassMix=0;hostBypassTarget=false;hostBypassStep=static_cast<float>(1/(std::isfinite(sr)&&sr>=8000&&sr<=384000?sr*.02:960.));}
 void releaseResources()override{}void reset()override{brain.cancelRT();stemRemix.reset();engine.reset();updateModules();inputMeter.reset();outputMeter.reset();inputLoudness.reset();outputLoudness.reset();limiterMeter.prepare(displayRate.load());for(auto& channel:hostDryDelay)std::fill(channel.begin(),channel.end(),0);hostDryPosition=0;hostBypassMix=hostBypassTarget?1.f:0.f;}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{renderAudio(b,false);}
 void processBlockBypassed(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{renderAudio(b,true);}
 void renderAudio(juce::AudioBuffer<float>& b,bool hostBypassed){
 juce::ScopedNoDenormals noDenormals;if(b.getNumChannels()<2)return;
 if(resetMeterRequest.exchange(false)){inputMeter.reset();outputMeter.reset();inputLoudness.reset();outputLoudness.reset();}
 auto value=[&](size_t index){return parameters[index]->load();};
 masterlab::configureGraph(engine,value);
 hostBypassTarget=hostBypassed;
 {std::array<float,21> control{};
  for(size_t n=0;n<control.size();++n)control[n]=parameters[226+n]->load();
  auto sample=stemRemix.fallbackPosition();bool running=true;
  if(auto* playhead=getPlayHead())if(auto pos=playhead->getPosition()){
   if(auto timeline=pos->getTimeInSamples())sample=*timeline;
   running=pos->getIsPlaying();
  }
  stemRemix.render(b,displayRate.load(),sample,running,control);
 }
 if(hostDryDelay[0].empty()){b.clear();return;}
 for(int offset=0;offset<b.getNumSamples();){int length=std::min(128,b.getNumSamples()-offset);
  for(int i=0;i<length;++i){size_t read=(hostDryPosition+1)%hostDryDelay[0].size();for(int c=0;c<2;++c){inputScratch[c][i]=masterlab::clean(b.getSample(c,offset+i));hostDryDelay[c][hostDryPosition]=inputScratch[c][i];hostDryScratch[c][i]=hostDryDelay[c][read];}hostDryPosition=read;}
  brain.capture(inputScratch[0].data(),inputScratch[1].data(),length);
  engine.process(b.getWritePointer(0,offset),b.getWritePointer(1,offset),length,value(6)>.5f,limiterGainTrace.data());
  for(int i=0;i<length;++i){hostBypassMix=hostBypassed?std::min(1.f,hostBypassMix+hostBypassStep):std::max(0.f,hostBypassMix-hostBypassStep);limiterGainTrace[i]+=hostBypassMix*(1-limiterGainTrace[i]);for(int c=0;c<2;++c){float wet=b.getSample(c,offset+i);b.setSample(c,offset+i,wet+hostBypassMix*(hostDryScratch[c][i]-wet));}}
  for(int i=0;i<length;++i){
   inputMeter.push(inputScratch[0][i],inputScratch[1][i]);outputMeter.push(b.getSample(0,offset+i),b.getSample(1,offset+i));inputLoudness.push(inputScratch[0][i],inputScratch[1][i]);outputLoudness.push(b.getSample(0,offset+i),b.getSample(1,offset+i));
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
 if(responseCount+responseCount2){int index=responseCount?responseStart:responseStart2;engine.copyEqCoefficients(responses[index].bands);engine.copyBandBlends(responses[index].blends);responses[index].stereo=engine.stereoRead();responses[index].dynamics=engine.dynamicsRead();responses[index].parallel=engine.parallelRead();responses[index].exciter=engine.harmonicExciterRead();responses[index].analog=engine.analogColorRead();responses[index].bass=engine.bassArchitectRead();responses[index].transient=engine.transientSculptorRead();responses[index].truePeak=engine.finalTruePeakRead();responses[index].dynamicGain=engine.dynamicRead();responses[index].dynamicBlends=engine.dynamicBlends();engine.dynamicCoefficients(responses[index].dynamicBands);responses[index].stabilizerGain=engine.stabilizerRead();engine.stabilizerCoefficients(responses[index].stabilizerBands);responses[index].stabilizerMix=engine.stabilizerBlend();responses[index].rate=displayRate.load();responses[index].eqMix=engine.eqBlend();responses[index].inputReading=inputMeter.read();responses[index].outputReading=outputMeter.read();responses[index].inputLufs=inputLoudness.read();responses[index].outputLufs=outputLoudness.read();}
 responseFifo.finishedWrite(responseCount+responseCount2);
 const auto reading=outputMeter.read();peak.store(reading.peak);rms.store(reading.rms);correlation.store(reading.correlation);
 int s1,n1,s2,n2;fifo.prepareToWrite(b.getNumSamples(),s1,n1,s2,n2);
 for(int j=0;j<n1;++j){traceL[s1+j]=b.getSample(0,j);traceR[s1+j]=b.getSample(1,j);}for(int j=0;j<n2;++j){traceL[s2+j]=b.getSample(0,n1+j);traceR[s2+j]=b.getSample(1,n1+j);}fifo.finishedWrite(n1+n2);
 }
 juce::String loadStemsUI(const juce::File& folder){
  auto result=stemRemix.loadFolder(folder);
  if(result.isEmpty()){
   state.state.setProperty("stemFolder",folder.getFullPathName(),&undoManager);
   auto* param=state.getParameter("stemMode");
   beginUiTransaction("Stem Brain import");
   param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(1.f));
   param->endChangeGesture();finishUiTransaction();
  }
  return result;
 }
 void clearStemsUI(){
  stemRemix.unload();
  auto* p=state.getParameter("stemMode");
  p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(0.f));p->endChangeGesture();
  state.state.removeProperty("stemFolder",&undoManager);
 }
 bool applyStemVariantUI(int variant){
  auto bank=stemRemix.snapshot();if(!bank||variant<0||variant>1)return false;
  // Conservative source-relative rebalance: avoid amplifying artifacts.
  // B has +1.5 dB lead-vocal prominence and -1 dB drums, not fake source AI.
  const std::array<float,4> offsets=variant==0?
     std::array<float,4>{0.f,0.f,0.f,0.f}:
     std::array<float,4>{-1.f,0.f,-.5f,1.5f};
  beginUiTransaction(variant==0?"Stem Brain natural balance":"Stem Brain vocal-forward");
  for(int i=0;i<4;++i){
   const auto id=juce::String(masterlab::stemParameterIds[1+5*i]);
   auto* param=state.getParameter(id);
   const float amount=std::clamp(offsets[static_cast<size_t>(i)],-6.f,6.f);
   param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(amount));
   param->endChangeGesture();
  }
  finishUiTransaction();return true;
 }
 void getStateInformation(juce::MemoryBlock& dest)override{auto xml=state.copyState().createXml();copyXmlToBinary(*xml,dest);}
 juce::ValueTree parseState(const void* data,int size){
  if(data==nullptr||size<=0||size>1024*1024)return {};
  auto xml=getXmlFromBinary(data,size);if(!xml)return {};
  return masterlab::validateState(juce::ValueTree::fromXml(*xml),state,getParameters());
 }
 void setStateInformation(const void* data,int size)override{
  auto validated=parseState(data,size);if(validated.isValid())state.replaceState(validated);
 }
 // UI/control thread only. Snapshot flushes pending parameter values before transaction boundaries.
 void beginUiTransaction(const juce::String& name){state.copyState();undoManager.beginNewTransaction(name);}
 void finishUiTransaction(){state.copyState();undoManager.beginNewTransaction();}
 bool undo(){state.copyState();bool changed=undoManager.undo();undoManager.beginNewTransaction();return changed;}
 bool redo(){state.copyState();bool changed=undoManager.redo();undoManager.beginNewTransaction();return changed;}
 // Factory preset 0: reset every real host parameter to its declared JUCE
 // default. Preserve host state schema and saved .jmlx files. One Undo action.
 bool loadFactoryDefaultUI(){
  const auto hostParameters=getParameters();
  bool changed=false;
  for(auto* p:hostParameters)
   changed|=std::abs(p->getValue()-p->getDefaultValue())>1.0e-6f;
  brain.clearUI();
  brainBaseline.clear();brainAuditionValues.clear();brainAuditionVariant=-1;
  brainActionMessage="DEFAULT 0: factory settings restored.";
  if(!changed)return true;
  beginUiTransaction("NEW / DEFAULT 0");
  for(auto* p:hostParameters){
   if(std::abs(p->getValue()-p->getDefaultValue())<=1.0e-6f)continue;
   p->beginChangeGesture();
   p->setValueNotifyingHost(p->getDefaultValue());
   p->endChangeGesture();
  }
  finishUiTransaction();
  resetMeterRequest.store(true);
  return true;
 }
 bool applyPreset(const void* data,int size){
  auto validated=parseState(data,size);if(!validated.isValid())return false;
  beginUiTransaction("Load preset");
  for(auto child:validated){auto* parameter=state.getParameter(child["id"].toString());parameter->beginChangeGesture();parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(child["value"])));parameter->endChangeGesture();}
  finishUiTransaction();return true;
 }
 bool savePreset(const juce::File& file){juce::MemoryBlock data;getStateInformation(data);return file.replaceWithData(data.getData(),data.getSize());}
 bool loadPreset(const juce::File& file){
  juce::FileInputStream input(file);if(!input.openedOk()||input.getTotalLength()>1024*1024)return false;
  juce::MemoryBlock data;input.readIntoMemoryBlock(data,1024*1024+1);return applyPreset(data.getData(),static_cast<int>(data.getSize()));
 }
 void setModulePresentUI(int kind,bool included){
  if(kind<0||kind>5)return;
  const char* presence=std::array<const char*,6>{"dynamicEqPresent","dynamicsPresent","stereoPresent","eqPresent","limPresent","stabilizerPresent"}[kind];
  const char* enabled=std::array<const char*,6>{"dynamicEqOn","dynamicsOn","stereoOn","eq","lim","stabilizerOn"}[kind];
  beginUiTransaction(included?"Add and activate module":"Remove module");
  auto set=[&](const juce::String& id,float value){auto* p=state.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();};
  set(presence,included?1.f:0.f);if(included){set(enabled,1);
   if(kind==3){bool any=false;for(int i=1;i<=16;++i)any|=state.getRawParameterValue("bandEnabled"+juce::String(i))->load()>.5f;if(!any)set("bandEnabled1",1);}
   if(kind==0){bool any=false;for(int i=1;i<=3;++i)any|=state.getRawParameterValue("deqBandOn"+juce::String(i))->load()>.5f;if(!any)set("deqBandOn1",1);}
  }finishUiTransaction();
 }
 void startBrainUI(masterlab::BrainConfig config){
  brainAuditionVariant=-1;brainAuditionValues.clear();
  brainActionMessage="Analysis started; parameters unchanged.";
  brainBaseline.clear();for(auto* parameter:parameters)brainBaseline.push_back(parameter->load());
  config.hasBaseline=true;for(size_t i=0;i<parameters.size();++i)config.baseline[i]=parameters[i]->load();
  config.existingEqEnabled=parameters[4]->load()>.5f&&parameters[86]->load()>.5f;config.dynamicsMode=static_cast<int>(parameters[100]->load());
  for(size_t i=0;i<16;++i){size_t offset=i==0?0:7+(i-1)*3;config.existingEq[i]={parameters[offset]->load(),parameters[offset+1]->load(),parameters[offset+2]->load(),static_cast<int>(parameters[54+i*2]->load()),parameters[55+i*2]->load()>.5f};}
  brain.startUI(config);
 }
 juce::String brainStaleReasonUI() const {
  const auto& expected=brainAuditionVariant>=0?brainAuditionValues:brainBaseline;
  if(expected.size()!=parameters.size())return "missing analysis/audition baseline";
  for(size_t i=0;i<parameters.size();++i){
   const auto id=masterlab::parameterId(i);
   auto* parameter=state.getParameter(id);
   if(!parameter)return "unknown parameter "+juce::String(id);
   const float current=parameters[i]->load();
   const float expectedValue=expected[i];
   // Compare in normalized JUCE coordinates, not with a fixed tolerance in Hz,
   // dB, ms or skewed frequency units. The previous raw 1e-5 threshold falsely
   // marked live GUI attachments stale after setValueNotifyingHost round trips.
   const float delta=std::abs(parameter->convertTo0to1(current)-
                              parameter->convertTo0to1(expectedValue));
   if(!std::isfinite(delta)||delta>2.0e-5f)
    return juce::String(id)+" changed: "+juce::String(expectedValue,6)+
           " -> "+juce::String(current,6)+" (norm delta "+juce::String(delta,7)+")";
  }
  return {};
 }
 bool brainStaleUI()const {return brainStaleReasonUI().isNotEmpty();}
 bool auditionBrainUI(int variant){
  auto snapshot=brain.readUI();
  if(snapshot.phase!=masterlab::BrainPhase::Ready||brainStaleUI()){
   brainActionMessage="Cannot audition: "+brainStaleReasonUI()+
      " (or analysis not finished).";return false;
  }
  if(variant<0||variant>1||(variant==1&&!snapshot.alternateReady))return false;
  if(brainAuditionVariant==variant)return true;
  const auto& proposal=variant==snapshot.selectedVariant?snapshot.proposal:snapshot.alternateProposal;
  if(!proposal.valid||!proposal.previewVerified||proposal.changes.empty())return false;
  for(const auto& [id,value]:proposal.changes){
   auto* p=state.getParameter(id);
   if(!p||!std::isfinite(value)||value<p->getNormalisableRange().start||
      value>p->getNormalisableRange().end)return false;
  }
  // Undo the last audition before switching. Undo on B then restores
  // the pre-Brain state, not variant A.
  if(brainAuditionVariant>=0){
   if(!undo()){
    brainActionMessage="Audition switch: Undo manager could not revert the previous preview.";
    return false;
   }
   brainAuditionVariant=-1;brainAuditionValues.clear();
  }
  if(brainStaleUI()){
   brainActionMessage="Audition switch: previous preview was not fully reverted: "+
                      brainStaleReasonUI();return false;
  }
  if(!brain.selectVariantUI(variant)){
   brainActionMessage="Audition switch: requested recipe is not available.";return false;
  }
  beginUiTransaction(variant==0?"Master Brain audition A":"Master Brain audition B");
  for(const auto& [id,value]:proposal.changes){
   auto* p=state.getParameter(id);
   p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();
  }
  finishUiTransaction();
  brainAuditionVariant=variant;brainAuditionValues.clear();
  for(auto* parameter:parameters)brainAuditionValues.push_back(parameter->load());
  brainActionMessage=variant==0?"Audition A is live in the DSP. Apply to keep it.":
                                   "Audition B is live in the DSP. Apply to keep it.";
  return true;
 }
 bool applyBrainUI(){
  auto result=brain.readUI();
  if(result.phase!=masterlab::BrainPhase::Ready||!result.proposal.valid){
   brainActionMessage="Apply blocked: no valid ready proposal. Analyse first.";return false;
  }
  if(brainStaleUI()){
   brainActionMessage="Apply blocked: parameters changed after analysis. Reanalyse.";return false;
  }
  if(brainAuditionVariant>=0){
   if(brainAuditionVariant!=result.selectedVariant)return false;
   brain.markAppliedUI();brainActionMessage="APPLIED: auditioned variant retained in DSP; Undo restores pre-Brain parameters.";
   return true;
  }
  if(result.proposal.changes.empty()){
   brainActionMessage="Apply blocked: proposal contains no changes.";return false;
  }
  bool changed=false;
  for(const auto& [id,value]:result.proposal.changes){
   auto* parameter=state.getParameter(id);
   if(!parameter||!std::isfinite(value)||value<parameter->getNormalisableRange().start||
      value>parameter->getNormalisableRange().end){
    brainActionMessage="Apply blocked: invalid parameter "+juce::String(id);return false;
   }
   changed|=std::abs(state.getRawParameterValue(id)->load()-value)>1.0e-5f;
  }
  if(!changed){
   brainActionMessage="Proposal already matches current parameters; no audible change.";return false;
  }
  beginUiTransaction("Master Brain proposal");
  for(const auto& [id,value]:result.proposal.changes){
   auto* parameter=state.getParameter(id);parameter->beginChangeGesture();
   parameter->setValueNotifyingHost(parameter->convertTo0to1(value));parameter->endChangeGesture();
  }
  finishUiTransaction();brain.markAppliedUI();
  brainActionMessage="APPLIED: "+juce::String(static_cast<int>(result.proposal.changes.size()))+
                      " parameter assignments; Undo restores previous values.";
  return true;
 }
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
};
