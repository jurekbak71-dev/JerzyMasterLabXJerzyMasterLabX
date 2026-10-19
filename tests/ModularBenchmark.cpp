#include "SerialGraph.h"
#include <chrono>
#include <iostream>
int main(){
 std::cout<<"sample_rate,block_size,audio_seconds,wall_seconds,realtime_ratio\n";
 for(double rate:{44100.,48000.,96000.,192000.})for(int block:{32,256,2048}){
  masterlab::SerialGraph graph;graph.parameters(1000,2,1,-1,true,true);graph.limiterParameters(6,120);
  masterlab::StereoSettings s;s.enabled=true;s.low=.5f;s.high=1.2f;graph.stereoSettings(s);
  masterlab::DynamicsSettings d;d.enabled=true;d.analog=true;d.upward=2;graph.dynamicsSettings(d);
  masterlab::DynamicEqSettings e;e.enabled=true;graph.dynamicEqSettings(e);
  masterlab::StabilizerSettings a;a.enabled=true;graph.stabilizerSettings(a);graph.prepare(rate);
  std::vector<float> l(block),r(block),trace(block);int blocks=static_cast<int>(rate*5/block);auto begin=std::chrono::steady_clock::now();
  for(int j=0;j<blocks;++j){for(int i=0;i<block;++i){l[i]=.2f*std::sin((j*block+i)*.13f);r[i]=.17f*std::cos((j*block+i)*.11f);}graph.process(l.data(),r.data(),block,false,trace.data());}
  double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count(),audio=blocks*block/rate;std::cout<<rate<<','<<block<<','<<audio<<','<<wall<<','<<audio/wall<<'\n';
 }
}
