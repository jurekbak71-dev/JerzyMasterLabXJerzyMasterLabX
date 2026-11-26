#include "LiveHostExchange.h"
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <chrono>
#include <memory>
#include <vector>
struct Gain{float gain;unsigned delay=0,pos=0;std::array<float,64> left{},right{};ml::GraphDiagnostics diagnostics()const noexcept{ml::GraphDiagnostics d;d.latency=delay;return d;}
bool process(float* l,float* r,unsigned n)noexcept{for(unsigned i=0;i<n;++i){float a=l[i]*gain,b=r[i]*gain;if(delay){const unsigned rd=(pos+64-delay)%64;l[i]=left[rd];r[i]=right[rd];left[pos]=a;right[pos]=b;pos=(pos+1)%64;}else{l[i]=a;r[i]=b;}}return true;}};
int main(){Gain a{1},b{2};ml::LiveHostExchange<Gain> e;e.resetWhenStopped(&a);std::array<float,64> l{},r{};e.publish(&b);float previous=1;
for(int block=0;block<8;++block){l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));for(unsigned i=0;i<64;++i){assert(l[i]>=previous&&l[i]-previous<=1.f/256+1e-6f);assert(std::abs(l[i]+r[i])<1e-6f);if(block<4)assert(l[i]==1);previous=l[i];}if(block<7)assert(e.takeRetired()==nullptr);}
assert(e.takeRetired()==&a);assert(previous==2);l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));assert(l[0]==2&&r[0]==-2);
// A faster replacement retains the outgoing latency and is aligned during crossfade.
Gain slow{1,7},fast{1,0};e.resetWhenStopped(&slow);unsigned sample=0;
auto verify=[&](bool initial){for(unsigned i=0;i<64;++i){l[i]=float(sample+i+1);r[i]=-l[i];}assert(e.process(l.data(),r.data(),64));for(unsigned i=0;i<64;++i){float expected=(initial&&sample+i<7)?0.f:float(sample+i+1-7);assert(std::abs(l[i]-expected)<1e-5f);assert(std::abs(r[i]+expected)<1e-5f);}sample+=64;};
verify(true);e.publish(&fast);for(int i=0;i<9;++i)verify(false);assert(e.takeRetired()==&slow);assert(e.latency()==7);
// Slower replacement: use constant signal to detect dips and an impulse after settling.
Gain direct{1,0},delayed{1,13};e.resetWhenStopped(&direct);
l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));e.publish(&delayed);
bool retired=false;for(int block=0;block<20;++block){l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));for(unsigned i=0;i<64;++i){assert(std::abs(l[i]-1)<1e-6f);assert(std::abs(r[i]+1)<1e-6f);}if(e.takeRetired()==&direct){retired=true;break;}}
assert(retired&&e.latency()==13);l.fill(0);r.fill(0);assert(e.process(l.data(),r.data(),64));l[0]=1;r[0]=-1;assert(e.process(l.data(),r.data(),64));for(unsigned i=0;i<64;++i){assert(std::abs(l[i]-(i==13?1.f:0.f))<1e-6f);}
// Change target latency after fade begins; retirement must be delayed until re-alignment.
Gain x{1,0},y{2,0};e.resetWhenStopped(&x);e.publish(&y);
for(int block=0;block<5;++block){l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));}
assert(e.takeRetired()==nullptr);y.delay=5;l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));assert(e.takeRetired()==nullptr);
retired=false;for(int block=0;block<24;++block){l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));for(float v:l)assert(std::isfinite(v));if(e.takeRetired()==&x){retired=true;break;}}assert(retired&&e.latency()==5);
 // Repeated replacements on independent exchanges, with no shared signal state.
 constexpr unsigned count=8, cycles=100;std::array<Gain,count> first{},second{};
 std::vector<std::unique_ptr<ml::LiveHostExchange<Gain>>> exchanges;
 for(unsigned i=0;i<count;++i){first[i].gain=float(i+1);second[i].gain=float(i+2);exchanges.push_back(std::make_unique<ml::LiveHostExchange<Gain>>());exchanges.back()->resetWhenStopped(&first[i]);}
 const auto start=std::chrono::steady_clock::now();double maxJump=0;
 for(unsigned cycle=0;cycle<cycles;++cycle)for(unsigned i=0;i<count;++i){auto& ex=*exchanges[i];auto* old=(cycle%2)?&second[i]:&first[i];auto* next=(cycle%2)?&first[i]:&second[i];ex.publish(next);float prior=old->gain;for(unsigned block=0;block<8;++block){l.fill(1);r.fill(-1);assert(ex.process(l.data(),r.data(),64));for(unsigned n=0;n<64;++n){maxJump=std::max(maxJump,double(std::abs(l[n]-prior)));assert(std::abs(l[n]+r[n])<1e-6f);assert(std::abs(l[n]-prior)<=1.f/256+1e-5f);prior=l[n];}}assert(ex.takeRetired()==old);assert(std::abs(prior-next->gain)<1e-6f);}
 const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
 std::cout<<"{\"instances\":"<<count<<",\"replacements_per_instance\":"<<cycles<<",\"max_sample_jump\":"<<maxJump<<",\"elapsed_ms\":"<<ms<<",\"verified_pdc_samples\":13}\n";
}
