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
  masterlab::StereoSettings stereo;stereo.enabled=true;stereo.stereoize=.7f;stereo.low=.25f;stereo.high=1.5f;
  masterlab::DynamicsSettings dynamics;dynamics.enabled=true;dynamics.threshold=-24;dynamics.ratio=4;
  graph.parameters(2000,6,1,-1,true,true);graph.limiterParameters(4,120);graph.prepare(rate);legacy.parameters(2000,6,1,-1,true,true);legacy.limiterParameters(4,120);legacy.prepare(rate);
  std::array<float,257> l{},r{},trace{},ll{},rr{};
  // Compare against the legacy output delayed by the declared 128-sample
  // final-stage FIFO. Preserve all sample values, not the old absolute timing.
  std::array<std::array<float,masterlab::FinalTruePeak::latencySamples+masterlab::AnalogExciter::latencySamples()>,2> legacyDelay{};
  size_t delayCursor=0;
  for(int block=0;block<100;++block){
   for(int i=0;i<257;++i){l[i]=.15f*std::sin((block*257+i)*.11f);r[i]=.1f*std::cos((block*257+i)*.037f);}
   ll=l;rr=r;graph.process(l.data(),r.data(),257,false,trace.data());
   legacy.process(ll.data(),rr.data(),257);
   for(int i=0;i<257;++i){
    const float expectedLeft=legacyDelay[0][delayCursor];
    const float expectedRight=legacyDelay[1][delayCursor];
    legacyDelay[0][delayCursor]=ll[i];
    legacyDelay[1][delayCursor]=rr[i];
    delayCursor=(delayCursor+1)%legacyDelay[0].size();
    check(std::abs(l[i]-expectedLeft)<1e-6f&&std::abs(r[i]-expectedRight)<1e-6f,
          "default topology changed latency-aligned legacy sound");
   }
  }
  graph.stereoSettings(stereo);graph.dynamicsSettings(dynamics);
  for(int code=0;code<720;++code){graph.routing(code,false);graph.topology(code%2!=0,{true,true,true,true,true,true});for(int block=0;block<5;++block){for(int i=0;i<257;++i){l[i]=.2f*std::sin((code*1285+block*257+i)*.09f);r[i]=-.12f*std::cos((code*1285+block*257+i)*.031f);}audit=true;graph.process(l.data(),r.data(),257,false,trace.data());audit=false;for(int i=0;i<257;++i)check(std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<4&&std::abs(r[i])<4,"switching route produced invalid audio");check(graph.latency()==static_cast<int>(std::ceil(rate*.005))+masterlab::AnalogExciter::latencySamples()+masterlab::FinalTruePeak::latencySamples,"route changed latency");}}
  // A limiter before a compressor is measurably different from the reverse.
  masterlab::SerialGraph a,b;for(auto* g:{&a,&b}){g->parameters(2000,0,1,-6,false,true);g->dynamicsSettings(dynamics);g->limiterParameters(12,120);}
  a.prepare(rate,false,masterlab::encodeOrder({5,0,1,2,3,4}));b.prepare(rate,false,masterlab::encodeOrder({5,0,4,2,3,1}));double difference=0;
  for(int block=0;block<200;++block){for(int i=0;i<257;++i)l[i]=r[i]=.3f*std::sin((block*257+i)*.17f);ll=l;rr=r;a.process(l.data(),r.data(),257,false,trace.data());b.process(ll.data(),rr.data(),257,false,trace.data());if(block>100)for(int i=0;i<257;++i)difference+=std::abs(l[i]-ll[i]);}
  check(difference>1,"module order does not affect audio");
 }
 // Nine independently movable modules; final combined limiter is pinned.
 for(int shift=0;shift<9;++shift){
  auto candidate=masterlab::defaultFullOrder;
  std::rotate(candidate.begin(),candidate.begin()+shift,candidate.end()-1);
  auto sanitized=masterlab::sanitizeFullOrder(candidate);
  check(sanitized==candidate,"a valid movable route changed");
  check(sanitized.back()==10,"final limiter is not locked at chain tail");
  auto sorted=sanitized;std::sort(sorted.begin(),sorted.end());
  check(sorted==masterlab::FullModuleOrder{0,1,2,3,5,6,8,9,10,11},
        "router lost or duplicated one of ten real modules");
 }
 {
  const auto repaired=masterlab::sanitizeFullOrder({6,6,6,6,6,6,6,6,10,255});
  auto sorted=repaired;std::sort(sorted.begin(),sorted.end());
  check(sorted==masterlab::FullModuleOrder{0,1,2,3,5,6,8,9,10,11},
        "corrupt intermediate host automation duplicates processors");
  check(repaired.back()==10,"host tried to move final limiter away from tail");
 }
 // Moving Analog Exciter before the spectral EQ alters real processed audio,
 // while the true-peak limiter remains the last module in both routes.
 for(double rate:{44100.,48000.}){
  masterlab::SerialGraph standard,moved;
  for(auto* g:{&standard,&moved}){
   g->parameters(2000,12,1,-1,true,true);
   g->limiterParameters(5,120);
   masterlab::AnalogColorSettings color;color.enabled=true;color.driveDb=15;color.bias=.3f;color.mix=.85f;
   g->analogColorSettings(color);
   masterlab::FinalTruePeakSettings peak;peak.enabled=true;peak.ceilingDb=-6;peak.driveDb=6;
   g->finalTruePeakSettings(peak);
   g->prepare(rate);
  }
  auto changed=masterlab::defaultFullOrder;
  std::swap(changed[4],changed[5]);
  standard.extendedRouting(true,masterlab::defaultFullOrder);
  moved.extendedRouting(true,changed);
  std::array<float,257> a{},b{},c{},d{},trace{};
  double delta=0;
  for(int block=0;block<200;++block){
   for(int i=0;i<257;++i){
    const float v=.3f*std::sin((block*257+i)*.14f);
    a[i]=b[i]=c[i]=d[i]=v;
   }
   audit=true;
   standard.process(a.data(),b.data(),257,false,trace.data());
   moved.process(c.data(),d.data(),257,false,trace.data());
   audit=false;
   if(block>100)for(int i=0;i<257;++i){
    check(std::isfinite(a[i])&&std::isfinite(c[i])&&std::abs(a[i])<8&&std::abs(c[i])<8,
          "full router emitted invalid sample");
    delta+=std::abs(a[i]-c[i]);
   }
  }
  check(standard.fullRouterActive()&&moved.fullRouterActive(),"full route not activated");
  check(delta>.01,"real Analog Exciter/EQ reorder did not change audio");
  check(standard.fullModuleOrder().back()==10&&moved.fullModuleOrder().back()==10,
        "final limiter moved from end");
  check(standard.latency()==moved.latency(),"full router changed reported PDC");
 }
 check(allocations==0,"routing allocated in callback");std::cout<<"Passed: 720 complete permutations, legacy sound, allocation-free live changes, finite output, stable PDC and audible topology at four rates\n";
}
