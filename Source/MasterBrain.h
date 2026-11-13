#pragma once
#include <juce_dsp/juce_dsp.h>
#include "BrainRules.h"
#include "BrainPreview.h"
#include "LoudnessMeter.h"
#include "ModularDsp.h"
#include <mutex>
namespace masterlab {
enum class BrainPhase { Idle, Pending, Analysing, Validating, Ready, Failed, Cancelled, Applied };
struct BrainResourceReading {std::size_t heavyBytes=0;bool working=false;};
struct BrainSnapshot {
 BrainPhase phase=BrainPhase::Idle;BrainConfig config;BrainFeatures features;
 BrainProposal proposal,alternateProposal;int selectedVariant=0;bool alternateReady=false;
 double progress=0;std::string message="Play the track from the beginning, then analyse. No parameters change until Apply.";
};
class MasterBrain final : private juce::Thread {
 struct Packet {std::array<float,128> left{},right{};std::uint64_t generation=0,firstSample=0;int length=0;};
 struct Job {
  std::uint64_t generation=0,count=0,target=0;int order=13,size=8192,head=0,hop=0,fftFrames=0;
  Split bandLeft,bandRight;std::array<float,3> splitCoefficients{};std::array<double,4> bandLL{},bandRR{},bandLR{};
  double ll=0,rr=0,lr=0;float peak=0;std::uint64_t clips=0;
  BrainConfig config;BrainFeatures features;LoudnessMeter loudness;BrainDetailAnalyzer detail;std::vector<float> previewLeft,previewRight;
  std::unique_ptr<juce::dsp::FFT> fft;
  std::array<std::array<float,32768>,2> samples{};
  std::array<float,65536> left{},right{};
  std::array<double,5> energy{};
  Job(double rate,BrainConfig cfg,std::uint64_t gen):generation(gen),config(cfg){
   splitCoefficients={static_cast<float>(1-std::exp(-6.283185307179586*140/rate)),static_cast<float>(1-std::exp(-6.283185307179586*2500/rate)),static_cast<float>(1-std::exp(-6.283185307179586*8000/rate))};
   features.rate=rate;target=static_cast<std::uint64_t>(std::ceil(rate*cfg.seconds));order=std::clamp(static_cast<int>(std::ceil(std::log2(rate/6))),12,15);size=1<<order;
   fft=std::make_unique<juce::dsp::FFT>(order);loudness.prepare(rate);detail.prepare(rate,cfg.seconds);const size_t previewCount=static_cast<size_t>(std::ceil(rate*std::min(16,cfg.seconds)));previewLeft.resize(previewCount);previewRight.resize(previewCount);
  }
  void spectrum(){
   for(int i=0;i<size;++i){const float window=.5f-.5f*std::cos(juce::MathConstants<float>::twoPi*i/(size-1));const int index=(head+size+i)%size;left[i]=samples[0][index]*window;right[i]=samples[1][index]*window;}
   std::fill(left.begin()+size,left.begin()+size*2,0);std::fill(right.begin()+size,right.begin()+size*2,0);
   fft->performFrequencyOnlyForwardTransform(left.data());fft->performFrequencyOnlyForwardTransform(right.data());detail.spectrum(left.data(),right.data(),size);
   constexpr std::array<double,6> edges{20,80,250,2000,6000,20000};const double binWidth=features.rate/size;
   for(int i=1;i<size/2;++i){double low=(i-.5)*binWidth,high=(i+.5)*binWidth,power=static_cast<double>(left[i])*left[i]+static_cast<double>(right[i])*right[i];
    for(size_t band=0;band<5;++band){double overlap=std::max(0.,std::min(high,edges[band+1])-std::max(low,edges[band]));energy[band]+=power*overlap/binWidth;}
   }++fftFrames;
  }
  void process(const Packet& packet){
   for(int i=0;i<packet.length&&count<target;++i){float l=packet.left[i],r=packet.right[i];detail.push(l,r);// A representative DSP audition spans the full analysis, rather than using
    // only the first 16 seconds. Four 4-second windows cover start/middle/end.
    if(!previewLeft.empty()){
     const std::uint64_t N=previewLeft.size();
     if(target<=N){
      if(count<N){previewLeft[static_cast<size_t>(count)]=l;previewRight[static_cast<size_t>(count)]=r;}
     }else{
      const std::uint64_t part=N/4;
      for(std::uint64_t region=0;region<4;++region){
       const std::uint64_t start=(target-part)*region/3;
       if(count>=start&&count<start+part){
        size_t dst=static_cast<size_t>(region*part+count-start);
        previewLeft[dst]=l;previewRight[dst]=r;break;
       }
      }
     }
    }
    const float a=splitCoefficients[0],b=splitCoefficients[1],c=splitCoefficients[2];
    auto leftBands=bandLeft.tick(l,a,b,c),rightBands=bandRight.tick(r,a,b,c);for(size_t k=0;k<4;++k){bandLL[k]+=static_cast<double>(leftBands[k])*leftBands[k];bandRR[k]+=static_cast<double>(rightBands[k])*rightBands[k];bandLR[k]+=static_cast<double>(leftBands[k])*rightBands[k];}
    loudness.push(l,r);ll+=static_cast<double>(l)*l;rr+=static_cast<double>(r)*r;lr+=static_cast<double>(l)*r;peak=std::max(peak,std::max(std::abs(l),std::abs(r)));clips+=std::max(std::abs(l),std::abs(r))>1?1:0;
    samples[0][head]=l;samples[1][head]=r;head=(head+1)%size;++count;++hop;if(count>=static_cast<std::uint64_t>(size)&&hop>=size/2){spectrum();hop=0;}
   }
  }
  BrainFeatures finish(){for(size_t k=0;k<4;++k){features.bandCorrelation[k]=bandLL[k]*bandRR[k]>1e-24?static_cast<float>(std::clamp(bandLR[k]/std::sqrt(bandLL[k]*bandRR[k]),-1.,1.)):0;features.bandWidth[k]=static_cast<float>(std::sqrt(std::max(0.,bandLL[k]+bandRR[k]-2*bandLR[k])/std::max(1e-20,bandLL[k]+bandRR[k]+2*bandLR[k])));}features.detail=detail.finish();features.stereoAnalysed=true;features.seconds=count/features.rate;features.integrated=loudness.read().integrated;features.peak=peak;features.rms=count?static_cast<float>(std::sqrt((ll+rr)/(2*count))):0;features.correlation=ll*rr>1e-24?static_cast<float>(std::clamp(lr/std::sqrt(ll*rr),-1.,1.)):0;features.clips=clips;features.energy=energy;return features;}
 };
 juce::AbstractFifo fifo;std::vector<Packet> packets;
 std::mutex mutex;std::unique_ptr<Job> job;BrainSnapshot snapshot;
 std::atomic<std::uint64_t> active{0},dropped{0},cancelled{0},nextGeneration{0},targetSamples{0};
 std::uint64_t audioGeneration=0,audioCount=0;double rate=48000;const int capacity;
 void releasePacketsLocked(){std::vector<Packet>{}.swap(packets);fifo.reset();}
 void fail(const std::string& message){snapshot.phase=BrainPhase::Failed;snapshot.message=message;snapshot.proposal={};if(job){auto expected=job->generation;active.compare_exchange_strong(expected,0);}}
 void run()override{
  while(!threadShouldExit()){
   std::unique_ptr<Job> verification;
   {
    std::scoped_lock lock(mutex);
    if(job&&cancelled.load()==job->generation&&(snapshot.phase==BrainPhase::Pending||snapshot.phase==BrainPhase::Analysing)){snapshot.phase=BrainPhase::Cancelled;snapshot.message="Analysis cancelled. No parameters changed.";auto expected=job->generation;active.compare_exchange_strong(expected,0);job.reset();releasePacketsLocked();}
    if(job&&dropped.load()==job->generation&&snapshot.phase==BrainPhase::Analysing){fail("Analysis queue overflow. Retry during realtime playback; no partial result is used.");job.reset();releasePacketsLocked();}
    int start,n,start2,n2;fifo.prepareToRead(fifo.getNumReady(),start,n,start2,n2);
    auto consume=[&](int first,int length){for(int i=0;i<length;++i){const auto& packet=packets[static_cast<size_t>(first+i)];
     if(!job||snapshot.phase!=BrainPhase::Analysing||packet.generation!=job->generation)continue;
     if(packet.firstSample!=job->count){fail("Audio continuity was lost. Reanalyse the fragment.");continue;}
     job->process(packet);snapshot.progress=static_cast<double>(job->count)/job->target;
     if(job->count==job->target){snapshot.features=job->finish();snapshot.proposal=proposeMastering(snapshot.features,snapshot.config);snapshot.phase=snapshot.proposal.valid?BrainPhase::Validating:BrainPhase::Ready;snapshot.message=snapshot.proposal.valid?"Verifying the proposal through the actual DSP chain.":"Insufficient material or no modules selected. Apply is disabled.";}
    }};consume(start,n);consume(start2,n2);fifo.finishedRead(n+n2);
    if(job&&snapshot.phase==BrainPhase::Pending){snapshot.phase=BrainPhase::Analysing;snapshot.message="Listening to input before processing. Play audio in the DAW.";active.store(job->generation);}
    if(job&&snapshot.phase==BrainPhase::Validating)verification=std::move(job);
   }
   if(verification){
    auto proposal=proposeMastering(verification->features,verification->config);
    // Two independently verified options from exactly the same captured audio.
    // B is an alternate mastering character, not a copy of the A controls.
    BrainConfig alternativeConfig=verification->config;
    alternativeConfig.style=verification->config.style==2?0:2;
    alternativeConfig.intensity=std::clamp(verification->config.intensity*
                                          (alternativeConfig.style==2?1.15f:.8f),.25f,1.5f);
    auto alternative=proposeMastering(verification->features,alternativeConfig);
    const auto generation=verification->generation;
    auto abort=[&]{return threadShouldExit()||nextGeneration.load()!=generation||cancelled.load()==generation;};
    try{
     verifyBrainProposal(proposal,verification->features,verification->config,
                         verification->previewLeft,verification->previewRight,abort);
     if(!abort())verifyBrainProposal(alternative,verification->features,alternativeConfig,
                                   verification->previewLeft,verification->previewRight,abort);
    }
    catch(const std::exception&){proposal.valid=false;proposal.previewVerified=false;alternative.valid=false;
     proposal.warnings.push_back("DSP verification failed. No parameters changed; Apply disabled.");}
    std::scoped_lock lock(mutex);
    if(cancelled.load()==generation&&nextGeneration.load()==generation&&snapshot.phase==BrainPhase::Validating){snapshot.phase=BrainPhase::Cancelled;snapshot.proposal={};snapshot.message="Verification cancelled. No parameters changed.";}
    if(!abort()&&snapshot.phase==BrainPhase::Validating){snapshot.proposal=std::move(proposal);
      snapshot.alternateReady=alternative.valid&&alternative.previewVerified;
      snapshot.alternateProposal=std::move(alternative);snapshot.selectedVariant=0;
      snapshot.phase=BrainPhase::Ready;
      snapshot.message=snapshot.proposal.previewVerified?
        "A/B proposals verified through actual DSP. Audition A or B, then Apply.":
        "Analysis complete. Review before Apply.";
      job.reset();releasePacketsLocked();}
   }
   wait(10);
  }
 }
public:
 explicit MasterBrain(int packetCapacity=1024):Thread("MasterLab analysis"),fifo(std::max(2,packetCapacity)),capacity(std::max(2,packetCapacity)){}
 ~MasterBrain()override{active.store(0);stopThread(3000);}
 void prepare(double sampleRate){
  active.store(0);nextGeneration.fetch_add(1);{
   std::scoped_lock lock(mutex);rate=std::isfinite(sampleRate)&&sampleRate>=8000&&sampleRate<=384000?sampleRate:48000;
   releasePacketsLocked();job.reset();snapshot.phase=BrainPhase::Idle;snapshot.progress=0;snapshot.proposal={};snapshot.message="Play up to 20 seconds of a representative loud section. No parameters change before Apply.";audioGeneration=audioCount=0;
  }if(!isThreadRunning())startThread();
 }
 void startUI(BrainConfig config){
  std::scoped_lock lock(mutex);active.store(0);const auto gen=nextGeneration.fetch_add(1)+1;config=cleanBrainConfig(config);
  packets.resize(static_cast<size_t>(capacity));fifo.reset();
  job=std::make_unique<Job>(rate,config,gen);snapshot={};snapshot.phase=BrainPhase::Pending;snapshot.config=config;snapshot.message="Waiting for the analysis worker and DAW audio.";targetSamples.store(job->target);
 }
 void cancelRT()noexcept {active.store(0);cancelled.store(nextGeneration.load());}
 void cancelUI(){cancelRT();std::scoped_lock lock(mutex);active.store(0);job.reset();releasePacketsLocked();snapshot.phase=BrainPhase::Cancelled;snapshot.proposal={};snapshot.message="Analysis cancelled. No parameters changed.";}
 // User-initiated NEW/DEFAULT: invalidate proposals without touching the realtime
 // FIFO or requiring a host transport stop. Stored presets remain untouched.
 void clearUI(){
  cancelRT();std::scoped_lock lock(mutex);
  active.store(0);
  job.reset();releasePacketsLocked();
  snapshot=BrainSnapshot{};
  snapshot.message="Factory default loaded. Play audio and analyse to create a new proposal.";
 }

