#include "../Source/Processor.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdlib>
#include <new>
static thread_local bool audit=false;static thread_local size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
std::vector<float> values(Processor& p){std::vector<float> out;for(auto* parameter:p.getParameters())out.push_back(parameter->getValue());return out;}
masterlab::BrainSnapshot wait(Processor& p,masterlab::BrainPhase phase){
 for(int i=0;i<15000;++i){auto snapshot=p.brain.readUI();if(snapshot.phase==phase)return snapshot;if(snapshot.phase==masterlab::BrainPhase::Failed){std::cerr<<snapshot.message<<'\n';std::exit(1);}std::this_thread::sleep_for(std::chrono::milliseconds(2));}check(false,"analysis timeout");return {};
}
int main(){
 // Brain must bound analysis input independently of the GUI selection.
 masterlab::BrainConfig bound;
 for(int requested:{30,60,120,180,300,600}){
  bound.seconds=requested;
  check(masterlab::cleanBrainConfig(bound).seconds<=20,"Brain accepted an analysis interval above 20 seconds");
 }
 bound.seconds=20;
 check(masterlab::cleanBrainConfig(bound).seconds==20,"Brain rejected the 20-second interval");
 // Exercise a real 20-second capture at 8 kHz, not just config clamping.
 {
  masterlab::MasterBrain full20;
  full20.prepare(8000);
  masterlab::BrainConfig length20;length20.seconds=20;
  full20.startUI(length20);
  for(int retry=0;retry<2000&&full20.readUI().phase!=masterlab::BrainPhase::Analysing;++retry)
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
  check(full20.readUI().phase==masterlab::BrainPhase::Analysing,"20-second worker did not start");
  std::array<float,128> wave{};
  for(int packet=0;packet<1250;++packet){
   for(int i=0;i<128;++i)wave[static_cast<size_t>(i)]=.1f*std::sin((packet*128+i)*.04f);
   full20.capture(wave.data(),wave.data(),128);
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  masterlab::BrainSnapshot twenty;
  for(int retry=0;retry<10000;++retry){
   twenty=full20.readUI();
   if(twenty.phase==masterlab::BrainPhase::Ready||twenty.phase==masterlab::BrainPhase::Failed)break;
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  check(twenty.phase==masterlab::BrainPhase::Ready&&
        std::abs(twenty.features.seconds-20.)<1.e-6,"full 20-second capture failed");
  for(int retry=0;retry<2000&&full20.resourcesUI().workerRunning;++retry)
   std::this_thread::sleep_for(std::chrono::milliseconds(1));
  check(!full20.resourcesUI().workerRunning&&full20.resourcesUI().packetBytes==0,
        "20-second job retained resources");
 }
 // Real producer/consumer race: repeatedly close and reopen the capture gate
 // while the audio callback is actively offering packets on another thread.
 // The test is intended to catch use-after-free during cancel/clear/prepare.
 {
  masterlab::MasterBrain gate(256);
  gate.prepare(48000);
  std::atomic<bool> stopProducer{false};
  std::array<float,128> input{};
  std::thread producer([&]{
   while(!stopProducer.load(std::memory_order_relaxed)){
    gate.capture(input.data(),input.data(),static_cast<int>(input.size()));
    std::this_thread::yield();
   }
  });
  masterlab::BrainConfig shortConfig;
  for(int cycle=0;cycle<16;++cycle){
   gate.startUI(shortConfig);
   for(int spin=0;spin<20;++spin)std::this_thread::yield();
   gate.cancelUI();
   gate.clearUI();
   gate.prepare(48000);
   auto resources=gate.resourcesUI();
   check(!resources.workerRunning&&!resources.jobAllocated&&resources.packetBytes==0,
         "concurrent capture retained analysis resources");
  }
  stopProducer.store(true,std::memory_order_relaxed);
  producer.join();
 }
 Processor p;p.prepareToPlay(48000,256);auto* gain=p.state.getParameter("gain");gain->setValueNotifyingHost(gain->convertTo0to1(2));auto before=values(p);
 masterlab::BrainConfig config;p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);
 juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;int sample=0;
 // Keep the accelerated host simulation bounded. A tight loop can outrun the
 // background analyzer on fast CI CPUs and tests queue overflow instead of capture.
 auto feed=[&](bool silent){for(int block=0;block<1500;++block){for(int i=0;i<256;++i,++sample){float x=silent?0.f:.15f*std::sin(juce::MathConstants<double>::twoPi*1000*sample/48000);buffer.setSample(0,i,x);buffer.setSample(1,i,x);}audit=true;p.processBlock(buffer,midi);audit=false;std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
 feed(false);auto result=wait(p,masterlab::BrainPhase::Ready);
 check(allocations==0,"audio capture allocated C++ heap memory");check(result.config.useTruePeak&&result.config.targetLufs>=-20.f&&result.config.targetLufs<=-9.f&&result.config.ceiling<=-1.f,"adaptive mastering targets were not calculated");
 const auto measuredInput=*std::max_element(result.features.inputPreviewEnvelope.begin(),
     result.features.inputPreviewEnvelope.end());
 const auto measuredOutput=*std::max_element(result.proposal.previewOutputEnvelope.begin(),
     result.proposal.previewOutputEnvelope.end());
 check(measuredInput>.05f&&measuredOutput>.01f&&std::isfinite(measuredOutput),
       "real input/output waveform envelopes were not captured from DSP");
 check(std::isfinite(result.proposal.previewTruePeakDb),"preview true-peak measurement unavailable");check(result.proposal.previewVerified&&result.proposal.previewPasses>=1,"proposal skipped actual DSP verification");check(result.proposal.valid&&std::abs(result.features.seconds-8)<1e-6&&result.progress==1,"capture result or duration incorrect");
 for(int retry=0;retry<2000&&p.brain.resourcesUI().workerRunning;++retry)
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 const auto idleResources=p.brain.resourcesUI();
 check(!idleResources.workerRunning&&!idleResources.jobAllocated&&idleResources.packetBytes==0,
       "Brain retained worker, audio buffers or FFT after completion");
 check(std::abs(result.features.integrated-(-16.48f))<.15f&&result.features.correlation>.999f,"captured audio measurements incorrect");check(values(p)==before,"analysis changed parameters without approval");
 auto revised=config;revised.genre=4;revised.intensity=.5f;p.brain.reviseUI(revised);
 check(p.brain.readUI().phase==masterlab::BrainPhase::Idle&&values(p)==before,"revision must request new capture after PCM is released");
 p.startBrainUI(revised);wait(p,masterlab::BrainPhase::Analysing);p.brain.cancelRT();wait(p,masterlab::BrainPhase::Cancelled);
 check(values(p)==before,"cancel changed host parameters");
 p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);feed(false);wait(p,masterlab::BrainPhase::Ready);
 gain->setValueNotifyingHost(gain->convertTo0to1(3));auto manual=values(p);check(p.brainStaleUI()&&!p.applyBrainUI()&&values(p)==manual,"stale proposal overwrote manual settings");gain->setValueNotifyingHost(gain->convertTo0to1(2));
 check(p.applyBrainUI()&&values(p)!=before,"approved proposal not applied");for(const char* id:{"stereoPresent","dynamicsPresent","dynamicEqPresent"}){float proposed=-1;for(const auto& change:p.brain.readUI().proposal.changes)if(change.first==id)proposed=change.second;check(p.state.getRawParameterValue(id)->load()==proposed,"Brain did not apply its selected module presence");}check(p.undo()&&values(p)==before,"proposal is not one Undo transaction");check(!p.applyBrainUI(),"applied proposal applied twice");
 // Variant A/B are distinct verified recipes from one capture. Their audition
 // switches the *actual processor* with reversible host automation.
 p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);
 feed(false);auto variants=wait(p,masterlab::BrainPhase::Ready);
 check(variants.alternateReady&&variants.alternateProposal.previewVerified,
       "two real DSP-verified mastering variants were not produced");
 check(variants.features.detail.sectionLevelsDb.size()>=2,
       "consecutive full-capture loudness sections are unavailable");
 check(values(p)==before,"A/B preview changed host settings before user request");
 check(p.auditionBrainUI(0),"audition A failed");
 auto aValues=values(p);check(aValues!=before,"audition A made no audio-parameter changes");
 check(p.auditionBrainUI(1),"audition B failed");
 auto bValues=values(p);check(bValues!=aValues,"audition B did not change the mastering recipe");
 check(p.brain.readUI().selectedVariant==1&&!p.brainStaleUI(),"B selection stale or lost");
 check(p.applyBrainUI()&&p.brain.readUI().phase==masterlab::BrainPhase::Applied,
       "Apply did not commit the audible B mastering result");
 check(p.undo()&&values(p)==before,"Undo after B Apply did not restore pre-Brain parameters");
 check(!p.applyBrainUI(),"Apply ran again after the variant was committed");
 // Preview must agree with the actual Processor after applying exactly its settings.
 std::array<float,masterlab::parameterCount> settings{};for(size_t i=0;i<settings.size();++i)settings[i]=p.state.getRawParameterValue(masterlab::parameterId(i))->load();
 // The preceding Undo restored the baseline: verify that state, including existing EQ.
 std::vector<float> testLeft(48000),testRight(48000);for(size_t i=0;i<testLeft.size();++i)testLeft[i]=testRight[i]=static_cast<float>(.1*std::sin(i*.13));
 auto preview=masterlab::renderBrainPreview(testLeft,testRight,48000,settings,[]{return false;});check(preview.complete,"actual DSP preview failed");
 p.prepareToPlay(48000,256);masterlab::LoudnessMeter actual;actual.prepare(48000);float peak=0;const size_t latency=static_cast<size_t>(p.getLatencySamples());
 for(size_t offset=0;offset<testLeft.size()+latency;offset+=128){int n=static_cast<int>(std::min(size_t{128},testLeft.size()+latency-offset));buffer.setSize(2,n);for(int i=0;i<n;++i){float x=offset+i<testLeft.size()?testLeft[offset+i]:0;buffer.setSample(0,i,x);buffer.setSample(1,i,x);}p.processBlock(buffer,midi);for(int i=0;i<n;++i)if(offset+i>=latency){actual.push(buffer.getSample(0,i),buffer.getSample(1,i));peak=std::max(peak,std::abs(buffer.getSample(0,i)));}}
 check(std::abs(actual.read().integrated-preview.lufs)<.02f&&std::abs(peak-preview.peak)<.002f,"preview differs from actual Processor");
 check(!masterlab::renderBrainPreview(testLeft,testRight,48000,settings,[]{return true;}).complete,"preview ignored cancellation");buffer.setSize(2,256);
 p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);feed(true);auto silence=wait(p,masterlab::BrainPhase::Ready);check(!silence.proposal.valid&&!p.applyBrainUI()&&values(p)==before,"silence generated or applied a proposal");
 for(int retry=0;retry<2000&&p.brain.resourcesUI().workerRunning;++retry)
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
 check(!p.brain.resourcesUI().jobAllocated&&p.brain.resourcesUI().packetBytes==0,
       "silent input retained analysis buffers");
 p.startBrainUI(config);p.brain.cancelRT();for(int i=0;i<200&&p.brain.readUI().phase!=masterlab::BrainPhase::Cancelled;++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));check(p.brain.readUI().phase==masterlab::BrainPhase::Cancelled,"pending analysis did not cancel");
 // Deliberately undersized queue: data loss must invalidate the entire analysis.
 masterlab::MasterBrain tiny(2);tiny.prepare(48000);tiny.startUI(config);for(int i=0;i<200&&tiny.readUI().phase!=masterlab::BrainPhase::Analysing;++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));
 std::array<float,4096> zeros{};tiny.capture(zeros.data(),zeros.data(),4096);for(int i=0;i<200&&tiny.readUI().phase!=masterlab::BrainPhase::Failed;++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));check(tiny.readUI().phase==masterlab::BrainPhase::Failed,"queue overflow produced a usable partial result");
 // NEW / factory preset zero must restore every declared host default,
 // invalidate Brain proposals and remain one Undo transaction.
 p.beginUiTransaction("Configure non-default test scene");
 for(const char* id:{"gain16","limiterDrive","dynPunch","stereoLowWidth","analogDrive","bassSub"}){
  auto* a=p.state.getParameter(id);
  a->beginChangeGesture();a->setValueNotifyingHost(.87f);a->endChangeGesture();
 }
 p.finishUiTransaction();
 const auto configured=values(p);
 check(p.loadFactoryDefaultUI(),"NEW failed to execute");
 check(p.brain.readUI().phase==masterlab::BrainPhase::Idle,
       "NEW left an old Brain result or selected variant");
 check(p.brainBaseline.empty()&&p.brainAuditionValues.empty()&&p.brainAuditionVariant==-1,
       "NEW retained stale Brain state");
 for(auto* parameter:p.getParameters())
  check(std::abs(parameter->getValue()-parameter->getDefaultValue())<1.0e-6f,
        "NEW did not reset all host parameters to factory defaults");
 check(p.undo()&&values(p)==configured,"factory NEW is not a single Undo operation");
 check(p.redo(),"factory NEW redo failed");
 for(auto* parameter:p.getParameters())
  check(std::abs(parameter->getValue()-parameter->getDefaultValue())<1.0e-6f,
        "factory NEW redo did not restore all defaults");
 // Real four-stem playback from aligned WAV files. This never fakes source
 // separation with spectral filters and does not require inference in CTest.
 {
  const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getNonexistentChildFile("masterlab-stem-test","",false);
  check(folder.createDirectory().wasOk(),"cannot create stem test folder");
  juce::WavAudioFormat wav;
  for(int stem=0;stem<4;++stem){
   juce::AudioBuffer<float> source(2,1024);
   const float amplitude=(stem+1)*.04f;
   for(int i=0;i<1024;++i){source.setSample(0,i,amplitude);source.setSample(1,i,amplitude);}
   auto file=folder.getChildFile(juce::String(masterlab::stemNames[static_cast<size_t>(stem)])+".wav");
   std::unique_ptr<juce::FileOutputStream> output(file.createOutputStream());
   check(output!=nullptr,"stem WAV output missing");
   auto* raw=output.release();
   std::unique_ptr<juce::AudioFormatWriter> writer(
      wav.createWriterFor(raw,44100,2,32,{},0));
   check(writer!=nullptr,"stem WAV writer unavailable");
   check(writer->writeFromAudioSampleBuffer(source,0,1024),"stem WAV write failed");
  }
  masterlab::StemRemix mixer;
  auto error=mixer.loadFolder(folder);
  check(error.isEmpty(),"four aligned WAVs failed to import");
  check(mixer.snapshot()!=nullptr&&mixer.snapshot()->samples==1024,
        "stem bank did not load exact samples");
  std::array<float,21> recipe{};
  recipe[0]=1;
  for(int stem=0;stem<4;++stem)recipe[static_cast<size_t>(3+5*stem)]=1.f;
  juce::AudioBuffer<float> result(2,128);
  result.clear();
  mixer.render(result,44100.,0,true,recipe);
  check(std::abs(result.getSample(0,25)-.4f)<1.e-4f,
        "all stem playback did not reconstruct the synthetic mix");
  recipe[static_cast<size_t>(1+3*5+3)]=1.f; // mute vocals
  result.clear();mixer.render(result,44100.,128,true,recipe);
  check(std::abs(result.getSample(0,25)-.24f)<1.e-4f,
        "mute vocals leaked into stem mix");
  recipe[static_cast<size_t>(1+3*5+3)]=0.f;
  recipe[static_cast<size_t>(1+1*5+4)]=1.f; // solo bass
  result.clear();mixer.render(result,44100.,256,true,recipe);
  check(std::abs(result.getSample(0,25)-.08f)<1.e-4f,
        "solo bass did not isolate real bass WAV");
  recipe[static_cast<size_t>(1+1*5+4)]=0.f;
  recipe[0]=0;result.clear();result.applyGain(.0f);
  result.addSample(0,8,.6f);
  mixer.render(result,44100.,384,true,recipe);
  check(std::abs(result.getSample(0,8)-.6f)<1.e-6f,
        "stem bypass did not preserve original DAW audio");
  recipe[0]=1;result.clear();
  mixer.render(result,44100.,1100,true,recipe);
  check(std::abs(result.getSample(0,20))<1.e-7f,"stems did not end at file boundary");
  mixer.unload();
  check(mixer.snapshot()==nullptr,"unload retained stale stem audio");
  folder.deleteRecursively();
 }
 p.releaseResources();std::cout<<"Passed: real audio capture, exact duration, no callback C++ allocations, independent worker measurements, no automatic changes, revision, stale protection, single Undo, silence, cancel and overflow\n";
}
