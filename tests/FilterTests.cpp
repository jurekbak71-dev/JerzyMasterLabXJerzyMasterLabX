#include "Dsp.h"
#include <cstdlib>
#include <iostream>
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 using masterlab::FilterType;constexpr double pi=3.14159265358979323846;
 for(double rate:{44100.,48000.,96000.,192000.})for(int kind=0;kind<6;++kind){
  masterlab::Biquad b;b.configure(rate,1200,6,.70710678,static_cast<FilterType>(kind));
  for(double hz:{71.,411.,2300.,17000.}){
   b.reset();double energy=0,inputEnergy=0;
   for(int i=0;i<static_cast<int>(rate*.7);++i){float x=static_cast<float>(.05*std::sin(2*pi*hz*i/rate));float y=b.tick(x);check(std::isfinite(y),"non-finite filter output");if(i>rate*.5){energy+=y*y;inputEnergy+=x*x;}}
   double measured=std::sqrt(energy/inputEnergy),predicted=b.magnitude(rate,hz);
   check(std::abs(measured-predicted)<.002*std::max(1.,predicted),"actual filter differs from displayed transfer");
  }
 }
 masterlab::Biquad b;
 b.configure(48000,1000,6,.70710678,FilterType::LowShelf);check(std::abs(20*std::log10(b.magnitude(48000,1))-6)<.001&&std::abs(b.magnitude(48000,23000)-1)<.001,"low shelf asymptotes");
 b.configure(48000,1000,6,.70710678,FilterType::HighShelf);check(std::abs(b.magnitude(48000,1)-1)<.001&&std::abs(20*std::log10(b.magnitude(48000,23000))-6)<.001,"high shelf asymptotes");
 b.configure(48000,1000,0,.70710678,FilterType::HighPass);check(b.magnitude(48000,10)<.001&&std::abs(20*std::log10(b.magnitude(48000,1000))+3.0103)<.001,"high pass cutoff");
 b.configure(48000,1000,0,.70710678,FilterType::LowPass);check(b.magnitude(48000,23000)<.001&&std::abs(20*std::log10(b.magnitude(48000,1000))+3.0103)<.001,"low pass cutoff");
 b.configure(48000,1000,0,2,FilterType::Notch);check(b.magnitude(48000,1000)<1e-8&&b.magnitude(48000,10)>.999,"notch cancellation");
 for(double rate:{44100.,192000.})for(int kind=0;kind<6;++kind)for(float q:{.1f,10.f})for(float db:{-18.f,18.f}){
  masterlab::RoutedEngine engine;engine.parameters(1000,db,q,0,true,false);engine.setBandMode(0,static_cast<FilterType>(kind),true);engine.prepare(rate);
  std::vector<float> l(257),r(257);
  for(int block=0;block<120;++block){engine.setBandMode(0,static_cast<FilterType>((kind+block/10)%6),block%20<10);
   for(int i=0;i<257;++i){l[i]=.001f*std::sin((block*257+i)*.17f);r[i]=-l[i];}engine.process(l.data(),r.data(),257);
   for(float y:l)check(std::isfinite(y)&&std::abs(y)<5,"filter type or bypass transition unstable");
  }
 }
 masterlab::Engine engine;engine.parameters(1000,12,1,0,true,false);engine.setBandMode(0,FilterType::LowPass,false);engine.prepare(48000);
 std::vector<float> l(24000,.1f),r(24000,.1f);engine.process(l.data(),r.data(),24000);
 check(std::abs(l.back()-.1f)<1e-6,"disabled band still filters audio");
 engine.setBandMode(0,FilterType::Bell,true);std::fill(l.begin(),l.end(),.1f);std::fill(r.begin(),r.end(),.1f);engine.process(l.data(),r.data(),24000);
 std::array<float,16> mixes;engine.copyBandBlends(mixes);check(mixes[0]>.999,"band bypass did not re-enable");
 std::cout<<"Passed: 96 measured response comparisons, shelf asymptotes, 12 dB/oct cutoffs, notch, extreme Q/gain/rate transitions and band bypass\n";
}
