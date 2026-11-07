#include "LiveStemWorker.h"
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(false)
using namespace masterlab;
struct Fixture final:LiveStemBackend {
 std::thread::id audio=std::this_thread::get_id();bool primed{};LiveStemHop previous{};
 void reset()override{CHECK(std::this_thread::get_id()!=audio);primed=false;previous={};}
 bool process(const LiveStemHop& input,SeparatedStemHop& output)override{
  CHECK(std::this_thread::get_id()!=audio);output={};
  for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<128;++i)output[0][c][i]=previous[c][i]*2;
  previous=input;
  const auto valid=primed;primed=true;return valid;
 }
};
static void waitFor(LiveStemWorker& w,std::uint64_t hops,bool allowFailures=false){
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 while(w.statistics().hops<hops&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 CHECK(w.statistics().hops>=hops);if(!allowFailures)CHECK(w.statistics().failures==0);
}
static void varyingSamples(){
 LiveStemEngine engine;CHECK(engine.prepare(44100,512,1));
 LiveStemWorker worker(engine,std::make_unique<Fixture>());
 std::array<float,256> l{},r{};
 for(int block=0;block<3;++block){
  for(int i=0;i<256;++i)l[i]=r[i]=.1f+float(block*256+i)*.0001f;
  engine.process(l.data(),r.data(),256,{block*256,true,false,true});
  waitFor(worker,(block+1)*2);
  for(int i=0;i<256;++i)CHECK(l[i]==(block<2?0.f:2*(.1f+float(block*256+i-512)*.0001f))&&r[i]==l[i]);
 }
 engine.reset();l.fill(.25f);r.fill(.25f);engine.process(l.data(),r.data(),256,{0,true,false,true});
 waitFor(worker,8);for(float x:l)CHECK(x==0);
 CHECK(worker.statistics().outputFrames>=128*4);
}
struct Fault final:LiveStemBackend {
 int call{};bool primed{};
 void reset()override{primed=false;}
 bool process(const LiveStemHop&,SeparatedStemHop& out)override{
  ++call;out={};
  if(call==1)throw std::runtime_error("injected inference failure");
  if(call==2){out[0][0][0]=std::numeric_limits<float>::quiet_NaN();return true;}
  out[0][0].fill(.5f);out[0][1].fill(.5f);bool valid=primed;primed=true;return valid;
 }
};
static void failuresRecover(){
 LiveStemEngine engine;CHECK(engine.prepare(44100,1024,1));LiveStemWorker worker(engine,std::make_unique<Fault>());
 std::array<float,768> l{},r{};l.fill(.25f);r.fill(.25f);engine.process(l.data(),r.data(),768,{0,true,false,true});
 waitFor(worker,6,true);CHECK(worker.statistics().failures==2);
 l.fill(.25f);r.fill(.25f);engine.process(l.data(),r.data(),768,{768,true,false,true});
 waitFor(worker,12,true);CHECK(worker.statistics().failures==2);
 for(int i=0;i<768;++i)CHECK(l[i]==(i<256?0.f:(i<512?.25f:.5f))&&r[i]==l[i]);
}
struct Gate {std::atomic<bool> entered{false},release{false};};
struct Blocked final:LiveStemBackend {
 std::shared_ptr<Gate> gate;bool once{},primed{};
 explicit Blocked(std::shared_ptr<Gate> g):gate(std::move(g)){}
 void reset()override{primed=false;}
 bool process(const LiveStemHop&,SeparatedStemHop& out)override{
  if(!once){once=true;gate->entered.store(true);gate->release.wait(false);}
  out={};out[0][0].fill(.5f);out[0][1].fill(.5f);bool valid=primed;primed=true;return valid;
 }
};
static void seekDuringInference(){
 LiveStemEngine engine;CHECK(engine.prepare(44100,512,1));auto gate=std::make_shared<Gate>();
 LiveStemWorker worker(engine,std::make_unique<Blocked>(gate));
 struct Release {std::shared_ptr<Gate> g;~Release(){g->release.store(true);g->release.notify_all();}} release{gate};
 std::array<float,256> l{},r{};l.fill(.25f);r.fill(.25f);
 engine.process(l.data(),r.data(),128,{0,true,false,true});
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 while(!gate->entered.load()&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 CHECK(gate->entered.load());
 // These callback operations must finish while inference is still blocked.
 engine.reset();l.fill(.25f);r.fill(.25f);engine.process(l.data(),r.data(),256,{1000,true,false,true});
 gate->release.store(true);gate->release.notify_all();waitFor(worker,3);
 for(float x:l)CHECK(x==0);
 for(int block=1;block<3;++block){l.fill(.25f);r.fill(.25f);engine.process(l.data(),r.data(),256,{1000+block*256,true,false,true});waitFor(worker,3+block*2);}
 for(float x:l)CHECK(x==.5f);
}
int main(){varyingSamples();failuresRecover();seekDuringInference();std::cout<<"LiveStemWorker: off-audio inference, varying previous-hop alignment, blocked seek, exception/nonfinite recovery and shutdown passed\n";}
