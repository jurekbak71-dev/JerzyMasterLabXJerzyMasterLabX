#include "LiveHostExchange.h"
#include <array>
#include <cassert>
#include <cmath>
struct Gain{float gain;unsigned delay=0,pos=0;std::array<float,64> left{},right{};ml::GraphDiagnostics diagnostics()const noexcept{ml::GraphDiagnostics d;d.latency=delay;return d;}
bool process(float* l,float* r,unsigned n)noexcept{for(unsigned i=0;i<n;++i){float a=l[i]*gain,b=r[i]*gain;if(delay){const unsigned rd=(pos+64-delay)%64;l[i]=left[rd];r[i]=right[rd];left[pos]=a;right[pos]=b;pos=(pos+1)%64;}else{l[i]=a;r[i]=b;}}return true;}};
int main(){Gain a{1},b{2};ml::LiveHostExchange<Gain> e;e.resetWhenStopped(&a);std::array<float,64> l{},r{};e.publish(&b);float previous=1;
for(int block=0;block<8;++block){l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));for(unsigned i=0;i<64;++i){assert(l[i]>=previous&&l[i]-previous<=1.f/256+1e-6f);assert(std::abs(l[i]+r[i])<1e-6f);if(block<4)assert(l[i]==1);previous=l[i];}if(block<7)assert(e.takeRetired()==nullptr);}
assert(e.takeRetired()==&a);assert(previous==2);l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));assert(l[0]==2&&r[0]==-2);
// A faster replacement retains the outgoing latency and is aligned during crossfade.
Gain slow{1,7},fast{1,0};e.resetWhenStopped(&slow);unsigned sample=0;
auto verify=[&](bool initial){for(unsigned i=0;i<64;++i){l[i]=float(sample+i+1);r[i]=-l[i];}assert(e.process(l.data(),r.data(),64));for(unsigned i=0;i<64;++i){float expected=(initial&&sample+i<7)?0.f:float(sample+i+1-7);assert(std::abs(l[i]-expected)<1e-5f);assert(std::abs(r[i]+expected)<1e-5f);}sample+=64;};
verify(true);e.publish(&fast);for(int i=0;i<9;++i)verify(false);assert(e.takeRetired()==&slow);assert(e.latency()==7);
}
