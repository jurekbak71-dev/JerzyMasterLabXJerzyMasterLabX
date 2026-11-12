#include "SerialGraph.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>

static thread_local bool audit=false;
static thread_local size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
using Block=std::array<std::array<float,257>,2>;

void configure(masterlab::SerialGraph& graph,double rate,bool parallel,std::array<bool,6> present,bool eq,bool dynamics,bool limiter=false){
 graph.parameters(1200,9,.8f,-12,eq,limiter);graph.limiterParameters(limiter?12.f:0.f,80);
 masterlab::DynamicsSettings d;d.enabled=dynamics;d.threshold=-30;d.ratio=5;d.attack=1;d.release=80;graph.dynamicsSettings(d);
 graph.topology(parallel,present);graph.prepare(rate);
}
Block input(int block,double rate){Block out;for(int i=0;i<257;++i){const double t=(block*257+i)/rate;out[0][i]=.18f*std::sin(6.283185307179586*311*t)+.07f*std::sin(6.283185307179586*2400*t);out[1][i]=.14f*std::sin(6.283185307179586*311*t+.3)+.06f*std::sin(6.283185307179586*5100*t);}return out;}
void process(masterlab::SerialGraph& graph,Block& block){std::array<float,257> trace{};audit=true;graph.process(block[0].data(),block[1].data(),257,false,trace.data());audit=false;}

