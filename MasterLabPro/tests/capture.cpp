#include "AudioRack.h"
#include <iostream>
#include <memory>
int main(){
 auto rack=std::make_unique<ml::AudioRack>();rack->prepare(48000);
 std::array<float,ml::fftHop> l{},r{};
 auto feed=[&](int blocks){for(int b=0;b<blocks;++b){l.fill(.125f);r.fill(-.25f);rack->process(l.data(),r.data(),l.size());}};
 ml::AnalysisFrame f;
 feed(3);if(rack->pollAnalysis(f))return 1;
 feed(1);if(!rack->pollAnalysis(f)||f.endSample!=4096||f.sourceId!=0)return 2;
 for(std::size_t i=0;i<ml::fftSize;++i)if(f.samples[0][0][i]!=.125f||f.samples[1][1][i]!=-.25f||f.samples[2][0][i]!=0)return 3;
 feed(1);if(!rack->pollAnalysis(f)||f.endSample!=5120)return 4;
 auto epoch=f.epoch;
 ml::RackConfig c;c.stages[0]={1,6.0205999f};c.deltaId=1;c.moduleAnalysis=true;
 if(!rack->submit(c))return 5;
 feed(3);if(rack->pollAnalysis(f))return 6;
 feed(1);if(!rack->pollAnalysis(f)||f.sourceId!=1||f.epoch==epoch||f.endSample!=9216)return 7;
 if(std::abs(f.samples[1][0].back()-.25f)>1e-6f||std::abs(f.samples[2][1].back()+.25f)>1e-6f)return 8;
 feed(10);int count=0;while(rack->pollAnalysis(f))++count;if(count!=4)return 9;
 auto last=f.endSample;feed(1);if(!rack->pollAnalysis(f)||f.endSample<=last+ml::fftHop)return 10;
 rack->resetWhenStopped();rack->prepare(44100);feed(4);
 if(!rack->pollAnalysis(f)||f.sourceId||f.endSample!=4096||f.sampleRate!=44100)return 11;
 std::cout<<"PASS capture synchronization, overlap, source reset, queue overflow, restart\n";
}
