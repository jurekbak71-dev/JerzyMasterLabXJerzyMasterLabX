#include "Stabilizer.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>

int main(){
 for(double rate:{44100.,48000.,96000.,192000.})for(int scenario=0;scenario<3;++scenario){
  masterlab::StabilizerSettings settings;settings.enabled=true;settings.cut=scenario==1;settings.tame=scenario!=2;settings.profile=scenario;settings.amount=scenario==2?63.f:100.f;settings.speed=17;settings.smoothing=42;
  masterlab::Stabilizer stabilizer;stabilizer.settings(settings);stabilizer.prepare(rate);std::array<float,257> left{},right{};std::uint64_t hash=1469598103934665603ull;
  for(int block=0;block<500;++block){for(int i=0;i<257;++i){const double t=(block*257+i)/rate;left[i]=.19f*std::sin(6.283185307179586*125*t)+.07f*std::cos(6.283185307179586*2200*t);right[i]=.15f*std::sin(6.283185307179586*125*t+.2)+.08f*std::cos(6.283185307179586*7000*t);}stabilizer.process(left.data(),right.data(),257);if(block>200)for(int channel=0;channel<2;++channel)for(float value:channel?std::span<float>(right):std::span<float>(left)){const auto bits=std::bit_cast<std::uint32_t>(value);for(int byte=0;byte<4;++byte){hash^=(bits>>(byte*8))&255;hash*=1099511628211ull;}}}
  std::cout<<static_cast<int>(rate)<<','<<scenario<<','<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<std::dec<<'\n';
 }
}
