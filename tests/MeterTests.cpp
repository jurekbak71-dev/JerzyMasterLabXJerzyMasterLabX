#include "WindowMeter.h"
#include <iostream>
#include <cstdlib>
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 constexpr double pi=3.14159265358979323846;
 for(double rate:{44100.,48000.,96000.,192000.}){
  masterlab::WindowMeter mono,anti,right;mono.prepare(rate);anti.prepare(rate);right.prepare(rate);
  for(int i=0;i<rate;++i){float x=static_cast<float>(.5*std::sin(2*pi*1000*i/rate));mono.push(x,x);anti.push(x,-x);right.push(0,x);}
  auto m=mono.read(),a=anti.read(),r=right.read();
  check(m.ready&&std::abs(m.rms-.5/std::sqrt(2.))<1e-6&&std::abs(m.peak-.5)<.001,"RMS or peak calibration");
  check(std::abs(m.correlation-1)<1e-6&&std::abs(m.balance)<1e-6&&m.sideRms<1e-6&&std::abs(m.midRms-m.rms)<1e-6,"mono or M/S calibration");
  check(std::abs(a.correlation+1)<1e-6&&a.midRms<1e-6&&std::abs(a.sideRms-a.rms)<1e-6,"opposite-phase M/S calibration");
  check(r.balance==1&&r.correlation==0&&std::abs(r.rms-.25)<1e-6,"single-channel balance or RMS");
 }
 masterlab::WindowMeter meter;meter.prepare(48000);meter.push(1.5f,-.5f);auto clipped=meter.read();check(clipped.peak==1.5f&&clipped.clippedSamples==1&&!clipped.ready,"clip count or warmup");
 for(int i=0;i<19199;++i)meter.push(0,0);check(meter.read().peak==1.5f,"sliding peak expired early");meter.push(0,0);
 auto silence=meter.read();check(silence.peak==0&&silence.rms<1e-9&&silence.clippedSamples==0&&silence.correlation==0,"400 ms peak expiration or silence");
 meter.reset();check(meter.read().peak==0&&!meter.read().ready,"reset did not clear meter");
 // Compare arbitrary block partitions and a brute-force window after a long changing signal.
 for(int block:{1,257,4097}){
  meter.prepare(48000);double ll=0,rr=0,lr=0;float peak=0;unsigned clips=0;
  const int total=240000,window=19200;
  for(int offset=0;offset<total;offset+=block)for(int i=offset;i<std::min(total,offset+block);++i){float l=1.1f*std::sin(i*.17f),r=.7f*std::cos(i*.21f);meter.push(l,r);if(i>=total-window){ll+=static_cast<double>(l)*l;rr+=static_cast<double>(r)*r;lr+=static_cast<double>(l)*r;peak=std::max(peak,std::max(std::abs(l),std::abs(r)));clips+=std::max(std::abs(l),std::abs(r))>1?1:0;}}
  auto reading=meter.read();check(std::abs(reading.rms-std::sqrt((ll+rr)/(2*window)))<1e-6&&std::abs(reading.correlation-lr/std::sqrt(ll*rr))<1e-6&&reading.peak==peak&&reading.clippedSamples==clips,"block partitions or running sums differ from full window");
 }
 std::cout<<"Passed: four sample rates, calibrated RMS/peak, mono and antiphase M/S, balance, exact sliding peak/clip expiration, reset and variable blocks\n";
}
