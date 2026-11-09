#include "Analogizer.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
static bool auditing=false;static size_t allocations=0;
void* operator new(size_t n){if(auditing)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,size_t)noexcept{std::free(p);}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete[](void* p,size_t)noexcept{std::free(p);}
static void require(bool ok,const char* msg){if(!ok){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
constexpr double pi=3.14159265358979323846;
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  for(int mode=0;mode<3;++mode){
   masterlab::AnalogizerSettings cfg;cfg.enabled=true;
   cfg.mode=static_cast<masterlab::AnalogizerMode>(mode);
   cfg.driveDb=9;cfg.warmth=.4f;cfg.deFizz=.7f;cfg.mix=.75f;
   masterlab::Analogizer a,b;a.settings(cfg);b.settings(cfg);a.prepare(sr);b.prepare(sr);
   constexpr int n=16384;std::array<float,n> l{},r{},reference{},rhs{};
   for(int i=0;i<n;++i)l[i]=r[i]=reference[i]=rhs[i]=.3f*std::sin(2*pi*997*i/sr);
   auditing=true;
   for(int i=0;i<n;i+=257)a.process(l.data()+i,r.data()+i,std::min(257,n-i));
   b.process(reference.data(),rhs.data(),n);
   auditing=false;
   double delta=0;
   for(int i=0;i<n;++i){
    require(std::isfinite(l[i])&&l[i]==r[i],"finite dual mono");
    require(std::abs(l[i]-reference[i])<1e-6f,"buffer boundary invariant");
    delta+=std::abs(l[i]-.3f*std::sin(2*pi*997*i/sr));
   }
   require(delta/n>1.e-4,"processor has real audible nonlinear effect");
   require(a.latencySamples()==0,"no added PDC");
  }
  masterlab::Analogizer off;off.prepare(sr);
  float l[2]{.19f,-.37f},r[2]{-.19f,.37f};
  off.process(l,r,2);
  require(l[0]==.19f&&l[1]==-.37f&&r[0]==-.19f&&r[1]==.37f,"disabled exact bypass");
 }
 require(allocations==0,"callback allocated memory");
 std::puts("Analogizer: three distinct modes, full bypass, block invariance and 0 extra latency PASS");
}
