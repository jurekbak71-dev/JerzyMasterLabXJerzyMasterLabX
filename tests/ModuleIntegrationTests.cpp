#include "../Source/Processor.h"
#include <iostream>
#include <cstdlib>
#include <new>
static thread_local bool audit=false;static thread_local size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool condition,const char* message){if(!condition){std::cerr<<message<<'\n';std::exit(1);}}
void set(Processor& p,const char* id,float number){auto* parameter=p.state.getParameter(id);check(parameter!=nullptr,"unknown parameter");parameter->setValueNotifyingHost(parameter->convertTo0to1(number));}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  Processor p;for(const char* id:{"eqPresent","limPresent","dynamicEqPresent","dynamicsPresent","stereoPresent"})set(p,id,0);
  p.prepareToPlay(rate,257);juce::AudioBuffer<float> audio(2,257);juce::MidiBuffer midi;int sample=0;const int latency=p.getLatencySamples();
  auto render=[&](int blocks,bool monoSum){for(int block=0;block<blocks;++block){for(int i=0;i<257;++i,++sample){audio.setSample(0,i,.1f*std::sin(sample*.031));audio.setSample(1,i,.08f*std::cos(sample*.017));}audit=true;p.processBlock(audio,midi);audit=false;if(sample>static_cast<int>(rate))for(int i=0;i<257;++i){const int original=sample-257+i-latency;const float l=.1f*std::sin(original*.031),r=.08f*std::cos(original*.017);if(monoSum)check(std::abs(audio.getSample(0,i)+audio.getSample(1,i)-l-r)<1e-5,"stereo module changes mono sum");else check(std::abs(audio.getSample(0,i)-l)<1e-5&&std::abs(audio.getSample(1,i)-r)<1e-5,"empty chain changes audio beyond PDC");}check(p.getLatencySamples()==latency,"add/remove changed latency");}};
  render(static_cast<int>(rate/257)+10,false);set(p,"stereoizeAmount",.7f);set(p,"stereoLowWidth",.2f);set(p,"stereoMidWidth",.5f);set(p,"stereoHighWidth",1.7f);set(p,"stereoPresent",1);render(80,true);
  set(p,"stereoPresent",0); // Let the 20 ms fade settle before asserting bit-level transparency.
  for(int i=0;i<static_cast<int>(rate*.4/257)+1;++i){audio.clear();p.processBlock(audio,midi);}sample=0;render(static_cast<int>(rate/257)+10,false);
  set(p,"stereoPresent",1);set(p,"dynamicsPresent",1);set(p,"dynamicEqPresent",1);set(p,"dynamicsMode",1);set(p,"upwardAmount",6);set(p,"deqRange2",-7);set(p,"chainOrder",693);set(p,"stabilizerPresent",1);set(p,"stabilizerAmount",67);
  juce::MemoryBlock saved;p.getStateInformation(saved);Processor restored;restored.setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));p.prepareToPlay(rate,257);restored.prepareToPlay(rate,257);
  juce::AudioBuffer<float> other(2,257);for(int block=0;block<100;++block){for(int i=0;i<257;++i){audio.setSample(0,i,.15f*std::sin((block*257+i)*.11));audio.setSample(1,i,-audio.getSample(0,i)*.7f);}other.makeCopyOf(audio);audit=true;p.processBlock(audio,midi);restored.processBlock(other,midi);audit=false;for(int c=0;c<2;++c)for(int i=0;i<257;++i)check(std::isfinite(audio.getSample(c,i))&&audio.getSample(c,i)==other.getSample(c,i),"restored modular render differs");}
 }
 // The INSERT profile must preserve a time-aligned dry reference without
 // changing the DSP controls or the host-reported processing latency.
 {
  Processor insert;
  set(insert,"workMode",1);set(insert,"insertBlend",0);
  set(insert,"eqPresent",1);set(insert,"limPresent",1);
  insert.prepareToPlay(48000.,257);
  const int delay=insert.getLatencySamples();
  juce::AudioBuffer<float> signal(2,257);juce::MidiBuffer midi;
  int samples=0;
  for(int block=0;block<220;++block){
   for(int i=0;i<257;++i,++samples){
    signal.setSample(0,i,.21f*std::sin(samples*.071f));
    signal.setSample(1,i,.11f*std::cos(samples*.047f));
   }
   audit=true;insert.processBlock(signal,midi);audit=false;
   if(block>150)for(int i=0;i<257;++i){
    const int t=samples-257+i-delay;
    const float expectedL=.21f*std::sin(t*.071f),expectedR=.11f*std::cos(t*.047f);
    check(std::abs(signal.getSample(0,i)-expectedL)<1.e-5f&&
          std::abs(signal.getSample(1,i)-expectedR)<1.e-5f,
          "INSERT 0 wet does not preserve latency-aligned input");
   }
  }
  check(insert.getLatencySamples()==delay,"INSERT mode changed host PDC");
 }
 // INSERT 0% must be blended ahead of the *physical final* true-peak
 // limiter. A high-level dry signal may not bypass the output ceiling.
 {
  Processor insert;
  set(insert,"workMode",1);set(insert,"insertBlend",0);
  set(insert,"truePeakPresent",1);set(insert,"truePeakOn",1);
  set(insert,"truePeakCeiling",-6.f);
  insert.prepareToPlay(48000.,257);
  const float ceiling=std::pow(10.f,-6.f/20.f);
  juce::AudioBuffer<float> signal(2,257);juce::MidiBuffer midi;
  for(int block=0;block<160;++block){
   for(int i=0;i<257;++i){
    const float sample=.96f*std::sin((block*257+i)*.17f);
    signal.setSample(0,i,sample);signal.setSample(1,i,sample);
   }
   audit=true;insert.processBlock(signal,midi);audit=false;
   if(block>60)for(int i=0;i<257;++i)
    check(std::abs(signal.getSample(0,i))<=ceiling+1.e-4f&&
          std::abs(signal.getSample(1,i))<=ceiling+1.e-4f,
          "INSERT dry blend bypassed fixed final limiter");
  }
 }
 check(allocations==0,"prepared module chain allocated in callback");std::cout<<"Passed: add/remove transparency and stable PDC, stereo mono sum, Analog/upward/Dynamic EQ state and deterministic audio, no callback allocation at four sample rates\n";
}
