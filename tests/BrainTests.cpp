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
 for(int i=0;i<1500;++i){auto snapshot=p.brain.readUI();if(snapshot.phase==phase)return snapshot;if(snapshot.phase==masterlab::BrainPhase::Failed){std::cerr<<snapshot.message<<'\n';std::exit(1);}std::this_thread::sleep_for(std::chrono::milliseconds(2));}check(false,"analysis timeout");return {};
}
int main(){
 Processor p;p.prepareToPlay(48000,256);auto* gain=p.state.getParameter("gain");gain->setValueNotifyingHost(gain->convertTo0to1(2));auto before=values(p);
 masterlab::BrainConfig config;p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);
 juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;int sample=0;
 auto feed=[&](bool silent){for(int block=0;block<1500;++block){for(int i=0;i<256;++i,++sample){float x=silent?0.f:.15f*std::sin(juce::MathConstants<double>::twoPi*1000*sample/48000);buffer.setSample(0,i,x);buffer.setSample(1,i,x);}audit=true;p.processBlock(buffer,midi);audit=false;}};
 feed(false);auto result=wait(p,masterlab::BrainPhase::Ready);
 check(allocations==0,"audio capture allocated C++ heap memory");check(result.proposal.valid&&std::abs(result.features.seconds-8)<1e-6&&result.progress==1,"capture result or duration incorrect");
 check(std::abs(result.features.integrated-(-16.48f))<.15f&&result.features.correlation>.999f,"captured audio measurements incorrect");check(values(p)==before,"analysis changed parameters without approval");
 auto revised=config;revised.genre=4;revised.intensity=.5f;p.brain.reviseUI(revised);check(p.brain.readUI().phase==masterlab::BrainPhase::Ready&&values(p)==before,"revision changed audio or discarded capture");
 gain->setValueNotifyingHost(gain->convertTo0to1(3));auto manual=values(p);check(p.brainStaleUI()&&!p.applyBrainUI()&&values(p)==manual,"stale proposal overwrote manual settings");gain->setValueNotifyingHost(gain->convertTo0to1(2));
 check(p.applyBrainUI()&&values(p)!=before,"approved proposal not applied");for(const char* id:{"stereoPresent","dynamicsPresent","dynamicEqPresent"})check(p.state.getRawParameterValue(id)->load()==1,"Brain omitted new module presence");check(p.undo()&&values(p)==before,"proposal is not one Undo transaction");check(!p.applyBrainUI(),"applied proposal applied twice");
 p.startBrainUI(config);wait(p,masterlab::BrainPhase::Analysing);feed(true);auto silence=wait(p,masterlab::BrainPhase::Ready);check(!silence.proposal.valid&&!p.applyBrainUI()&&values(p)==before,"silence generated or applied a proposal");
 p.startBrainUI(config);p.brain.cancelRT();for(int i=0;i<200&&p.brain.readUI().phase!=masterlab::BrainPhase::Cancelled;++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));check(p.brain.readUI().phase==masterlab::BrainPhase::Cancelled,"pending analysis did not cancel");
 // Deliberately undersized queue: data loss must invalidate the entire analysis.
 masterlab::MasterBrain tiny(2);tiny.prepare(48000);tiny.startUI(config);for(int i=0;i<200&&tiny.readUI().phase!=masterlab::BrainPhase::Analysing;++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));
 std::array<float,4096> zeros{};tiny.capture(zeros.data(),zeros.data(),4096);for(int i=0;i<200&&tiny.readUI().phase!=masterlab::BrainPhase::Failed;++i)std::this_thread::sleep_for(std::chrono::milliseconds(2));check(tiny.readUI().phase==masterlab::BrainPhase::Failed,"queue overflow produced a usable partial result");
 p.releaseResources();std::cout<<"Passed: real audio capture, exact duration, no callback C++ allocations, independent worker measurements, no automatic changes, revision, stale protection, single Undo, silence, cancel and overflow\n";
}
