#include "../Source/Processor.h"
#include <iostream>
#include <cstdlib>
void check(bool ok,const char* m){if(!ok){std::cerr<<m<<'\n';std::exit(1);}}
void set(Processor& p,const char* id,float v){auto* parameter=p.state.getParameter(id);check(parameter,"missing APVTS id");parameter->setValueNotifyingHost(parameter->convertTo0to1(v));}
std::vector<float> run(int rate,bool present,bool enabled,bool parallel,bool extra){
 Processor p;set(p,"eqPresent",extra);set(p,"eq",0);set(p,"limPresent",0);set(p,"stereoPresent",present);set(p,"stereoOn",enabled);set(p,"stereoizeAmount",.7f);set(p,"routingMode",parallel);
 p.prepareToPlay(rate,257);check(p.getLatencySamples()==int(std::ceil(rate*.005))+masterlab::FinalTruePeak::latencySamples,"Stereoize changed PDC");
 std::vector<float> side;juce::AudioBuffer<float> audio(2,257);juce::MidiBuffer midi;
 for(int block=0;block<int(rate*2/257);++block){for(int i=0;i<257;++i){float x=.1f*std::sin(6.283185307*713*(block*257+i)/rate);audio.setSample(0,i,x);audio.setSample(1,i,x);}p.processBlock(audio,midi);if(block*257>rate)for(int i=0;i<257;++i)side.push_back((audio.getSample(0,i)-audio.getSample(1,i))*.5f);}
 return side;
}
int main(){
 for(int rate:{44100,48000,96000}){
  auto active=run(rate,true,true,false,false),removed=run(rate,false,true,false,false),bypass=run(rate,true,false,false,false),parallel=run(rate,true,true,true,true);
  double energy=0;for(size_t i=0;i<active.size();++i){energy+=active[i]*active[i];check(removed[i]==0&&bypass[i]==0,"presence/enabled flags do not bypass Stereoize");check(std::abs(parallel[i]*2-active[i])<2e-6,"parallel no longer dilutes by branch count");}
  check(energy/active.size()>1e-5,"APVTS -> graph -> Stereoize is inaudible on mono");
 }
 std::cout<<"Passed: Stereoize APVTS reaches audio, presence/on flags, Parallel 1/N diagnosis and unchanged PDC at three rates\n";
}
