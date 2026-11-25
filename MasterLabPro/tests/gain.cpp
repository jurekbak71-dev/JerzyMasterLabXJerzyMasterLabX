#include "GainDSP.h"
#include <array>
#include <limits>
#include <iostream>
int main(){
 ml::GainDSP g;g.prepare(48000);
 std::array<float,960> l,r;l.fill(.1f);r.fill(-.2f);
 g.process(l.data(),r.data(),l.size());
 if(l[0]!=.1f||r.back()!=-.2f)return 1;
 g.setGainDb(6.0205999f);g.process(l.data(),r.data(),l.size());
 if(!(l.front()>.1f&&l.front()<.101f)||std::abs(l.back()-.2f)>1e-6f||std::abs(r.back()+.4f)>1e-6f)return 2;
 g.setGainDb(0);l.fill(.1f);g.process(l.data(),nullptr,l.size());
 if(std::abs(l.back()-.1f)>1e-6f)return 3;
 g.setGainDb(std::numeric_limits<float>::quiet_NaN());g.process(nullptr,nullptr,960);
 l.fill(.1f);g.process(l.data(),l.data(),l.size());
 if(l.back()!=.1f)return 4;
 std::cout<<"PASS gain unity, ramp, stereo, reset, NaN target, aliased channels\n";
}
