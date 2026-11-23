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
 // The model and GUI must never accept a capture longer than twenty seconds.
 masterlab::BrainConfig bound;
 for(int requested:{30,60,120,180,300,600}){
  bound.seconds=requested;
  check(masterlab::cleanBrainConfig(bound).seconds<=20,"Analysis length above 20 seconds accepted");
 }
 bound.seconds=20;
 check(masterlab::cleanBrainConfig(bound).seconds==20,"Twenty-second capture rejected");
 Processor p;p.prepareToPlay(48000,256);auto* gain=p.state.getParameter("gain");gain->setValueNotifyingHost(gain->convertTo0to1(2));auto before=values(p);
 masterlab::BrainConfig config;p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);
 juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;int sample=0;
 // Keep the accelerated host simulation bounded. A tight loop can outrun the
 // background analyzer on fast CI CPUs and tests queue overflow instead of capture.
 auto feed=[&](bool silent){for(int block=0;block<1500;++block){for(int i=0;i<256;++i,++sample){float x=silent?0.f:.15f*std::sin(juce::MathConstants<double>::twoPi*1000*sample/48000);buffer.setSample(0,i,x);buffer.setSample(1,i,x);}audit=true;p.processBlock(buffer,midi);audit=false;std::this_thread::sleep_for(std::chrono::milliseconds(1));}};
 feed(false);auto result=wait(p,masterlab::BrainPhase::Ready);
 check(allocations==0,"audio capture allocated C++ heap memory");
 check(result.config.useTruePeak&&result.config.targetLufs>=-20.f&&result.config.targetLufs<=-9.f&&result.config.ceiling<=-1.f,"Automatic LUFS/TP goals missing");
 check(std::isfinite(result.proposal.previewTruePeakDb),"4x true peak was not measured");
 float proposedFinalDrive=-1.f;
 for(const auto& change:result.proposal.changes)
  if(change.first=="truePeakDrive")proposedFinalDrive=change.second;
 check(proposedFinalDrive>=0.f&&proposedFinalDrive<=18.f,
       "Brain proposal does not address the actual Final Maximizer input drive");check(result.proposal.previewVerified&&result.proposal.previewPasses>=1,"proposal skipped actual DSP verification");check(result.proposal.valid&&std::abs(result.features.seconds-8)<1e-6&&result.progress==1,"capture result or duration incorrect");
 for(int i=0;i<2000&&p.brain.resourcesUI().workerRunning;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
 check(!p.brain.resourcesUI().workerRunning&&!p.brain.resourcesUI().jobAllocated&&p.brain.resourcesUI().packetBytes==0,"Brain kept worker or PCM/FFT while idle");
 check(std::abs(result.features.integrated-(-16.48f))<.15f&&result.features.correlation>.999f,"captured audio measurements incorrect");check(values(p)==before,"analysis changed parameters without approval");
 auto revised=config;revised.genre=4;revised.intensity=.5f;p.brain.reviseUI(revised);
 check(p.brain.readUI().phase==masterlab::BrainPhase::Idle&&values(p)==before,"Changed settings should request fresh capture");
 p.startBrainUI(revised);wait(p,masterlab::BrainPhase::Analysing);p.brain.cancelRT();wait(p,masterlab::BrainPhase::Cancelled);check(values(p)==before,"Cancellation changed parameters");
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
 // Independently measured 4x reconstructed peak cannot be zero for
 // non-silent output. The old preview disabled its detector entirely.
 const float measuredSampleDb=20.f*std::log10(std::max(1.e-9f,preview.peak));
 check(preview.truePeakDb>-35.f&&
       std::abs(preview.truePeakDb-measuredSampleDb)<3.f,
       "Brain true-peak detector reported a false zero instead of actual audio");
 // Isolate final-stage gain from the legacy masked limiter drive. This
 // catches the original regression: Master Brain changed the wrong parameter.
 auto lowFinal=settings;lowFinal[221]=1.f;lowFinal[222]=1.f;lowFinal[223]=-1.f;lowFinal[225]=0.f;
 auto highFinal=lowFinal;highFinal[225]=6.f;
 const auto lowPreview=masterlab::renderBrainPreview(testLeft,testRight,48000,lowFinal,[]{return false;});
 const auto highPreview=masterlab::renderBrainPreview(testLeft,testRight,48000,highFinal,[]{return false;});
 check(lowPreview.complete&&highPreview.complete&&
       highPreview.lufs>lowPreview.lufs+4.5f,
       "Final Maximizer gain did not change measured mastering loudness");
 check(highPreview.truePeakDb<=-.85f,
       "Final Maximizer gain produced a true-peak overshoot");
 p.prepareToPlay(48000,256);masterlab::LoudnessMeter actual;actual.prepare(48000);float peak=0;const size_t latency=static_cast<size_t>(p.getLatencySamples());
 for(size_t offset=0;offset<testLeft.size()+latency;offset+=128){int n=static_cast<int>(std::min(size_t{128},testLeft.size()+latency-offset));buffer.setSize(2,n);for(int i=0;i<n;++i){float x=offset+i<testLeft.size()?testLeft[offset+i]:0;buffer.setSample(0,i,x);buffer.setSample(1,i,x);}p.processBlock(buffer,midi);for(int i=0;i<n;++i)if(offset+i>=latency){actual.push(buffer.getSample(0,i),buffer.getSample(1,i));peak=std::max(peak,std::abs(buffer.getSample(0,i)));}}
 check(std::abs(actual.read().integrated-preview.lufs)<.02f&&std::abs(peak-preview.peak)<.002f,"preview differs from actual Processor");
 check(!masterlab::renderBrainPreview(testLeft,testRight,48000,settings,[]{return true;}).complete,"preview ignored cancellation");buffer.setSize(2,256);
 p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);feed(true);auto silence=wait(p,masterlab::BrainPhase::Ready);check(!silence.proposal.valid&&!p.applyBrainUI()&&values(p)==before,"silence generated or applied a proposal");
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
 p.releaseResources();std::cout<<"Passed: real audio capture, exact duration, no callback C++ allocations, independent worker measurements, no automatic changes, revision, stale protection, single Undo, silence, cancel and overflow\n";
}
