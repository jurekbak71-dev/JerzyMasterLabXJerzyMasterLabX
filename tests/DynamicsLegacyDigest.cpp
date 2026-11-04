#include "ModularDsp.h"
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <span>
int main(){for(double rate:{44100.,48000.,96000.,192000.})for(int scenario=0;scenario<3;++scenario){masterlab::DynamicsSettings settings;settings.enabled=true;settings.threshold=-24;settings.ratio=scenario==2?1.f:4.f;settings.attack=7;settings.release=130;settings.knee=5;settings.mix=scenario==1?.63f:1.f;settings.analog=scenario==1;settings.upward=scenario==2?7.f:0.f;masterlab::SmartCompressor compressor;compressor.settings(settings);compressor.prepare(rate);std::array<float,257> l{},r{};std::uint64_t hash=1469598103934665603ull;for(int block=0;block<500;++block){for(int i=0;i<257;++i){const double t=(block*257+i)/rate;l[i]=.32f*std::sin(6.283185307179586*311*t)+.04f*std::cos(6.283185307179586*4001*t);r[i]=.27f*std::sin(6.283185307179586*311*t+.2)+.06f*std::cos(6.283185307179586*7103*t);}compressor.process(l.data(),r.data(),257);if(block>200)for(int channel=0;channel<2;++channel)for(float value:channel?std::span<float>(r):std::span<float>(l)){const auto bits=std::bit_cast<std::uint32_t>(value);for(int byte=0;byte<4;++byte){hash^=(bits>>(byte*8))&255;hash*=1099511628211ull;}}}std::cout<<static_cast<int>(rate)<<','<<scenario<<','<<std::hex<<std::setw(16)<<std::setfill('0')<<hash<<std::dec<<'\n';}}
