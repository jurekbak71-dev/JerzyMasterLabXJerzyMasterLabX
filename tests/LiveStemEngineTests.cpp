#include "LiveStemEngine.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>

static thread_local std::size_t allocations = 0;
void* operator new(std::size_t n) { ++allocations; if (auto* p=std::malloc(n?n:1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void operator delete[](void* p,std::size_t) noexcept { std::free(p); }
#define CHECK(x) do { if(!(x)){std::cerr<<"FAIL "<<#x<<" line "<<__LINE__<<'\n';std::abort();} } while(false)
using namespace masterlab;

// Removing the dry delay or advancing it by blocks instead of samples breaks this oracle.
static void variableBlocksAndMono() {
 LiveStemEngine e; CHECK(e.prepare(48000,3,4));
 std::array<float,12> l{1,2,3,4,5,6,7,8,9,10,11,12}, r=l;
 std::size_t at=0;
 for(auto n:{1u,0u,2u,7u,2u}) {
  e.process(l.data()+at,r.data()+at,n,{static_cast<std::int64_t>(at),true,false,true}); at+=n;
 }
 for(std::size_t i=0;i<12;++i) CHECK(l[i]==(i<3?0.f:float(i-2)) && r[i]==l[i]);
 StereoLiveFrame f{};
 for(std::uint64_t i=0;i<12;++i){CHECK(e.popInput(f)); CHECK(f.left==float(i+1)&&f.position==i);}
 CHECK(!e.popInput(f));
 e.reset(); float mono[]{.1f,.2f,.3f,.4f};
 e.process(mono,nullptr,4,{0,true,false,true});
 CHECK(mono[0]==0 && mono[3]==.1f);
 CHECK(e.popInput(f) && f.left==.1f && f.right==.1f);
}

// A queued result from before a seek/loop or stop must never appear in a new epoch.
static void transportAndStaleResults() {
 LiveStemEngine e; CHECK(e.prepare(44100,2,4));
 float l[]{.25f,.25f},r[]{.25f,.25f};
 e.process(l,r,2,{-2,true,false,true});
 StereoLiveFrame old{},discard{};CHECK(e.popInput(old));CHECK(e.popInput(discard));
 SeparatedLiveFrame stale{}; stale.epoch=old.epoch;stale.position=old.position;stale.channels[0]=10;
 CHECK(e.pushOutput(stale));
 l[0]=l[1]=r[0]=r[1]=.5f;
 e.process(l,r,2,{100,true,false,true}); CHECK(l[0]==0 && l[1]==0);
 StereoLiveFrame next{}; CHECK(e.popInput(next));CHECK(e.popInput(next));CHECK(next.epoch!=old.epoch);
 const auto seekEpoch=next.epoch;
 l[0]=r[0]=.3f; e.process(l,r,1,{102,false,false,true}); CHECK(l[0]==0);
 while(e.popInput(next)) CHECK(false); // stopped input is monitored dry, never submitted to inference
 l[0]=r[0]=.4f; e.process(l,r,1,{102,true,false,true}); CHECK(e.popInput(next)); CHECK(next.epoch!=seekEpoch);
 l[0]=r[0]=.2f; e.process(l,r,1,{0,false,true,true}); CHECK(e.popInput(next)); // offline render can have isPlaying=false
}

// Starvation must fade the last correction, not jump to dry or replay an old timestamp.
static void alignedOutputAndStarvation() {
 LiveStemEngine e;CHECK(e.prepare(48000,2,4));
 float l[2]{.25f,.25f},r[2]{.25f,.25f};e.process(l,r,2,{0,true,false,true});
 StereoLiveFrame input{};
 for(int i=0;i<2;++i){CHECK(e.popInput(input));SeparatedLiveFrame out{};out.position=input.position;out.epoch=input.epoch;out.channels[0]=.5f;out.channels[1]=.5f;CHECK(e.pushOutput(out));}
 float a=.25f,b=.25f; e.process(&a,&b,1,{2,true,false,true});CHECK(std::abs(a-.3125f)<1e-6f);
 a=b=.25f;e.process(&a,&b,1,{3,true,false,true});CHECK(std::abs(a-.375f)<1e-6f);
 const float expected[]{.34375f,.3125f,.28125f,.25f,.25f};
 for(int i=0;i<5;++i){a=b=.25f;e.process(&a,&b,1,{4+i,true,false,true});CHECK(std::abs(a-expected[i])<1e-6f);CHECK(a==b);}
 CHECK(e.statistics().missingFrames>=5);
}

static void silenceInvalidAndBoundedQueues() {
 LiveStemEngine e;CHECK(!e.prepare(0,2,4));CHECK(e.prepare(96000,2,4));
 float l[2]{},r[2]{};e.process(l,r,2,{0,true,false,true});
 StereoLiveFrame in{}; CHECK(e.popInput(in));
 SeparatedLiveFrame bad{};bad.epoch=in.epoch;bad.position=in.position;bad.channels.fill(std::numeric_limits<float>::quiet_NaN());CHECK(e.pushOutput(bad));
 float a=0,b=0;e.process(&a,&b,1,{2,true,false,true});CHECK(a==0&&b==0);
 CHECK(e.statistics().invalidFrames==1);
 CHECK(e.popInput(in));SeparatedLiveFrame hallucination{};hallucination.epoch=in.epoch;hallucination.position=in.position;hallucination.channels[0]=.5f;CHECK(e.pushOutput(hallucination));
 e.process(&a,&b,1,{3,true,false,true});CHECK(a==0&&b==0);
 for(std::size_t i=0;i<LiveStemEngine::queueCapacity+10;++i){a=b=.1f;e.process(&a,&b,1,{0,true,false,false});CHECK(std::isfinite(a));}
 CHECK(e.statistics().droppedInput>0);
}

static void noCallbackAllocationsAndReset() {
 LiveStemEngine e;CHECK(e.prepare(48000,1024,240));
 const auto before=allocations;
 std::array<float,257> l{},r{};
 for(int block=0;block<300;++block){l.fill(.2f);r.fill(-.2f);e.process(l.data(),r.data(),l.size(),{block*257,true,false,true});if(block==100)e.reset();}
 e.process(nullptr,nullptr,0,{});
 CHECK(allocations==before);
}
// Resuming inference halfway through dry fallback must blend from the audible
// correction, rather than restarting wet gain at zero and causing a level jump.
static void recoveryDuringFade() {
 LiveStemEngine e;CHECK(e.prepare(48000,2,4));
 float observed[8]{};
 for(int i=0;i<8;++i){
  float l=.25f,r=.25f;e.process(&l,&r,1,{i,true,false,true});observed[i]=l;
  StereoLiveFrame input{};CHECK(e.popInput(input));
  if(i!=4){SeparatedLiveFrame out{};out.position=input.position;out.epoch=input.epoch;out.channels[0]=out.channels[1]=.5f;CHECK(e.pushOutput(out));}
 }
 CHECK(std::abs(observed[5]-.5f)<1e-6f);
 CHECK(std::abs(observed[6]-.4375f)<1e-6f);
 CHECK(std::abs(observed[7]-.453125f)<1e-6f);
}
static void concurrentWorkerAcrossSeeks() {
 LiveStemEngine e;CHECK(e.prepare(48000,32,16));
 std::atomic<bool> done{false};
 // Identity worker is a boundary test fixture, never a separator/model.
 std::thread worker([&]{StereoLiveFrame in{};while(!done.load()){
  if(!e.popInput(in)){std::this_thread::yield();continue;}
  SeparatedLiveFrame out{};out.position=in.position;out.epoch=in.epoch;
  out.channels[0]=in.left;out.channels[1]=in.right;
  e.pushOutput(out);
 }});
 std::array<float,257> l{},r{};
 for(int block=0;block<1000;++block){
  const bool seek=block%7==0;
  if(seek)e.reset();
  l.fill(.25f);r.fill(-.25f);
  e.process(l.data(),r.data(),257,{0,true,false,false});
  for(std::size_t i=0;i<257;++i){const float expected=seek&&i<32?0.f:.25f;CHECK(l[i]==expected&&r[i]==-expected);}
 }
 done.store(true);worker.join();
}
static void finiteFallbackAtExtremeAmplitude() {
 LiveStemEngine e;CHECK(e.prepare(48000,1,2));
 const float maximum=std::numeric_limits<float>::max();
 for(int i=0;i<6;++i){
  float l=i<2?-maximum/4:maximum,r=l;
  e.process(&l,&r,1,{i,true,false,true});CHECK(std::isfinite(l)&&std::isfinite(r));
  StereoLiveFrame in{};CHECK(e.popInput(in));
  if(i<2){SeparatedLiveFrame out{};out.position=in.position;out.epoch=in.epoch;out.channels[0]=out.channels[1]=maximum/4;CHECK(e.pushOutput(out));}
 }
}
// Regression: do not count model output as delivered WET when subtracting
// the delayed dry sample overflows float and it must be rejected.
static void invalidWetCorrectionDoesNotCountAsDelivered() {
 LiveStemEngine e;CHECK(e.prepare(48000,1,2));
 const float max=std::numeric_limits<float>::max();
 float l=max,r=max;
 e.process(&l,&r,1,{0,true,false,true});
 StereoLiveFrame input{};CHECK(e.popInput(input));
 SeparatedLiveFrame output{};
 output.epoch=input.epoch;output.position=input.position;
 output.channels[0]=-max;output.channels[1]=-max;
 CHECK(e.pushOutput(output));
 l=r=max;
 e.process(&l,&r,1,{1,true,false,true});
 CHECK(std::isfinite(l)&&std::isfinite(r));
 CHECK(e.statistics().deliveredWetFrames==0);
}
static void impossibleFutureDoesNotHideValidOutput() {
 LiveStemEngine e;CHECK(e.prepare(48000,1,1));
 float l=.25f,r=.25f;e.process(&l,&r,1,{0,true,false,true});
 StereoLiveFrame in{};CHECK(e.popInput(in));
 SeparatedLiveFrame out{};out.epoch=in.epoch;out.position=std::numeric_limits<std::uint64_t>::max();
 CHECK(e.pushOutput(out));out.position=in.position;out.channels[0]=out.channels[1]=.5f;CHECK(e.pushOutput(out));
 l=r=.25f;e.process(&l,&r,1,{1,true,false,true});CHECK(l==.5f&&r==.5f);CHECK(e.statistics().invalidFrames==1);
}
static float runFourStemMixer(const LiveStemControls& controls){
 LiveStemEngine e; CHECK(e.prepare(44100,1,1));
 float value=0.f;
 for(int i=0;i<5000;++i){
  float l=1.f,r=1.f;
  e.process(&l,&r,1,{i,true,false,true},controls);
  value=l;
  StereoLiveFrame in{};
  CHECK(e.popInput(in));
  SeparatedLiveFrame out{};
  out.position=in.position;out.epoch=in.epoch;
  for(std::size_t s=0;s<4;++s){out.channels[s*2]=.25f;out.channels[s*2+1]=.25f;}
  CHECK(e.pushOutput(out));
 }
 CHECK(e.statistics().deliveredWetFrames>1000);
 return value;
}
static void independentStemMixerControls(){
 LiveStemControls controls{};
 CHECK(std::abs(runFourStemMixer(controls)-1.f)<.005f);
 controls.mute[3]=true; // mute VOCALS only
 CHECK(std::abs(runFourStemMixer(controls)-.75f)<.005f);
 controls.mute[3]=false;
 controls.solo[1]=true; // only BASS
 CHECK(std::abs(runFourStemMixer(controls)-.25f)<.005f);
 controls.solo[1]=false;
 controls.gainLinear[0]=2.f; // DRUMS +6.02 dB
 CHECK(std::abs(runFourStemMixer(controls)-1.25f)<.005f);
 controls.gainLinear[0]=1.f;
 controls.tone[0]=1.f; // presence up and low down, tonal balance shifts smoothly
 CHECK(runFourStemMixer(controls)<.95f);
 controls.tone[0]=-1.f; // low up and presence down
 CHECK(runFourStemMixer(controls)>1.08f);
 controls.tone[0]=0.f;
 controls.width[0]=0.f; // midside centre remains unchanged for mono
 CHECK(std::abs(runFourStemMixer(controls)-1.f)<.005f);
}
// Regression: negative DAW preroll must not be mistaken for a seek after
// contiguous blocks. The previous unsigned conversion wrapped on negatives.
static void negativePrerollContinuity(){
 LiveStemEngine e;CHECK(e.prepare(48000,1,1));
 float l=.25f,r=.25f;
 e.process(&l,&r,1,{-2,true,false,true});
 StereoLiveFrame first{};CHECK(e.popInput(first));
 l=r=.25f;e.process(&l,&r,1,{-1,true,false,true});
 StereoLiveFrame second{};CHECK(e.popInput(second));
 CHECK(first.epoch==second.epoch);
 CHECK(second.position==first.position+1);
 l=r=.25f;e.process(&l,&r,1,{0,true,false,true});
 StereoLiveFrame third{};CHECK(e.popInput(third));
 CHECK(second.epoch==third.epoch);
 CHECK(third.position==second.position+1);
 // Also exercise the minimum representable host position without signed overflow.
 e.reset();l=r=.25f;
 e.process(&l,&r,1,{std::numeric_limits<std::int64_t>::min(),true,false,true});
 CHECK(e.popInput(first));
 l=r=.25f;
 e.process(&l,&r,1,{std::numeric_limits<std::int64_t>::min()+1,true,false,true});
 CHECK(e.popInput(second));
 CHECK(first.epoch==second.epoch);
}
int main(){invalidWetCorrectionDoesNotCountAsDelivered();negativePrerollContinuity();variableBlocksAndMono();transportAndStaleResults();alignedOutputAndStarvation();silenceInvalidAndBoundedQueues();noCallbackAllocationsAndReset();recoveryDuringFade();concurrentWorkerAcrossSeeks();finiteFallbackAtExtremeAmplitude();impossibleFutureDoesNotHideValidOutput();independentStemMixerControls();std::cout<<"LiveStemEngineTests OK\n";}
