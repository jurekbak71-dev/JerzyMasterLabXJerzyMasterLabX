#include "ModularDsp.h"
#include <cstdlib>
#include <iostream>
#include <new>
static bool audit=false;static size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 for(float knee:{0.f,3.f,6.f,18.f})for(float ratio:{1.f,2.f,12.f}){
  for(float boundary:{-18-knee*.5f,-18+knee*.5f}){const float a=masterlab::SmartCompressor::reduction(boundary-.0001f,-18,ratio,knee),b=masterlab::SmartCompressor::reduction(boundary,-18,ratio,knee),c=masterlab::SmartCompressor::reduction(boundary+.0001f,-18,ratio,knee);check(std::abs(a-b)<.001f&&std::abs(b-c)<.001f,"compressor knee has a discontinuity");}
 }

 for(double sr:{44100.,48000.,96000.,192000.}){
  masterlab::StereoImager s;masterlab::StereoSettings c;c.enabled=true;s.settings(c);s.prepare(sr);
  std::array<float,257> l{},r{};double unity=0,monoError=0;
  for(int block=0;block<150;++block){for(int i=0;i<257;++i){l[i]=.2f*std::sin((block*257+i)*.13f);r[i]=.13f*std::cos((block*257+i)*.031f);}auto dl=l,dr=r;audit=true;s.process(l.data(),r.data(),257);audit=false;for(int i=0;i<257;++i)unity=std::max(unity,static_cast<double>(std::max(std::abs(l[i]-dl[i]),std::abs(r[i]-dr[i]))));}
  check(unity<1e-6,"unity stereo widths colour the signal");c.low=0;c.mid=.4f;c.high=1.6f;s.settings(c);
  for(int block=0;block<150;++block){for(int i=0;i<257;++i){l[i]=.2f*std::sin((block*257+i)*.13f);r[i]=.13f*std::cos((block*257+i)*.031f);}auto dl=l,dr=r;s.process(l.data(),r.data(),257);for(int i=0;i<257;++i)monoError=std::max(monoError,static_cast<double>(std::abs(l[i]+r[i]-dl[i]-dr[i])));}
  check(monoError<1e-6,"width changes break mono sum");c.mono=true;c.balance=.7f;s.settings(c);
  for(int block=0;block<200;++block){l.fill(.2f);r.fill(-.1f);s.process(l.data(),r.data(),257);}check(std::abs(l.back()-r.back())<1e-5,"mono preview not mono");
  c={};c.enabled=true;c.monoBass=true;s.settings(c);s.reset();
  for(float hz:{40.f,2000.f}){double source=0,filtered=0;for(int block=0;block<250;++block){for(int i=0;i<257;++i)l[i]=.1f*std::sin(6.283185307179586*hz*(block*257+i)/sr);r=l;for(auto& x:r)x=-x;auto before=l;s.process(l.data(),r.data(),257);if(block>150)for(int i=0;i<257;++i){source+=before[i]*before[i];filtered+=l[i]*l[i];}}check(hz<100?filtered/source<.2:filtered/source>.8,"mono bass is not frequency selective");}
  masterlab::SmartCompressor comp;masterlab::DynamicsSettings d;d.enabled=true;d.threshold=-18;d.ratio=4;d.attack=1;d.release=100;d.knee=0;comp.settings(d);comp.prepare(sr);
  double in=0,out=0;for(int block=0;block<400;++block){for(int i=0;i<257;++i)l[i]=r[i]=.5f*std::sin((block*257+i)*.13f);auto before=l;audit=true;comp.process(l.data(),r.data(),257);audit=false;if(block>200)for(int i=0;i<257;++i){in+=before[i]*before[i];out+=l[i]*l[i];check(l[i]==r[i],"compressor stereo image shifted");}}
  check(10*std::log10(out/in)<-6,"downward compressor does not reduce loud input");d.ratio=1;d.upward=9;d.upwardThreshold=-30;comp.settings(d);comp.reset();in=out=0;
  for(int block=0;block<400;++block){for(int i=0;i<257;++i)l[i]=r[i]=.003f*std::sin((block*257+i)*.13f);auto before=l;comp.process(l.data(),r.data(),257);if(block>200)for(int i=0;i<257;++i){in+=before[i]*before[i];out+=l[i]*l[i];}}
  check(10*std::log10(out/in)>5,"upward does not lift quiet signal");comp.reset();l.fill(0);r.fill(0);comp.process(l.data(),r.data(),257);check(l.back()==0,"upward generates sound from silence");
  check(masterlab::SmartCompressor::upward(-120,-30,18)==0,"upward noise floor protection missing");
  d.upward=0;d.analog=false;comp.settings(d);comp.reset();masterlab::SmartCompressor coloured;d.analog=true;coloured.settings(d);coloured.prepare(sr);double difference=0,dc=0;
  for(int i=0;i<static_cast<int>(sr);++i){float digital=.5f*std::sin(6.283185307179586*1000*i/sr),analog=digital,spare=digital,spare2=analog;comp.process(&digital,&spare,1);coloured.process(&analog,&spare2,1);difference+=std::abs(analog-digital);dc+=analog;}
  check(difference/sr>1e-4&&std::abs(dc/sr)<1e-5,"Analog mode has no colour or adds DC");
  masterlab::DynamicEqualizer eq;masterlab::DynamicEqSettings e;e.enabled=true;for(auto& band:e.bands)band.enabled=false;e.bands[0]={1000,0,2,-30,-9,2,50,true};eq.settings(e);eq.prepare(sr);in=out=0;
  for(int block=0;block<400;++block){for(int i=0;i<257;++i)l[i]=r[i]=.3f*std::sin(6.283185307179586*1000*(block*257+i)/sr);auto before=l;audit=true;eq.process(l.data(),r.data(),257);audit=false;if(block>200)for(int i=0;i<257;++i){in+=before[i]*before[i];out+=l[i]*l[i];}}
  check(10*std::log10(out/in)<-7&&eq.read()[0]<-8,"dynamic EQ detector or reduction not working");
  e.bands[0].range=6;eq.settings(e);eq.reset();in=out=0;for(int block=0;block<300;++block){for(int i=0;i<257;++i)l[i]=r[i]=.2f*std::sin(6.283185307179586*1000*(block*257+i)/sr);auto before=l;eq.process(l.data(),r.data(),257);if(block>150)for(int i=0;i<257;++i){in+=before[i]*before[i];out+=l[i]*l[i];}}check(10*std::log10(out/in)>5,"positive Dynamic EQ range does not boost");
  e.enabled=false;eq.settings(e);for(int i=0;i<400;++i){l.fill(.01f);r=l;eq.process(l.data(),r.data(),257);}check(std::abs(l.back()-.01f)<1e-6,"dynamic EQ bypass not transparent");
 }
 check(allocations==0,"new DSP allocates in audio processing");
 std::cout<<"Passed: stereo unity/mono sum/mono preview, downward and upward compression, silence protection, dynamic EQ reduction/bypass and no callback allocation at four sample rates\n";
}
