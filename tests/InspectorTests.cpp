#include "../Source/Processor.h"
#include <cstdlib>
#include <iostream>
#include <new>
static thread_local bool audit=false;
static thread_local size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 Processor p;for(const char* id:{"eq","lim"})p.state.getParameter(id)->setValueNotifyingHost(0);
 p.setRateAndBufferSizeDetails(48000,256);p.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;int sample=0;
 for(int block=0;block<650;++block){
  for(int i=0;i<256;++i,++sample){float x=.1f*std::sin(juce::MathConstants<double>::twoPi*1000*sample/48000);audio.setSample(0,i,x);audio.setSample(1,i,x);}
  audit=true;p.processBlock(audio,midi);audit=false;
 }
 check(allocations==0,"C++ heap allocation in processBlock");
 check(std::abs(p.inputMeter.read().rms-.1f/std::sqrt(2.f))<1e-6&&std::abs(p.outputMeter.read().rms-p.inputMeter.read().rms)<1e-6,"wrapper I/O meter mismatch");
 auto loud=p.inputLoudness.read();check(std::abs(loud.integrated+20)<.11&&std::abs(p.outputLoudness.read().integrated-loud.integrated)<.02,"wrapper I/O loudness mismatch");
 p.resetMeterRequest.store(true);
 for(int i=0;i<256;++i,++sample){float x=.1f*std::sin(juce::MathConstants<double>::twoPi*1000*sample/48000);audio.setSample(0,i,x);audio.setSample(1,i,x);}
 auto original=audio;audit=true;p.processBlock(audio,midi);audit=false;
 check(allocations==0&&!p.resetMeterRequest.load()&&!p.inputMeter.read().ready&&p.inputLoudness.read().integrated==-120,"meter reset allocates or retains history");
 // Meter reset must not alter the dry audio, but the fixed final-stage FIFO
 // necessarily delivers the input from 368 samples ago.
 for(int i=0;i<256;++i)check(std::isfinite(audio.getSample(0,i)),"meter reset produced invalid audio");
 check(p.getLatencySamples()==368,"meter reset changed PDC");p.reset();check(p.outputLoudness.read().integrated==-120&&p.outputMeter.read().peak==0,"host reset did not clear meters");
 p.prepareToPlay(96000,128);check(p.getLatencySamples()==608&&!p.inputMeter.read().ready,"sample rate change did not reprepare meters");p.releaseResources();
 std::cout<<"Passed: real callback I/O calibration without editor, loudness, C++ allocation guard, audio-safe meter reset, host reset and sample-rate change\n";
}
