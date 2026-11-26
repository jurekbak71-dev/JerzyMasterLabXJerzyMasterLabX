#include "LiveHostExchange.h"
#include <array>
#include <cassert>
#include <cmath>
struct Gain{float gain;bool process(float* l,float* r,unsigned n)noexcept{for(unsigned i=0;i<n;++i){l[i]*=gain;r[i]*=gain;}return true;}};
int main(){Gain a{1},b{2};ml::LiveHostExchange<Gain> e;e.resetWhenStopped(&a);std::array<float,64> l{},r{};e.publish(&b);float previous=1;
for(int block=0;block<4;++block){l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));for(unsigned i=0;i<64;++i){assert(l[i]>=previous&&l[i]-previous<=1.f/256+1e-6f);assert(std::abs(l[i]+r[i])<1e-6f);previous=l[i];}if(block<3)assert(e.takeRetired()==nullptr);}
assert(e.takeRetired()==&a);assert(previous==2);l.fill(1);r.fill(-1);assert(e.process(l.data(),r.data(),64));assert(l[0]==2&&r[0]==-2);}
