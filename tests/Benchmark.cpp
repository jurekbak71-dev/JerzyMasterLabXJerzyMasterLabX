#include "Dsp.h"
#include <chrono>
#include <iostream>
int main(){
 std::cout<<"sample_rate,block_size,audio_seconds,wall_seconds,realtime_ratio\n";
 for(double rate:{44100.,48000.,96000.,192000.})for(int block:{32,256,2048}){
 masterlab::RoutedEngine engine;engine.prepare(rate);engine.parameters(1000,2,1,-1,true,true);
 for(size_t band=1;band<16;++band)engine.setBand(band,40.f*std::pow(1.45f,static_cast<float>(band)),band%2?2.f:-2.f,1);
 std::vector<float> left(static_cast<size_t>(block)),right=left;
 const int blocks=static_cast<int>(rate*5/block);auto begin=std::chrono::steady_clock::now();
 for(int j=0;j<blocks;++j){for(int i=0;i<block;++i)left[i]=right[i]=.4f*std::sin(static_cast<float>((j*block+i)*.13));engine.process(left.data(),right.data(),block);}
 double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();double audio=blocks*block/rate;
 std::cout<<rate<<','<<block<<','<<audio<<','<<wall<<','<<audio/wall<<'\n';
 }
}
