#include "ModularDsp.h"
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>

static thread_local bool audit=false;static thread_local size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}

int main(){
 std::cout<<"rate,legacy_gr,strong_gr,legacy_rms,strong_rms,punch_db\n";
 for(double rate:{44100.,48000.,96000.,192000.}){
  masterlab::DynamicsSettings base;base.enabled=true;base.threshold=-30;base.ratio=6;base.attack=12;base.release=100;base.knee=3;
  masterlab::SmartCompressor legacy,strong;legacy.settings(base);strong.settings(base);base.intensity=2;strong.settings(base);legacy.prepare(rate);strong.prepare(rate);
  std::array<float,257> l{},r{},sl{},sr{};double legacyEnergy=0,strongEnergy=0;
  for(int block=0;block<450;++block){for(int i=0;i<257;++i){const double t=(block*257+i)/rate;l[i]=r[i]=.45f*std::sin(6.283185307179586*997*t);}sl=l;sr=r;audit=true;legacy.process(l.data(),r.data(),257);strong.process(sl.data(),sr.data(),257);audit=false;if(block>250)for(int i=0;i<257;++i){legacyEnergy+=l[i]*l[i];strongEnergy+=sl[i]*sl[i];}}
  const auto legacyRead=legacy.read(),strongRead=strong.read();check(strongRead.reduction>legacyRead.reduction*1.3f,"Strong mode does not increase measured gain reduction");check(strongEnergy<legacyEnergy*.65,"Strong mode does not create an audible compression difference");

  masterlab::DynamicsSettings wetSettings;base.intensity=0;wetSettings=base;wetSettings.mix=1;masterlab::DynamicsSettings mixedSettings=wetSettings;mixedSettings.mix=.35f;masterlab::DynamicsSettings deltaSettings=wetSettings;deltaSettings.delta=true;masterlab::SmartCompressor wet,mixed,delta;wet.settings(wetSettings);mixed.settings(mixedSettings);delta.settings(deltaSettings);for(auto* compressor:{&wet,&mixed,&delta})compressor->prepare(rate);double mixError=0,deltaError=0;
  for(int block=0;block<350;++block){std::array<float,257> dryL{},dryR{},wetL{},wetR{},mixL{},mixR{},deltaL{},deltaR{};for(int i=0;i<257;++i){const double t=(block*257+i)/rate;dryL[i]=dryR[i]=.32f*std::sin(6.283185307179586*431*t);}wetL=mixL=deltaL=dryL;wetR=mixR=deltaR=dryR;wet.process(wetL.data(),wetR.data(),257);mixed.process(mixL.data(),mixR.data(),257);delta.process(deltaL.data(),deltaR.data(),257);if(block>200)for(int i=0;i<257;++i){mixError=std::max(mixError,std::abs(static_cast<double>(mixL[i]-(dryL[i]+.35f*(wetL[i]-dryL[i])))));deltaError=std::max(deltaError,std::abs(static_cast<double>(deltaL[i]-(wetL[i]-dryL[i]))));}}
  check(mixError<2e-6,"Dry/Wet is not true parallel compression");check(deltaError<2e-6,"Dynamics Delta is not processed minus input");

  masterlab::DynamicsSettings plainSettings;plainSettings.enabled=true;plainSettings.ratio=1;plainSettings.attack=30;plainSettings.release=120;auto punchSettings=plainSettings;punchSettings.punch=100;punchSettings.intensity=2;masterlab::SmartCompressor plain,punch;plain.settings(plainSettings);punch.settings(punchSettings);plain.prepare(rate);punch.prepare(rate);float plainPeak=0,punchPeak=0,punchReading=0;
  for(int sample=0;sample<static_cast<int>(rate);sample+=257){const int n=std::min(257,static_cast<int>(rate)-sample);std::array<float,257> a{},b{};for(int i=0;i<n;++i){const int absolute=sample+i;a[i]=b[i]=(absolute%static_cast<int>(rate*.1)==0)?.75f:.04f*std::sin(absolute*.17f);}audit=true;plain.process(a.data(),a.data(),n);punch.process(b.data(),b.data(),n);audit=false;for(int i=0;i<n;++i){plainPeak=std::max(plainPeak,std::abs(a[i]));punchPeak=std::max(punchPeak,std::abs(b[i]));}punchReading=std::max(punchReading,punch.read().punch);}
  check(punchReading>1&&punchPeak>plainPeak*1.03f,"Punch does not preserve and lift transient contrast");
  std::array<float,257> silence{};punch.process(silence.data(),silence.data(),257);for(float sample:silence)check(sample==0,"Dynamics generates output from silence");
  masterlab::SmartCompressor automated;masterlab::DynamicsSettings automatedSettings=base;automatedSettings.threshold=-24;automatedSettings.ratio=8;automatedSettings.attack=2;automatedSettings.release=60;automated.prepare(rate);float previous=.4f,maxAutomationStep=0;
  for(int block=0;block<80;++block){automatedSettings.intensity=(block/8)%4;automatedSettings.punch=static_cast<float>((block*13)%101);automatedSettings.delta=(block/10)%2!=0;automated.settings(automatedSettings);std::array<float,257> constant{};constant.fill(.4f);automated.process(constant.data(),constant.data(),257);for(float output:constant){check(std::isfinite(output),"Dynamics automation produced a non-finite sample");maxAutomationStep=std::max(maxAutomationStep,std::abs(output-previous));previous=output;}}
  check(maxAutomationStep<.08f,"Intensity, Punch or Delta automation produced a click-sized discontinuity");
  std::cout<<rate<<','<<legacyRead.reduction<<','<<strongRead.reduction<<','<<std::sqrt(legacyEnergy)<<','<<std::sqrt(strongEnergy)<<','<<punchReading<<'\n';
 }
 check(allocations==0,"Phase 2B dynamics allocated on the audio thread");
}
