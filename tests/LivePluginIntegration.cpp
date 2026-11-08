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
#include <memory>
#include <thread>
#define CHECK(x) do{if(!(x)){std::cerr<<"FAIL "<<#x<<" at "<<__LINE__<<"\n";std::abort();}}while(false)
int main(){
#if !(defined(MASTERLAB_LIVE_STEM_PLUGIN) && MASTERLAB_LIVE_STEM_PLUGIN)
 std::cerr<<"Not a live plugin build\n";return 2;
#else
 juce::ScopedJuceInitialiser_GUI gui;
 Processor p;
 auto* live=p.state.getParameter("stemLiveOn");CHECK(live);
 // Model must not be constructed merely by opening the plugin.
 live->setValueNotifyingHost(0.f);
 auto* offline=p.state.getParameter("stemMode");CHECK(offline);
 offline->setValueNotifyingHost(offline->convertTo0to1(0.f));
 p.setRateAndBufferSizeDetails(44100,128);
 p.prepareToPlay(44100,128);
 CHECK(!p.liveNeuralAvailable()); // essential FL Studio safe-load regression
 // Reproduce FL Studio's actual GUI toggle, rather than calling the
 // processor's model initializer directly from a test.
 {
  std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
  CHECK(editor!=nullptr);
  auto* panel=editor->findChildWithID("stem-brain-panel");CHECK(panel);
  auto* button=dynamic_cast<juce::Button*>(panel->findChildWithID("stem-live-neural"));
  CHECK(button);
  button->setToggleState(true,juce::sendNotificationSync);
  button->onClick(); // exact production onClick path that previously crashed FL Studio
  // The CLI harness has no running JUCE message loop: simulate delivery
  // of the already-queued message-thread activation explicitly.
  // The exact production button onClick above is still exercised.
  p.startLiveNeuralUI();
  CHECK(live->getValue()>.5f);
  CHECK(!p.liveNeuralAvailable()); // no blocking ONNX load inside GUI callback
  // State restore while a model is being loaded must be non-blocking.
  juce::MemoryBlock savedState;
  p.getStateInformation(savedState);
  CHECK(savedState.getSize()>0);
  p.setStateInformation(savedState.getData(),static_cast<int>(savedState.getSize()));
  for(int tick=0;tick<2500 && !p.liveNeuralAvailable();++tick){
   p.finishLiveNeuralUI(); // explicit message-thread pump for CLI test host
   std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  CHECK(p.liveNeuralAvailable());
  p.setStateInformation(savedState.getData(),static_cast<int>(savedState.getSize()));
  CHECK(p.liveNeuralAvailable());
 }

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
  if(block==100)offline->setValueNotifyingHost(offline->convertTo0to1(1.f));
  if(block==200){
   offline->setValueNotifyingHost(offline->convertTo0to1(2.f));
   mute->setValueNotifyingHost(1.f);
  }
  if(block==300)offline->setValueNotifyingHost(offline->convertTo0to1(0.f));
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
 CHECK(p.liveWorker!=nullptr);
 CHECK(p.liveWorker->statistics().hops>20);
 // Regression: the OFFLINE source-mode selector cannot turn neural LIVE off.
 CHECK(p.liveNeuralAvailable());
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
