#include "../Source/Processor.h"
#include <iostream>
#include <cstdlib>
void check(bool ok,const char* reason){if(!ok){std::cerr<<reason<<'\n';std::exit(1);}}
void set(Processor& p,const juce::String& id,float x){auto* v=p.state.getParameter(id);check(v!=nullptr,"missing parameter");v->setValueNotifyingHost(v->convertTo0to1(x));}
void empty(Processor& p){for(const char* id:{"eqPresent","limPresent","dynamicEqPresent","dynamicsPresent","stereoPresent","stabilizerPresent"})set(p,id,0);}
void input(juce::AudioBuffer<float>& a,int block,double rate){for(int i=0;i<a.getNumSamples();++i){double t=(block*a.getNumSamples()+i)/rate;float mid=static_cast<float>(.2*std::sin(t*6283.185307)+.1*std::sin(t*502.654825));float side=static_cast<float>(.1*std::sin(t*21991.148575));a.setSample(0,i,mid+side);a.setSample(1,i,mid-side);}}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.})for(int mode:{0,1})for(int kind=0;kind<6;++kind){
  auto pOwner=std::make_unique<Processor>(),dryOwner=std::make_unique<Processor>();auto& p=*pOwner;auto& dry=*dryOwner;empty(p);empty(dry);set(p,"routingMode",static_cast<float>(mode));
  const char* on=std::array<const char*,6>{"dynamicEqOn","dynamicsOn","stereoOn","eq","lim","stabilizerOn"}[kind];set(p,on,0);
  if(kind==0){for(int k=1;k<=3;++k)set(p,"deqBandOn"+juce::String(k),0);set(p,"deqFreq1",1000);set(p,"deqThreshold1",-60);set(p,"deqRange1",-12);set(p,"deqAttack1",.1f);}
  if(kind==1){set(p,"dynThreshold",-40);set(p,"dynRatio",8);set(p,"dynAttack",1);}
  if(kind==2){for(const char* id:{"stereoLowWidth","stereoMidWidth","stereoHighWidth","stereoAirWidth"})set(p,id,0);}
  if(kind==3){for(int k=1;k<=16;++k)set(p,"bandEnabled"+juce::String(k),0);set(p,"gain",-12);}
  if(kind==4){set(p,"limiterDrive",12);set(p,"ceiling",-12);}
  if(kind==5){set(p,"stabilizerAmount",100);set(p,"stabilizerMode",1);set(p,"stabilizerSpeed",0);}
  p.setModulePresentUI(kind,true);check(p.state.getRawParameterValue(on)->load()==1,"add leaves module bypassed");
  p.prepareToPlay(rate,257);dry.prepareToPlay(rate,257);juce::MidiBuffer midi;juce::AudioBuffer<float> a(2,257),b(2,257);double difference=0;
  int blocks=static_cast<int>(rate*2/257);for(int block=0;block<blocks;++block){input(a,block,rate);b.makeCopyOf(a);p.processBlock(a,midi);dry.processBlock(b,midi);if(block>blocks/2)for(int c=0;c<2;++c)for(int i=0;i<257;++i){check(std::isfinite(a.getSample(c,i)),"module emitted nonfinite audio");difference+=std::abs(a.getSample(c,i)-b.getSample(c,i));}}
  if(difference<1){std::cerr<<"No DSP reaction kind="<<kind<<" routing="<<mode<<" rate="<<rate<<" difference="<<difference<<'\n';return 1;}
  set(p,on,0);for(int block=blocks;block<blocks+static_cast<int>(rate*.5/257)+10;++block){input(a,block,rate);b.makeCopyOf(a);p.processBlock(a,midi);dry.processBlock(b,midi);if(block>blocks+static_cast<int>(rate*.4/257))for(int c=0;c<2;++c)for(int i=0;i<257;++i)check(std::abs(a.getSample(c,i)-b.getSample(c,i))<1e-5,"module bypass does not restore dry PDC");}
 }
 auto undoOwner=std::make_unique<Processor>();auto& undo=*undoOwner;empty(undo);set(undo,"eq",0);for(int i=1;i<=16;++i)set(undo,"bandEnabled"+juce::String(i),0);undo.setModulePresentUI(3,true);check(undo.undo(),"add cannot be undone");check(undo.state.getRawParameterValue("eqPresent")->load()==0&&undo.state.getRawParameterValue("eq")->load()==0&&undo.state.getRawParameterValue("bandEnabled1")->load()==0,"add/activation is not one Undo transaction");
 // An EQ and a compressor in parallel must equal the mean of their independent outputs.
 auto parallelOwner=std::make_unique<Processor>(),eqOwner=std::make_unique<Processor>(),dynamicsOwner=std::make_unique<Processor>();auto& parallel=*parallelOwner;auto& eq=*eqOwner;auto& dynamics=*dynamicsOwner;for(auto* p:{&parallel,&eq,&dynamics}){empty(*p);set(*p,"gain",6);set(*p,"dynThreshold",-30);set(*p,"dynRatio",4);}
 parallel.setModulePresentUI(3,true);parallel.setModulePresentUI(1,true);set(parallel,"routingMode",1);eq.setModulePresentUI(3,true);dynamics.setModulePresentUI(1,true);
 for(auto* p:{&parallel,&eq,&dynamics})p->prepareToPlay(48000,257);juce::MidiBuffer midi;juce::AudioBuffer<float> a(2,257),b(2,257),c(2,257);
 for(int block=0;block<300;++block){input(a,block,48000);b.makeCopyOf(a);c.makeCopyOf(a);parallel.processBlock(a,midi);eq.processBlock(b,midi);dynamics.processBlock(c,midi);if(block>100)for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i)check(std::abs(a.getSample(ch,i)-(b.getSample(ch,i)+c.getSample(ch,i))*.5f)<1e-5,"parallel is not the aligned mean of real module outputs");}
 juce::MemoryBlock state;parallel.getStateInformation(state);auto restoredOwner=std::make_unique<Processor>();auto& restored=*restoredOwner;restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));check(restored.state.getRawParameterValue("routingMode")->load()==1,"parallel mode lost on restore");
 std::cout<<"Passed: all six modules react after re-add from bypass/all-bands-off, both topologies, four sample rates, bypass transparency, independent parallel sum and routing state\n";
}
