#include "SerialGraph.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace masterlab;
static void check(bool ok,const char* label){if(!ok){std::fprintf(stderr,"FAIL %s\n",label);std::abort();}}
static std::vector<float> render(double sr,bool parallel,bool enabled,int mode,int block) {
 SerialGraph graph;graph.prepare(sr);
 graph.parameters(1000.f,0.f,1.f,-1.f,false,false);
 graph.topology(parallel,{false,false,false,true,false,false},{false,false,false,false,false,false});
 HarmonicExciterSettings h;h.enabled=false;graph.harmonicExciterSettings(h);
 AnalogColorSettings color;color.enabled=enabled;color.mode=static_cast<AnalogColorMode>(mode);
 color.driveDb=18.f;color.mix=.85f;color.bias=.3f;color.dynamics=.5f;
 graph.analogColorSettings(color);
 constexpr int size=32768;std::vector<float> left(size),right(size),trace(static_cast<size_t>(block));
 for(int i=0;i<size;++i){left[i]=right[i]=.25f*std::sin(6.283185307179586*997*i/sr);}
 for(int i=0;i<size;i+=block)graph.process(left.data()+i,right.data()+i,std::min(block,size-i),false,trace.data());
 for(int i=0;i<size;++i)check(std::isfinite(left[i])&&left[i]==right[i],"finite dual mono");
 if(enabled)check(graph.analogColorRead().deltaRms>1e-5f,"color DSP metering");
 return left;
}
int main(){
 for(double sr:{44100.,48000.,96000.,192000.})for(bool parallel:{false,true}){
  auto neutral=render(sr,parallel,false,0,257);
  auto changed=render(sr,parallel,true,0,257);
  double delta=0.,energy=0.;
  for(size_t i=4096;i<neutral.size();++i){const double d=changed[i]-neutral[i];delta+=d*d;energy+=neutral[i]*neutral[i];}
  check(energy>1e-5&&delta/energy>1e-4,"graph reflects Analog Color settings");
  auto single=render(sr,parallel,true,0,1);
  auto wide=render(sr,parallel,true,0,4097);
  for(size_t i=0;i<changed.size();++i)check(std::abs(changed[i]-single[i])<1e-6f&&std::abs(changed[i]-wide[i])<1e-6f,"graph block independence");
  std::printf("%g Hz %s: analog delta %.6f PASS\n",sr,parallel?"parallel":"serial",std::sqrt(delta/energy));
 }
}
