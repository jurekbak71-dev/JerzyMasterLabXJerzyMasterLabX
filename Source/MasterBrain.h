#pragma once
#include <juce_dsp/juce_dsp.h>
#include "BrainRules.h"
#include "LoudnessMeter.h"
#include <mutex>
namespace masterlab {
enum class BrainPhase { Idle, Pending, Analysing, Ready, Failed, Cancelled, Applied };
struct BrainSnapshot {BrainPhase phase=BrainPhase::Idle;BrainConfig config;BrainFeatures features;BrainProposal proposal;double progress=0;std::string message="Play the loudest section, then analyse. No parameters change until Apply.";};
class MasterBrain final : private juce::Thread {
 struct Packet {std::array<float,128> left{},right{};std::uint64_t generation=0,firstSample=0;int length=0;};
 struct Job {
  std::uint64_t generation=0,count=0,target=0;int order=13,size=8192,head=0,hop=0,fftFrames=0;
  double ll=0,rr=0,lr=0;float peak=0;std::uint64_t clips=0;
  BrainConfig config;BrainFeatures features;LoudnessMeter loudness;
  std::unique_ptr<juce::dsp::FFT> fft;
  std::array<std::array<float,32768>,2> samples{};
  std::array<float,65536> left{},right{};
  std::array<double,5> energy{};
  Job(double rate,BrainConfig cfg,std::uint64_t gen):generation(gen),config(cfg){
   features.rate=rate;target=static_cast<std::uint64_t>(std::ceil(rate*cfg.seconds));order=std::clamp(static_cast<int>(std::ceil(std::log2(rate/6))),12,15);size=1<<order;
   fft=std::make_unique<juce::dsp::FFT>(order);loudness.prepare(rate);
  }
  void spectrum(){
   for(int i=0;i<size;++i){const float window=.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/(size-1));const int index=(head+size+i)%size;left[i]=samples[0][index]*window;right[i]=samples[1][index]*window;}
   std::fill(left.begin()+size,left.begin()+size*2,0);std::fill(right.begin()+size,right.begin()+size*2,0);
   fft->performFrequencyOnlyForwardTransform(left.data());fft->performFrequencyOnlyForwardTransform(right.data());
   constexpr std::array<double,6> edges{20,80,250,2000,6000,20000};const double binWidth=features.rate/size;
   for(int i=1;i<size/2;++i){double low=(i-.5)*binWidth,high=(i+.5)*binWidth,power=static_cast<double>(left[i])*left[i]+static_cast<double>(right[i])*right[i];
    for(size_t band=0;band<5;++band){double overlap=std::max(0.,std::min(high,edges[band+1])-std::max(low,edges[band]));energy[band]+=power*overlap/binWidth;}
   }++fftFrames;
  }
  void process(const Packet& packet){
   for(int i=0;i<packet.length&&count<target;++i){float l=packet.left[i],r=packet.right[i];loudness.push(l,r);ll+=static_cast<double>(l)*l;rr+=static_cast<double>(r)*r;lr+=static_cast<double>(l)*r;peak=std::max(peak,std::max(std::abs(l),std::abs(r)));clips+=std::max(std::abs(l),std::abs(r))>1?1:0;
    samples[0][head]=l;samples[1][head]=r;head=(head+1)%size;++count;++hop;if(count>=static_cast<std::uint64_t>(size)&&hop>=size/2){spectrum();hop=0;}
   }
  }
  BrainFeatures finish(){features.seconds=count/features.rate;features.integrated=loudness.read().integrated;features.peak=peak;features.rms=count?static_cast<float>(std::sqrt((ll+rr)/(2*count))):0;features.correlation=ll*rr>1e-24?static_cast<float>(std::clamp(lr/std::sqrt(ll*rr),-1.,1.)):0;features.clips=clips;features.energy=energy;return features;}
 };
 juce::AbstractFifo fifo;std::vector<Packet> packets;
 std::mutex mutex;std::unique_ptr<Job> job;BrainSnapshot snapshot;
 std::atomic<std::uint64_t> active{0},dropped{0},cancelled{0},nextGeneration{0},targetSamples{0};
 std::uint64_t audioGeneration=0,audioCount=0;double rate=48000;const int capacity;
 void fail(const std::string& message){snapshot.phase=BrainPhase::Failed;snapshot.message=message;snapshot.proposal={};if(job){auto expected=job->generation;active.compare_exchange_strong(expected,0);}}
 void run()override{
  while(!threadShouldExit()){
   {
    std::scoped_lock lock(mutex);
    if(job&&cancelled.load()==job->generation&&(snapshot.phase==BrainPhase::Pending||snapshot.phase==BrainPhase::Analysing)){snapshot.phase=BrainPhase::Cancelled;snapshot.message="Analysis cancelled. No parameters changed.";auto expected=job->generation;active.compare_exchange_strong(expected,0);}
    if(job&&dropped.load()==job->generation&&snapshot.phase==BrainPhase::Analysing)fail("Analysis queue overflow. Retry during realtime playback; no partial result is used.");
    int start,n,start2,n2;fifo.prepareToRead(fifo.getNumReady(),start,n,start2,n2);
    auto consume=[&](int first,int length){for(int i=0;i<length;++i){const auto& packet=packets[static_cast<size_t>(first+i)];
     if(!job||snapshot.phase!=BrainPhase::Analysing||packet.generation!=job->generation)continue;
     if(packet.firstSample!=job->count){fail("Audio continuity was lost. Reanalyse the fragment.");continue;}
     job->process(packet);snapshot.progress=static_cast<double>(job->count)/job->target;
     if(job->count==job->target){snapshot.features=job->finish();snapshot.proposal=proposeMastering(snapshot.features,snapshot.config);snapshot.phase=BrainPhase::Ready;snapshot.message=snapshot.proposal.valid?"Analysis complete. Review every proposed change before applying.":((!snapshot.config.useEq&&!snapshot.config.useLimiter)?"No processing modules selected. Analysis is available; Apply is disabled.":"Insufficient material. No proposal can be applied.");}
    }};consume(start,n);consume(start2,n2);fifo.finishedRead(n+n2);
    if(job&&snapshot.phase==BrainPhase::Pending){snapshot.phase=BrainPhase::Analysing;snapshot.message="Listening to input before processing. Play audio in the DAW.";active.store(job->generation);}
   }
   wait(10);
  }
 }
public:
 explicit MasterBrain(int packetCapacity=1024):Thread("MasterLab analysis"),fifo(std::max(2,packetCapacity)),capacity(std::max(2,packetCapacity)){}
 ~MasterBrain()override{active.store(0);stopThread(3000);}
 void prepare(double sampleRate){
  active.store(0);{
   std::scoped_lock lock(mutex);rate=std::isfinite(sampleRate)&&sampleRate>=8000&&sampleRate<=384000?sampleRate:48000;
   packets.resize(static_cast<size_t>(capacity));fifo.reset();job.reset();snapshot.phase=BrainPhase::Idle;snapshot.progress=0;snapshot.proposal={};snapshot.message="Play the loudest section, then analyse. No parameters change until Apply.";audioGeneration=audioCount=0;
  }if(!isThreadRunning())startThread();
 }
 void startUI(BrainConfig config){
  std::scoped_lock lock(mutex);active.store(0);const auto gen=nextGeneration.fetch_add(1)+1;config=cleanBrainConfig(config);
  job=std::make_unique<Job>(rate,config,gen);snapshot={};snapshot.phase=BrainPhase::Pending;snapshot.config=config;snapshot.message="Waiting for the analysis worker and DAW audio.";targetSamples.store(job->target);
 }
 void cancelRT()noexcept {active.store(0);cancelled.store(nextGeneration.load());}
 void cancelUI(){std::scoped_lock lock(mutex);active.store(0);snapshot.phase=BrainPhase::Cancelled;snapshot.proposal={};snapshot.message="Analysis cancelled. No parameters changed.";}
 void reviseUI(BrainConfig config){
  std::scoped_lock lock(mutex);if(snapshot.phase==BrainPhase::Pending||snapshot.phase==BrainPhase::Analysing)return;
  config=cleanBrainConfig(config);config.existingEq=snapshot.config.existingEq;config.existingEqEnabled=snapshot.config.existingEqEnabled;
  const bool durationChanged=config.seconds!=snapshot.config.seconds;snapshot.config=config;
  if(job&&snapshot.phase==BrainPhase::Ready&&!durationChanged){snapshot.proposal=proposeMastering(snapshot.features,config);snapshot.message="Proposal recalculated from the captured fragment. Review before Apply.";}
  else {snapshot.phase=BrainPhase::Idle;snapshot.progress=0;snapshot.proposal={};snapshot.message="Settings changed. Analyse a new fragment.";}
 }
 BrainSnapshot readUI(){std::scoped_lock lock(mutex);return snapshot;}
 void markAppliedUI(){std::scoped_lock lock(mutex);snapshot.phase=BrainPhase::Applied;snapshot.message="Applied as one Undo transaction. Measure and audition the full processed song.";}
 // Single audio producer. No locks, wake-up signals, allocation or FFT here.
 void capture(const float* left,const float* right,int length)noexcept {
  const auto gen=active.load();if(gen==0)return;if(audioGeneration!=gen){audioGeneration=gen;audioCount=0;}
  const auto target=targetSamples.load();
  for(int offset=0;offset<length&&audioCount<target;){
   int first,n,first2,n2;fifo.prepareToWrite(1,first,n,first2,n2);
   if(n+n2==0){dropped.store(gen);auto expected=gen;active.compare_exchange_strong(expected,0);return;}
   auto& packet=packets[static_cast<size_t>(n?first:first2)];packet.generation=gen;packet.firstSample=audioCount;packet.length=static_cast<int>(std::min<std::uint64_t>(std::min(128,length-offset),target-audioCount));
   for(int i=0;i<packet.length;++i){packet.left[i]=clean(left[offset+i]);packet.right[i]=clean(right[offset+i]);}audioCount+=packet.length;offset+=packet.length;fifo.finishedWrite(n+n2);
  }
  if(audioCount>=target){auto expected=gen;active.compare_exchange_strong(expected,0);}
 }
};
}
