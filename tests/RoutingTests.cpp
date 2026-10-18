#include "Dsp.h"
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(double rate:{44100.,48000.,96000.,192000.}){
  masterlab::RoutedEngine routed;masterlab::Engine forward,reverse;
  routed.prepare(rate);forward.prepare(rate);reverse.prepare(rate);
  auto configure=[](auto& e){e.parameters(1000,12,1,-6,true,true);};
  configure(routed);configure(forward);configure(reverse);
  const int count=static_cast<int>(rate*.04);
  std::vector<float> l(count),r(count);
  for(int i=0;i<count;++i)l[i]=r[i]=.5f*std::sin(static_cast<float>(i*.13));
  // Warm both orders with identical input before requesting a new order.
  for(int block=0;block<8;++block){auto a=l,b=r,c=l,d=r;routed.process(a.data(),b.data(),count);forward.process(c.data(),d.data(),count);c=l;d=r;reverse.process(c.data(),d.data(),count,true);}
  auto a=l,b=r,c=l,d=r,e=l,f=r;
  forward.process(c.data(),d.data(),count);reverse.process(e.data(),f.data(),count,true);routed.process(a.data(),b.data(),count,true);
  float mix=0,step=static_cast<float>(1/(rate*.02));
  for(int i=0;i<count;++i){mix=std::min(1.f,mix+step);const float expected=c[i]+mix*(e[i]-c[i]);check(std::abs(a[i]-expected)<1e-6f,"reorder must crossfade two warm, latency-aligned paths");}
  check(routed.latency()==forward.latency(),"reorder must preserve reported latency");
  // Reset establishes the currently selected path without replaying old filter/delay state.
  routed.reset();reverse.reset();a=l;b=r;c=l;d=r;routed.process(a.data(),b.data(),count,true);reverse.process(c.data(),d.data(),count,true);
  for(int i=0;i<count;++i)check(std::abs(a[i]-c[i])<1e-6f,"reset preserves selected order");
  masterlab::RoutedEngine split,whole;split.prepare(rate);whole.prepare(rate);configure(split);configure(whole);a=l;b=r;c=l;d=r;
  whole.process(a.data(),b.data(),count,true);for(int i=0;i<count;){int n=std::min(37,count-i);split.process(c.data()+i,d.data()+i,n,true);i+=n;}
  check(a==c&&b==d,"route crossfade independent of host block boundaries");
 }
 std::cout<<"Passed: warm-path crossfade, fixed PDC, reset, variable blocks at four sample rates\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
