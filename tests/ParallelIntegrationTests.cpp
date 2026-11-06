#include "../Source/Processor.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
void set(Processor& p,const char* id,float value){auto* parameter=p.state.getParameter(id);check(parameter!=nullptr,"missing APVTS parameter");parameter->setValueNotifyingHost(parameter->convertTo0to1(value));}
void empty(Processor& p){for(const char* id:{"dynamicEqPresent","dynamicsPresent","stereoPresent","eqPresent","limPresent","stabilizerPresent"})set(p,id,0);}
void fill(juce::AudioBuffer<float>& audio,int block,double rate){for(int i=0;i<audio.getNumSamples();++i){const double t=(block*audio.getNumSamples()+i)/rate;audio.setSample(0,i,.18f*std::sin(juce::MathConstants<double>::twoPi*311*t)+.06f*std::sin(juce::MathConstants<double>::twoPi*2400*t));audio.setSample(1,i,.13f*std::sin(juce::MathConstants<double>::twoPi*311*t+.31)+.05f*std::sin(juce::MathConstants<double>::twoPi*5100*t));}}

int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  auto parallel=std::make_unique<Processor>(),eq=std::make_unique<Processor>(),dynamics=std::make_unique<Processor>(),dry=std::make_unique<Processor>();
  for(auto* p:{parallel.get(),eq.get(),dynamics.get(),dry.get()}){empty(*p);set(*p,"gain",9);set(*p,"dynThreshold",-30);set(*p,"dynRatio",5);set(*p,"dynAttack",1);}
  parallel->setModulePresentUI(3,true);parallel->setModulePresentUI(1,true);set(*parallel,"routingMode",1);set(*parallel,"parallelTrimDynamics",6);set(*parallel,"parallelMixDynamics",.25f);set(*parallel,"parallelTrimEq",-6);set(*parallel,"parallelMixEq",.75f);set(*parallel,"parallelCompensated",1);
  eq->setModulePresentUI(3,true);dynamics->setModulePresentUI(1,true);
  for(auto* p:{parallel.get(),eq.get(),dynamics.get(),dry.get()}){p->setRateAndBufferSizeDetails(rate,257);p->prepareToPlay(rate,257);check(p->getLatencySamples()==static_cast<int>(std::ceil(rate*.005))+masterlab::FinalTruePeak::latencySamples,"APVTS parallel controls changed PDC");}
  juce::MidiBuffer midi;juce::AudioBuffer<float> a(2,257),b(2,257),c(2,257),d(2,257);const float gd=masterlab::dbGain(6),ge=masterlab::dbGain(-6);double error=0;
  for(int block=0;block<400;++block){fill(a,block,rate);b.makeCopyOf(a);c.makeCopyOf(a);d.makeCopyOf(a);parallel->processBlock(a,midi);eq->processBlock(b,midi);dynamics->processBlock(c,midi);dry->processBlock(d,midi);if(block>200)for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i){const float dm=d.getSample(ch,i)+.25f*(c.getSample(ch,i)-d.getSample(ch,i)),em=d.getSample(ch,i)+.75f*(b.getSample(ch,i)-d.getSample(ch,i));const float expected=(gd*dm+ge*em)/(gd+ge);error+=std::pow(a.getSample(ch,i)-expected,2);}}
  check(std::sqrt(error/(199*257*2))<2e-5,"APVTS trim/mix/compensation did not reach graph audio");
  juce::MemoryBlock saved;parallel->getStateInformation(saved);auto restored=std::make_unique<Processor>();restored->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
  for(const char* id:{"parallelTrimDynamics","parallelMixDynamics","parallelTrimEq","parallelMixEq","parallelCompensated"})check(restored->state.getRawParameterValue(id)->load()==parallel->state.getRawParameterValue(id)->load(),"parallel state did not restore");
 }
 std::cout<<"Passed: APVTS -> graph branch trim/mix/compensation, schema state and stable PDC at four rates\n";
}
