#include "Stabilizer.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>

static thread_local bool audit=false;static thread_local size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
struct Result {float correction=0,peak=0;double rms=0;};

Result render(double rate,int intensity,bool tame,int stimulus){
 masterlab::StabilizerSettings settings;settings.enabled=true;settings.amount=100;settings.speed=0;settings.smoothing=25;settings.intensity=intensity;settings.tame=tame;masterlab::Stabilizer stabilizer;stabilizer.settings(settings);stabilizer.prepare(rate);Result result;std::array<float,257> left{},right{};std::uint64_t samples=0;
 for(int block=0;block<static_cast<int>(rate*3/257);++block){for(int i=0;i<257;++i){const auto sample=block*257+i;const double t=sample/rate;float value=stimulus==1?.3f*std::sin(6.283185307179586*1250*t):.2f*std::sin(6.283185307179586*125*t)+.12f*std::sin(6.283185307179586*700*t)+.08f*std::cos(6.283185307179586*4000*t)+.05f*std::sin(6.283185307179586*12000*t);if(stimulus==2&&sample%static_cast<int>(rate*.1)==0)value+=.9f;left[i]=value;right[i]=value*.93f;}audit=true;stabilizer.process(left.data(),right.data(),257);audit=false;if(block>rate/257){for(int i=0;i<257;++i){check(std::isfinite(left[i])&&std::isfinite(right[i]),"Stabilizer produced a non-finite sample");result.peak=std::max(result.peak,std::max(std::abs(left[i]),std::abs(right[i])));result.rms+=static_cast<double>(left[i])*left[i]+static_cast<double>(right[i])*right[i];samples+=2;}for(float gain:stabilizer.read())result.correction=std::max(result.correction,std::abs(gain));}}
 result.rms=std::sqrt(result.rms/std::max<std::uint64_t>(1,samples));return result;
}

int main(){
 std::cout<<"rate,clean_db,strong_db,extreme_db,single_extreme_db,tame_db,untamed_db\n";
 for(double rate:{44100.,48000.,96000.,192000.}){
  const auto clean=render(rate,1,false,0),strong=render(rate,2,false,0),extreme=render(rate,3,false,0),single=render(rate,3,false,1),tame=render(rate,3,true,2),untamed=render(rate,3,false,2);
  std::cout<<rate<<','<<clean.correction<<','<<strong.correction<<','<<extreme.correction<<','<<single.correction<<','<<tame.correction<<','<<untamed.correction<<'\n';
  check(clean.correction<=4.001f&&strong.correction<=9.001f&&extreme.correction<=12.001f,"A Stabilizer profile exceeded its correction bound");
  check(strong.correction>clean.correction*1.15f&&extreme.correction>strong.correction*1.15f,"Strong and Extreme do not add measured correction authority");
  check(single.correction<7.f,"Narrow-band support protection lets a single tone approach the Extreme limit");
  check(tame.correction<untamed.correction*.8f,"Continuous transient protection does not reduce adaptive movement");
  masterlab::Stabilizer nearSilence;masterlab::StabilizerSettings settings;settings.enabled=true;settings.amount=100;settings.intensity=3;nearSilence.settings(settings);nearSilence.prepare(rate);std::array<float,257> tiny{};for(size_t i=0;i<tiny.size();++i)tiny[i]=(i%2?1.f:-1.f)*1e-10f;for(int block=0;block<100;++block)nearSilence.process(tiny.data(),tiny.data(),257);for(float sample:tiny)check(std::isfinite(sample)&&std::abs(sample)<2e-10f,"Near silence became unstable");
  masterlab::Stabilizer automated;settings.tame=false;automated.settings(settings);automated.prepare(rate);std::array<float,257> constant{};float previous=.2f,maxStep=0;for(int block=0;block<120;++block){settings.intensity=1+block/10%3;settings.profile=block/15%3;settings.cut=block/20%2;automated.settings(settings);constant.fill(.2f);automated.process(constant.data(),constant.data(),257);for(float sample:constant){check(std::isfinite(sample),"Stabilizer automation produced a non-finite sample");maxStep=std::max(maxStep,std::abs(sample-previous));previous=sample;}}check(maxStep<.04f,"Stabilizer mode automation produced a click-sized discontinuity");
 }
 check(allocations==0,"Phase 2C Stabilizer allocated on the audio thread");
}