int main(){
 std::cout<<"rate,legacy_delta_rms,controlled_delta_rms,correlation,peak\n";
 for(double rate:{44100.,48000.,96000.,192000.}){
  masterlab::SerialGraph legacy,eq,dynamics,dry,controlled,compensated,reversed;
  configure(legacy,rate,true,{false,true,false,true,false,false},true,true);
  configure(eq,rate,false,{},true,false);configure(dynamics,rate,false,{},false,true);configure(dry,rate,false,{},false,false);
  std::array<float,6> trims{},mixes{1,1,1,1,1,1};trims[1]=6;trims[3]=-6;mixes[1]=.25f;mixes[3]=.75f;
  controlled.parallelSettings(false,trims,mixes);configure(controlled,rate,true,{false,true,false,true,false,false},true,true);
  compensated.parallelSettings(true,trims,mixes);configure(compensated,rate,true,{false,true,false,true,false,false},true,true);
  reversed.parallelSettings(false,trims,mixes);reversed.routing(masterlab::encodeOrder({4,3,2,1,0,5}),false);configure(reversed,rate,true,{false,true,false,true,false,false},true,true);
  const float gd=masterlab::dbGain(6),ge=masterlab::dbGain(-6);double legacyError=0,controlledError=0,deltaEnergy=0,inputEnergy=0,ll=0,rr=0,lr=0,peak=0;
  for(int block=0;block<500;++block){auto source=input(block,rate),a=source,b=source,c=source,d=source,e=source,f=source,g=source;process(legacy,a);process(eq,b);process(dynamics,c);process(dry,d);process(controlled,e);process(compensated,f);process(reversed,g);
   if(block>200)for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i){
    const float oldExpected=(b[ch][i]+c[ch][i])*.5f;
    const float dynMixed=d[ch][i]+.25f*(c[ch][i]-d[ch][i]),eqMixed=d[ch][i]+.75f*(b[ch][i]-d[ch][i]);
    const float expected=(gd*dynMixed+ge*eqMixed)*.5f,expectedComp=(gd*dynMixed+ge*eqMixed)/(gd+ge);
    legacyError+=std::pow(a[ch][i]-oldExpected,2);controlledError+=std::pow(e[ch][i]-expected,2);
    check(std::abs(f[ch][i]-expectedComp)<2e-5f,"compensated weighting is not normalized");
    check(std::abs(g[ch][i]-e[ch][i])<2e-5f,"parallel output depends on card order");
    const double delta=e[ch][i]-d[ch][i];deltaEnergy+=delta*delta;inputEnergy+=static_cast<double>(d[ch][i])*d[ch][i];peak=std::max(peak,std::abs(static_cast<double>(e[ch][i])));
   }
   if(block>200)for(int i=0;i<257;++i){ll+=static_cast<double>(e[0][i])*e[0][i];rr+=static_cast<double>(e[1][i])*e[1][i];lr+=static_cast<double>(e[0][i])*e[1][i];}
  }
  const double legacyRms=std::sqrt(legacyError/(299*257*2)),controlledRms=std::sqrt(controlledError/(299*257*2));
  check(legacyRms<2e-6,"legacy equal 1/N output changed");check(controlledRms<2e-5,"trim/mix output does not match explicit branch equation");
  check(deltaEnergy/inputEnergy>.01,"branch controls do not create a measurable difference");
  auto reading=controlled.parallelRead();check(reading.active&&reading.branches==2&&!reading.compensated&&reading.present[1]&&reading.present[3],"parallel telemetry state is wrong");check(reading.rms[1]>0&&reading.rms[3]>0,"parallel telemetry has no real signal level");
  std::cout<<rate<<','<<legacyRms<<','<<controlledRms<<','<<lr/std::sqrt(ll*rr)<<','<<peak<<'\n';

  masterlab::SerialGraph bypassed;configure(bypassed,rate,true,{false,true,false,true,false,false},false,false);for(int block=0;block<100;++block){auto a=input(block,rate),b=a;process(bypassed,a);process(dry,b);if(block>50)for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i)check(std::abs(a[ch][i]-b[ch][i])<1e-6f,"all-bypassed branches do not null to aligned dry");}

  masterlab::SerialGraph diluted,onlyEq,eqReference,dryReference;configure(diluted,rate,true,{false,true,true,true,false,false},true,false);configure(onlyEq,rate,true,{false,false,false,true,false,false},true,false);configure(eqReference,rate,false,{},true,false);configure(dryReference,rate,false,{},false,false);for(int block=0;block<100;++block){auto a=input(block,rate),b=a,c=a,d=a;process(diluted,a);process(onlyEq,b);process(eqReference,c);process(dryReference,d);if(block>50)for(int ch=0;ch<2;++ch)for(int i=0;i<257;++i){check(std::abs(a[ch][i]-(c[ch][i]+2*d[ch][i])/3)<2e-6f,"one processed plus two bypassed branches does not use legacy 1/N");check(std::abs(b[ch][i]-c[ch][i])<2e-6f,"removed branches still affect the parallel sum");}}

  masterlab::SerialGraph limiter;configure(limiter,rate,true,{false,false,false,true,true,false},true,false,true);float minimumTrace=1;for(int block=0;block<100;++block){auto a=input(block,rate);std::array<float,257> limiterTrace{};limiter.process(a[0].data(),a[1].data(),257,false,limiterTrace.data());for(float value:limiterTrace)minimumTrace=std::min(minimumTrace,value);}check(minimumTrace<.999f,"parallel limiter gain trace is disconnected");check(limiter.parallelRead().limiterBypassed,"parallel limiter bypass warning was not raised");limiter.topology(true,{false,false,false,true,true,false},{true,true,true,true,false,true});check(!limiter.parallelRead().limiterBypassed&&!limiter.parallelRead().enabled[4],"bypassed limiter telemetry or warning is wrong");check(limiter.latency()==static_cast<int>(std::ceil(rate*.005))+masterlab::AnalogExciter::latencySamples()+masterlab::FinalTruePeak::latencySamples,"parallel controls changed PDC");
 }
 check(allocations==0,"parallel processing allocated on the audio thread");

 masterlab::SerialGraph automated;std::array<float,6> trims{},mixes{1,1,1,1,1,1};automated.parallelSettings(false,trims,mixes);configure(automated,48000,true,{false,true,false,true,false,false},true,true);float previous=0;double jump=0;int jumpBlock=0,jumpSample=0;std::array<float,128> l{},r{},trace{};
 for(int block=0;block<600;++block){if(block==100)trims[1]=12;if(block==220)mixes[3]=0;if(block==340){trims[1]=-24;mixes[3]=1;}automated.parallelSettings(block>=460,trims,mixes);l.fill(.1f);r.fill(.1f);automated.process(l.data(),r.data(),128,false,trace.data());for(int i=0;i<128;++i){const double step=std::abs(static_cast<double>(l[i]-previous));if(block>20&&step>jump){jump=step;jumpBlock=block;jumpSample=i;}previous=l[i];}}
 if(jump>=.002){std::cerr<<"maximum automated DC step="<<jump<<" at block "<<jumpBlock<<" sample "<<jumpSample<<'\n';check(false,"parallel trim/mix automation clicks on constant input");}
}
