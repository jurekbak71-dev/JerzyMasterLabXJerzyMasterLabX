#include "Dsp.h"
#include <iostream>
#include <stdexcept>
void check(bool pass,const char* message){if(!pass)throw std::runtime_error(message);}
int main(){try{
 for(double rate:{44100.,48000.,96000.,192000.})for(double frequency:{50.,200.,1000.,5000.,18000.}){
  masterlab::Biquad filter;filter.peak(rate,1200,9,.7);
  const double predicted=filter.magnitude(rate,frequency);double energy=0;
  for(int i=0;i<static_cast<int>(rate);++i){float y=filter.tick(static_cast<float>(.1*std::sin(6.283185307179586*frequency*i/rate)));if(i>=static_cast<int>(rate/2))energy+=y*y;}
  double measured=std::sqrt(energy/(rate/2)*2)/.1;
  check(std::abs(measured-predicted)<predicted*.001,"displayed filter response disagrees with measured audio gain");
 }
 masterlab::Engine engine;engine.prepare(48000);engine.parameters(1000,12,1,-1,true,false);
 std::array<masterlab::Biquad,16> coefficients;std::array<float,128> left{},right{};
 engine.process(left.data(),right.data(),128);engine.copyEqCoefficients(coefficients);
 check(coefficients[0].magnitude(48000,1000)<3.8,"response snapshot must reflect smoothing, not jump to target gain");
 for(int i=0;i<200;++i)engine.process(left.data(),right.data(),128);
 engine.copyEqCoefficients(coefficients);check(std::abs(coefficients[0].magnitude(48000,1000)-std::pow(10.,12./20))<.001,"settled audio response");
 std::cout<<"Passed: analytical response matches measured sine gain at 20 rate/frequency combinations; coefficient snapshots follow actual smoothing\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
