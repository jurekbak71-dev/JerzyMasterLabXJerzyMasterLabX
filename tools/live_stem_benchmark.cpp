// Measures the callback boundary WITHOUT a model/worker. Never a live AI claim.
#include "LiveStemEngine.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

int main() {
 using Clock=std::chrono::steady_clock;
 std::cout<<"kind,rate,block,latency_samples,callbacks,p50_us,p99_us,max_us,deadline_us,dropped_input\n";
 for(const auto* kind:{"boundary_starved","boundary_identity_fixture","boundary_stale_backlog"})
 for(int rate:{44100,48000,96000}) for(std::size_t block:{32u,128u,257u,1024u}) {
  masterlab::LiveStemEngine engine;
  if(!engine.prepare(rate,1024,static_cast<std::size_t>(rate/200))) return 1;
  std::vector<float> left(block),right(block);
  std::vector<double> durations;durations.reserve(2000);
  for(int iteration=0;iteration<2000;++iteration) {
   std::fill(left.begin(),left.end(),.25f);std::fill(right.begin(),right.end(),-.25f);
   const auto start=Clock::now();
   engine.process(left.data(),right.data(),block,{0,true,false,false});
   durations.push_back(std::chrono::duration<double,std::micro>(Clock::now()-start).count());
   // Feed the boundary outside the timed callback. These are test fixtures,
   // neither a concurrent model nor evidence of neural inference performance.
   masterlab::StereoLiveFrame input{};
   if(std::string_view(kind)!="boundary_starved") while(engine.popInput(input)) {
    if(std::string_view(kind)=="boundary_identity_fixture") {
     masterlab::SeparatedLiveFrame output{};output.epoch=input.epoch;output.position=input.position;
     for(std::size_t stem=0;stem<4;++stem){output.channels[stem*2]=input.left*.25f;output.channels[stem*2+1]=input.right*.25f;}
     engine.pushOutput(output);
    } else {
     for(int stale=0;stale<8;++stale) if(!engine.pushOutput({})) break;
    }
   }
  }
  std::sort(durations.begin(),durations.end());
  std::cout<<kind<<','<<rate<<','<<block<<",1024,2000,"<<durations[1000]<<','
   <<durations[1980]<<','<<durations.back()<<','<<1.e6*block/rate<<','<<engine.statistics().droppedInput<<'\n';
 }
}
