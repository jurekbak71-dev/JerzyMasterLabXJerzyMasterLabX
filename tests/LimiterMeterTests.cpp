#include "Dsp.h"
#include "LimiterMeter.h"
#include <iostream>
#include <vector>
#include <cstdlib>
void require(bool condition,const char* message){if(!condition){std::cerr<<message<<'\n';std::exit(1);}}
std::vector<masterlab::LimiterFrame> measure(int block,double rate){
 masterlab::LimiterMeter meter;meter.prepare(rate);std::vector<masterlab::LimiterFrame> frames;
 const int total=static_cast<int>(rate*1.13);
 for(int offset=0;offset<total;offset+=block)for(int i=offset;i<std::min(total,offset+block);++i){
  masterlab::LimiterFrame frame;float gain=i%701==0?.25f:.8f,left=i%311==0?1.2f:.1f;
  if(meter.push(gain,left,-.5f,frame))frames.push_back(frame);
 }
 return frames;
}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.,11025.}){
  auto baseline=measure(1,rate);
  require(baseline.size()>=112&&baseline.size()<=113,"incorrect audio-clocked frame count");
  for(int block:{32,257,4097}){auto frames=measure(block,rate);require(frames.size()==baseline.size(),"host block changes history length");for(size_t i=0;i<frames.size();++i){require(frames[i].endSample==baseline[i].endSample&&frames[i].minimumGain==baseline[i].minimumGain&&frames[i].outputPeak==baseline[i].outputPeak,"host block changes measurement");}}
  for(size_t i=0;i<baseline.size();++i)require(baseline[i].endSample==static_cast<std::uint64_t>(std::ceil((i+1)*rate*.01)),"frame clock drifts");
 }
 masterlab::LimiterMeter meter;meter.prepare(48000);masterlab::LimiterFrame frame;
 for(int i=0;i<480;++i)require(meter.push(1,0,0,frame)==(i==479),"silence frame timing");
 require(frame.minimumGain==1&&frame.outputPeak==0,"silence incorrectly reports GR");
 meter.prepare(48000);for(int i=0;i<480;++i)meter.push(.5f,.2f,-.7f,frame);
 require(frame.endSample==480&&frame.generation==2&&frame.minimumGain==.5f&&frame.outputPeak==.7f,"reset, GR or stereo peak incorrect");
 for(bool order:{false,true}){
  masterlab::RoutedEngine engine;engine.parameters(1000,0,1,-6,true,true);engine.prepare(48000,order);
  std::vector<float> l(24000,1),r(24000,-1),trace(24000);engine.process(l.data(),r.data(),24000,order,trace.data());
  float expected=std::pow(10.f,-6.f/20);
  for(int i=10000;i<24000;++i){require(std::abs(trace[i]-expected)<1e-5f,"reported limiter gain differs from actual attenuation");require(std::abs(l[i]-trace[i])<1e-5f&&std::abs(r[i]+trace[i])<1e-5f,"envelope does not match limited steady signal");}
  engine.parameters(1000,0,1,-6,true,false);std::fill(l.begin(),l.end(),1);std::fill(r.begin(),r.end(),1);engine.process(l.data(),r.data(),24000,order,trace.data());require(trace.back()>.999f,"bypass still reports gain reduction");
 }
 // Drive must boost the wet path, disappear under bypass, and leave latency fixed.
 masterlab::Engine boosted;boosted.limiterParameters(6,80);boosted.parameters(1000,0,1,0,false,true);boosted.prepare(48000);
 std::vector<float> left(24000,.1f),right(24000,-.1f),gain(24000);
 boosted.process(left.data(),right.data(),24000,false,gain.data());
 require(std::abs(left.back()-.1f*std::pow(10.f,6.f/20))<1e-5f&&gain.back()==1,"drive does not boost quiet wet signal");
 boosted.parameters(1000,0,1,0,false,false);std::fill(left.begin(),left.end(),.1f);std::fill(right.begin(),right.end(),-.1f);boosted.process(left.data(),right.data(),24000,false,gain.data());
 require(std::abs(left.back()-.1f)<1e-5f&&boosted.latency()==240,"drive leaks into bypass or changes latency");
 // Verify release affects the actual detector recovery, not just the displayed number.
 auto recovery=[](float milliseconds){masterlab::Engine e;e.limiterParameters(0,milliseconds);e.parameters(1000,0,1,-6,false,true);e.prepare(48000);
  std::vector<float> l(4800,2),r(4800,2),trace(4800);e.process(l.data(),r.data(),4800,false,trace.data());
  std::fill(l.begin(),l.end(),.01f);std::fill(r.begin(),r.end(),.01f);e.process(l.data(),r.data(),4800,false,trace.data());return trace.back();};
 require(recovery(10)>.99f&&recovery(1000)<.4f,"release does not change envelope recovery");
 std::cout<<"Passed: 10 ms sample clock, variable host blocks, stereo peak, reset, silence, actual limiter envelope, both routes and bypass\n";
}