 void reviseUI(BrainConfig config){
  std::scoped_lock lock(mutex);if(snapshot.phase==BrainPhase::Pending||snapshot.phase==BrainPhase::Analysing||snapshot.phase==BrainPhase::Validating)return;
  config=cleanBrainConfig(config);config.dynamicsMode=snapshot.config.dynamicsMode;config.existingEq=snapshot.config.existingEq;config.existingEqEnabled=snapshot.config.existingEqEnabled;config.hasBaseline=snapshot.config.hasBaseline;config.baseline=snapshot.config.baseline;
  const bool durationChanged=config.seconds!=snapshot.config.seconds;snapshot.config=config;
  if(job&&snapshot.phase==BrainPhase::Ready&&!durationChanged){job->config=config;snapshot.proposal={};snapshot.phase=BrainPhase::Validating;snapshot.message="Rechecking the captured fragment through DSP.";}
  else {snapshot.phase=BrainPhase::Idle;snapshot.progress=0;snapshot.proposal={};snapshot.message="Settings changed. Analyse a new fragment.";}
 }
 BrainSnapshot readUI(){std::scoped_lock lock(mutex);return snapshot;}
 BrainResourceReading resourceReadingUI(){
  std::scoped_lock lock(mutex);BrainResourceReading out;
  out.working=snapshot.phase==BrainPhase::Pending||snapshot.phase==BrainPhase::Analysing||snapshot.phase==BrainPhase::Validating;
  if(job)out.heavyBytes=sizeof(Job)+job->previewLeft.capacity()*sizeof(float)+job->previewRight.capacity()*sizeof(float);
  out.heavyBytes+=packets.capacity()*sizeof(Packet);return out;
 }
 bool selectVariantUI(int index){
  std::scoped_lock lock(mutex);
  if(snapshot.phase!=BrainPhase::Ready||index<0||index>1)return false;
  if(index==1&&!snapshot.alternateReady)return false;
  if(index!=snapshot.selectedVariant){
   std::swap(snapshot.proposal,snapshot.alternateProposal);
   snapshot.selectedVariant=index;
  }
  snapshot.message=index==0?"Variant A selected: original style. Audition and Apply.":"Variant B selected: contrasting style. Audition and Apply.";
  return true;
 }
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
