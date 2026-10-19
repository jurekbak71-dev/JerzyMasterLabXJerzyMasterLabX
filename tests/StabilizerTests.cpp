#include "Stabilizer.h"
#include <cstdlib>
#include <iostream>
#include <new>
static bool audit=false;static size_t allocations=0;
void* operator new(std::size_t n){if(audit)++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){for(double rate:{44100.,48000.,96000.,192000.}){
 masterlab::StabilizerSettings settings;settings.enabled=true;settings.cut=true;settings.amount=100;settings.speed=0;masterlab::Stabilizer s;s.settings(settings);s.prepare(rate);
 std::array<float,257> l{},r{};double input=0,output=0;
 for(int block=0;block<static_cast<int>(rate*2/257);++block){for(int i=0;i<257;++i)l[i]=r[i]=.2f*std::sin(6.283185307179586*1250*(block*257+i)/rate);auto before=l;audit=true;s.process(l.data(),r.data(),257);audit=false;if(block>rate/257)for(int i=0;i<257;++i){input+=before[i]*before[i];output+=l[i]*l[i];check(std::isfinite(l[i])&&l[i]==r[i],"Stabilizer invalid or unlinked audio");}}
 check(10*std::log10(output/input)<-2,"Stabilizer does not attenuate excess band energy");for(float gain:s.read())check(gain<=1e-6&&gain>=-6.001f,"Cut mode boosts or exceeds gain bound");
 settings.amount=0;s.settings(settings);s.reset();for(int i=0;i<257;++i){l[i]=.1f*std::sin(i*.12f);r[i]=.08f*std::cos(i*.03f);}auto beforeL=l,beforeR=r;s.process(l.data(),r.data(),257);for(int i=0;i<257;++i)check(l[i]==beforeL[i]&&r[i]==beforeR[i],"Amount zero is not transparent");
 settings.amount=100;settings.cut=false;s.settings(settings);s.reset();l.fill(0);r.fill(0);s.process(l.data(),r.data(),257);for(float x:l)check(x==0,"Stabilizer creates sound from silence");
 // Detector updates are sample-clocked and therefore independent of buffer length.
 masterlab::Stabilizer a,b,d;settings.profile=1;a.settings(settings);b.settings(settings);settings.delta=true;d.settings(settings);for(auto* p:{&a,&b,&d})p->prepare(rate);
 double residual=0;for(int block=0;block<100;++block){for(int i=0;i<257;++i){l[i]=.1f*std::sin((block*257+i)*.11f);r[i]=.07f*std::cos((block*257+i)*.03f);}auto bl=l,br=r,dl=l,dr=r,originalL=l,originalR=r;a.process(l.data(),r.data(),257);d.process(dl.data(),dr.data(),257);for(int i=0;i<257;++i)b.process(&bl[i],&br[i],1);for(int i=0;i<257;++i){check(l[i]==bl[i]&&r[i]==br[i],"Stabilizer depends on host block size");residual=std::max(residual,static_cast<double>(std::max(std::abs(l[i]+dl[i]-originalL[i]),std::abs(r[i]+dr[i]-originalR[i]))));}}
 check(residual<1e-6,"Delta is not the true input-output difference");settings.enabled=false;settings.delta=false;a.settings(settings);for(int block=0;block<static_cast<int>(rate*.5/257)+1;++block){l.fill(.01f);r=l;a.process(l.data(),r.data(),257);}check(std::abs(l.back()-.01f)<1e-6,"Stabilizer bypass not transparent");
 }check(allocations==0,"Stabilizer allocated in processing");std::cout<<"Passed: real tonal correction, Cut-only gain bounds, amount zero, silence, block invariance, exact Delta, stereo linking, bypass and no allocation at four rates\n";}
