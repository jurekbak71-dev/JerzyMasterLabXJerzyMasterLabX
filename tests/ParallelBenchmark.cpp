#include "SerialGraph.h"
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>

double run(double rate,bool controls){
 masterlab::SerialGraph graph;graph.parameters(1200,9,.8f,-1,true,false);masterlab::DynamicsSettings dynamics;dynamics.enabled=true;dynamics.threshold=-24;dynamics.ratio=4;graph.dynamicsSettings(dynamics);
 std::array<float,6> trims{},mixes{1,1,1,1,1,1};if(controls){trims[1]=6;trims[3]=-6;mixes[1]=.3f;mixes[3]=.8f;}graph.parallelSettings(controls,trims,mixes);graph.topology(true,{false,true,false,true,false,false});graph.prepare(rate);
 std::array<float,257> l{},r{},trace{};const auto start=std::chrono::steady_clock::now();for(int block=0;block<2000;++block){for(int i=0;i<257;++i){const double p=(block*257+i)*.071;l[i]=.15f*std::sin(p);r[i]=.12f*std::cos(p*.37);}graph.process(l.data(),r.data(),257,false,trace.data());}return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
}
int main(){std::cout<<"rate,legacy_best_seconds,controlled_best_seconds,overhead_percent,controlled_realtime_ratio\n";for(double rate:{44100.,48000.,96000.,192000.}){double legacy=1e9,controlled=1e9;for(int repeat=0;repeat<5;++repeat){legacy=std::min(legacy,run(rate,false));controlled=std::min(controlled,run(rate,true));}std::cout<<rate<<','<<legacy<<','<<controlled<<','<<(controlled/legacy-1)*100<<','<<(2000*257/rate)/controlled<<'\n';}}
