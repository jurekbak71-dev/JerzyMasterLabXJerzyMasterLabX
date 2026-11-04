#include "SerialGraph.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>

int main(){
 for(double rate:{44100.,48000.,96000.,192000.})for(int scenario=0;scenario<3;++scenario){
  masterlab::SerialGraph graph;const bool eq=scenario==0,dynamics=scenario<2;graph.parameters(1300,8,.7f,-1,eq,false);masterlab::DynamicsSettings d;d.enabled=dynamics;d.threshold=-28;d.ratio=4;d.attack=3;graph.dynamicsSettings(d);
  const std::array<std::array<bool,6>,3> presence{{{false,true,false,true,false,false},{false,true,true,true,false,false},{true,true,true,true,false,false}}};graph.topology(true,presence[scenario]);graph.prepare(rate);
  std::array<float,257> l{},r{},trace{};std::uint64_t hash=1469598103934665603ull;for(int block=0;block<500;++block){for(int i=0;i<257;++i){const double t=(block*257+i)/rate;l[i]=.17f*std::sin(6.283185307179586*311*t)+.04f*std::cos(6.283185307179586*4001*t);r[i]=.13f*std::sin(6.283185307179586*311*t+.2)+.06f*std::cos(6.283185307179586*7103*t);}graph.process(l.data(),r.data(),257,false,trace.data());if(block>200)for(int channel=0;channel<2;++channel)for(float value:channel?std::span<float>(r):std::span<float>(l)){const auto bits=std::bit_cast<std::uint32_t>(value);for(int byte=0;byte<4;++byte){hash^=(bits>>(byte*8))&255;hash*=1099511628211ull;}}}
  std::cout<<static_cast<int>(rate)<<','<<scenario<<','<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<std::dec<<'\n';
 }
}
