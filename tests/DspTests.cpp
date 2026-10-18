#include "Dsp.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <new>
static bool trackAllocations=false;static int allocations=0;
void* operator new(std::size_t n){if(trackAllocations)++allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(double sr:{44100.,48000.,96000.,192000.}){
 masterlab::Engine e;e.prepare(sr);e.parameters(1000,0,1,-1,true,true);
 std::vector<float> l(4096,2),r=l;e.process(l.data(),r.data(),4096);
 for(float x:l)check(std::isfinite(x)&&std::abs(x)<=.891251f,"ceiling");
 e.reset();l.assign(4096,0);r=l;l[0]=r[0]=.5f;e.process(l.data(),r.data(),4096);
 check(std::abs(l[e.latency()]-.5f)<1e-5,"latency impulse");
 e.reset();l.assign(4096,.2f);r=l;auto a=l,b=r;e.process(l.data(),r.data(),4096);e.reset();
 for(int p=0;p<4096;){int n=std::min(37,4096-p);e.process(a.data()+p,b.data()+p,n);p+=n;}
 check(l==a&&r==b,"variable blocks deterministic");
 l[0]=NAN;r[0]=INFINITY;e.process(l.data(),r.data(),4096);for(float x:l)check(std::isfinite(x),"finite output");
 }
 masterlab::Biquad b;b.peak(48000,1000,6,1);double energy=0;
 for(int i=0;i<48000;++i){auto y=b.tick(std::sin(6.283185307179586*1000*i/48000));if(i>24000)energy+=y*y;}
 check(energy>45000&&energy<50000,"EQ center gain");
 masterlab::Engine multiband;multiband.prepare(48000);multiband.parameters(1000,6,1,-1,true,false);multiband.setBand(1,1000,6,1);
 std::vector<float> left(96000),right(96000);for(size_t i=0;i<left.size();++i)left[i]=right[i]=.01f*std::sin(6.283185307179586*1000*i/48000);
 trackAllocations=true;multiband.process(left.data(),right.data(),static_cast<int>(left.size()));trackAllocations=false;
 check(allocations==0,"C++ allocation in DSP callback");double sum=0;for(size_t i=48000;i<left.size();++i)sum+=left[i]*left[i];
 check(sum>37&&sum<39,"two cascaded bands produce 12dB gain");
 masterlab::RoutedEngine realtime;realtime.prepare(48000);realtime.parameters(1000,6,1,-1,true,true);
 trackAllocations=true;realtime.process(left.data(),right.data(),static_cast<int>(left.size()),true);trackAllocations=false;
 check(allocations==0,"C++ allocation while crossfading route");
 std::cout<<"Passed: ceiling, latency, variable blocks, NaN/Inf at four sample rates; EQ center gain, cascaded bands and C++ allocation guard\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
