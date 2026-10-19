#include "SerialGraph.h"
#include <cstdlib>
#include <iostream>
#include <new>
static bool audit=false;static size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 for(int code=0;code<720;++code)for(bool legacy:{false,true}){auto order=masterlab::decodeOrder(code,legacy);auto sorted=order;std::sort(sorted.begin(),sorted.end());check(sorted==masterlab::ModuleOrder{0,1,2,3,4,5},"routing is not a permutation");check(masterlab::encodeOrder(order,legacy)==code,"permutation encoding lost information");}
 for(double rate:{44100.,48000.,96000.,192000.}){
  masterlab::SerialGraph graph;masterlab::RoutedEngine legacy;
  masterlab::StereoSettings stereo;stereo.enabled=true;stereo.low=.25f;stereo.high=1.5f;
  masterlab::DynamicsSettings dynamics;dynamics.enabled=true;dynamics.threshold=-24;dynamics.ratio=4;
  graph.parameters(2000,6,1,-1,true,true);graph.limiterParameters(4,120);graph.prepare(rate);legacy.parameters(2000,6,1,-1,true,true);legacy.limiterParameters(4,120);legacy.prepare(rate);
  std::array<float,257> l{},r{},trace{},ll{},rr{};
  // Default topology preserves previous audio when new modules are absent.
  for(int block=0;block<100;++block){for(int i=0;i<257;++i){l[i]=.15f*std::sin((block*257+i)*.11f);r[i]=.1f*std::cos((block*257+i)*.037f);}ll=l;rr=r;graph.process(l.data(),r.data(),257,false,trace.data());legacy.process(ll.data(),rr.data(),257);for(int i=0;i<257;++i)check(std::abs(l[i]-ll[i])<1e-6f&&std::abs(r[i]-rr[i])<1e-6f,"default topology changed legacy sound");}
  graph.stereoSettings(stereo);graph.dynamicsSettings(dynamics);
  for(int code=0;code<720;++code){graph.routing(code,false);for(int block=0;block<5;++block){for(int i=0;i<257;++i){l[i]=.2f*std::sin((code*1285+block*257+i)*.09f);r[i]=-.12f*std::cos((code*1285+block*257+i)*.031f);}audit=true;graph.process(l.data(),r.data(),257,false,trace.data());audit=false;for(int i=0;i<257;++i)check(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<4&&std::abs(r[i])<4,"switching route produced invalid audio");check(graph.latency()==static_cast<int>(std::ceil(rate*.005)),"route changed latency");}}
  // A limiter before a compressor is measurably different from the reverse.
  masterlab::SerialGraph a,b;for(auto* g:{&a,&b}){g->parameters(2000,0,1,-6,false,true);g->dynamicsSettings(dynamics);g->limiterParameters(12,120);}
  a.prepare(rate,false,masterlab::encodeOrder({5,0,1,2,3,4}));b.prepare(rate,false,masterlab::encodeOrder({5,0,4,2,3,1}));double difference=0;
  for(int block=0;block<200;++block){for(int i=0;i<257;++i)l[i]=r[i]=.3f*std::sin((block*257+i)*.17f);ll=l;rr=r;a.process(l.data(),r.data(),257,false,trace.data());b.process(ll.data(),rr.data(),257,false,trace.data());if(block>100)for(int i=0;i<257;++i)difference+=std::abs(l[i]-ll[i]);}
  check(difference>1,"module order does not affect audio");
 }
 check(allocations==0,"routing allocated in callback");std::cout<<"Passed: 720 complete permutations, legacy sound, allocation-free live changes, finite output, stable PDC and audible topology at four rates\n";
}
