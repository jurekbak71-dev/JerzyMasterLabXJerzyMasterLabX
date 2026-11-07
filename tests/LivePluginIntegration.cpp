// Real VST3 processor path, not just the standalone backend benchmark.
// Requires pinned StemgenRT evaluation models available via
// MASTERLAB_STEMGENRT_MODELS and MASTERLAB_LIVE_STEM_PLUGIN=1.
#include "../Source/Processor.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<#x<<" at "<<__LINE__<<"\n";std::abort();}}while(false)
int main(){
#if !(defined(MASTERLAB_LIVE_STEM_PLUGIN) && MASTERLAB_LIVE_STEM_PLUGIN)
 std::cerr<<"Not a live plugin build\n";return 2;
#else
 Processor p;
 auto* live=p.state.getParameter("stemLiveOn");CHECK(live);
 live->setValueNotifyingHost(1.f);
 auto* offline=p.state.getParameter("stemMode");CHECK(offline);
 offline->setValueNotifyingHost(offline->convertTo0to1(0.f));
 p.setRateAndBufferSizeDetails(44100,128);
 p.prepareToPlay(44100,128);
 CHECK(p.liveNeuralAvailable());
 CHECK(p.getLatencySamples()>=4096);
 auto* mute=p.state.getParameter("stemVocalsMute");CHECK(mute);
 juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,128);
 float maxOut=0;int finite=0;
 for(int block=0;block<400;++block){
  for(int i=0;i<128;++i){
   const float at=float(block*128+i);
   audio.setSample(0,i,.08f*std::sin(at*.015f)+.03f*std::cos(at*.043f));
   audio.setSample(1,i,.09f*std::sin(at*.016f)+.025f*std::cos(at*.031f));
  }
  if(block==200)mute->setValueNotifyingHost(1.f);
  p.processBlock(audio,midi);
  for(int c=0;c<2;++c)for(int i=0;i<128;++i){
   const float v=audio.getSample(c,i);CHECK(std::isfinite(v));
   maxOut=std::max(maxOut,std::abs(v));++finite;
  }
  // This is a paced integration smoke, not a DAW endurance qualification.
  std::this_thread::sleep_for(std::chrono::milliseconds(3));
 }
 CHECK(finite==400*256);
 CHECK(maxOut>.001f);
 p.releaseResources();
 CHECK(!p.liveNeuralAvailable());
 p.setRateAndBufferSizeDetails(48000,128);
 p.prepareToPlay(48000,128);
 CHECK(!p.liveNeuralAvailable()); // 44.1 kHz-only weights must not pitch-shift 48 kHz
 p.releaseResources();
 std::cout<<"Live VST3 processor path: neural model initialized, host audio processed,"
          <<" stem mute automated, 44.1k active / 48k safe fallback.\n";
 return 0;
#endif
}
