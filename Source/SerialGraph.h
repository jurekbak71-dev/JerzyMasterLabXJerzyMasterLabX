#pragma once
#include "Stabilizer.h"
namespace masterlab {
using ModuleOrder=std::array<int,6>;
// A single automatable integer encodes a permutation, so hosts cannot restore
// duplicate or incomplete routing. Legacy order keeps its original meaning.
inline ModuleOrder decodeOrder(int code,bool limiterFirst=false) noexcept {
 std::array<int,5> pool{0,1,2,3,4};ModuleOrder out{};code=std::clamp(code,0,719);int position=code/120;code%=120;int remaining=5;
 for(int i=0;i<6;++i){if(i==position){out[i]=5;continue;}int factorial=1;for(int k=2;k<remaining;++k)factorial*=k;int pick=code/factorial;code%=factorial;out[i]=pool[pick];for(int k=pick;k<remaining-1;++k)pool[k]=pool[k+1];--remaining;}
 if(limiterFirst)for(auto& kind:out)if(kind==3)kind=4;else if(kind==4)kind=3;
 return out;
}
inline int encodeOrder(ModuleOrder order,bool limiterFirst=false) noexcept {
 if(limiterFirst)for(auto& kind:order)if(kind==3)kind=4;else if(kind==4)kind=3;
 std::array<int,5> pool{0,1,2,3,4};int code=0,remaining=5;
 for(int i=0;i<6;++i){int kind=order[i];if(kind==5){code+=120*i;continue;}int pick=0;while(pick<remaining&&pool[pick]!=kind)++pick;if(pick==remaining)return 0;int factorial=1;for(int k=2;k<remaining;++k)factorial*=k;code+=pick*factorial;for(int k=pick;k<remaining-1;++k)pool[k]=pool[k+1];--remaining;}
 return code;
}
class SerialGraph {
 struct Bank {
  Engine engine;StereoImager stereo;SmartCompressor dynamics;DynamicEqualizer dynamicEq;Stabilizer stabilizer;
  ModuleOrder order{5,0,1,2,3,4};
  bool parallel=false;std::array<bool,6> present{};
  std::array<std::array<std::vector<float>,2>,6> alignment;size_t alignPosition=0;
  std::array<std::array<float,128>,2> branch{},sum{};std::array<float,128> branchTrace{};
  void parallelProcess(float* l,float* r,int n,float* trace)noexcept{
   for(auto& channel:sum)std::fill_n(channel.data(),n,0.f);std::fill_n(trace,n,1.f);int branches=0;
   for(int kind:order)if(present[kind]){++branches;std::copy_n(l,n,branch[0].data());std::copy_n(r,n,branch[1].data());
    auto* a=branch[0].data();auto* b=branch[1].data();
    switch(kind){case 5:stabilizer.process(a,b,n);break;case 0:dynamicEq.process(a,b,n);break;case 1:dynamics.process(a,b,n);break;case 2:stereo.process(a,b,n);break;case 3:engine.process(a,b,n,false,nullptr,1);break;case 4:engine.process(a,b,n,false,trace,2);break;}
    size_t position=alignPosition;for(int i=0;i<n;++i){if(kind!=4){size_t next=(position+1)%alignment[kind][0].size();for(int c=0;c<2;++c){float x=branch[c][i];branch[c][i]=alignment[kind][c][next];alignment[kind][c][position]=x;}position=next;}sum[0][i]+=branch[0][i];sum[1][i]+=branch[1][i];}
   }
   if(!branches){engine.process(l,r,n,false,trace,2);return;}
   for(int i=0;i<n;++i){l[i]=clean(sum[0][i]/branches);r[i]=clean(sum[1][i]/branches);trace[i]=1+(trace[i]-1)/branches;}
   alignPosition=(alignPosition+static_cast<size_t>(n))%alignment[0][0].size();
  }
  void process(float* l,float* r,int n,float* trace) noexcept {
   if(parallel){parallelProcess(l,r,n,trace);return;}
   for(int kind:order)switch(kind){
    case 5:stabilizer.process(l,r,n);break;case 0:dynamicEq.process(l,r,n);break;case 1:dynamics.process(l,r,n);break;case 2:stereo.process(l,r,n);break;
    case 3:engine.process(l,r,n,false,nullptr,1);break;case 4:engine.process(l,r,n,false,trace,2);break;
   }
  }
 };
 std::array<Bank,2> banks;int active=0;float progress=1,step=1.f/960;
 ModuleOrder requested{5,0,1,2,3,4};bool requestedParallel=false;std::array<bool,6> requestedPresent{};
 std::array<std::array<float,128>,2> scratch{};std::array<float,128> trace{};
public:
 void prepare(double rate,bool legacy=false,int code=0){rate=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:48000;requested=decodeOrder(code,legacy);active=0;progress=1;step=static_cast<float>(1/(rate*.02));for(auto& b:banks){b.order=requested;b.engine.prepare(rate);b.stereo.prepare(rate);b.dynamics.prepare(rate);b.dynamicEq.prepare(rate);b.stabilizer.prepare(rate);b.parallel=requestedParallel;b.present=requestedPresent;b.alignPosition=0;for(auto& module:b.alignment)for(auto& channel:module)channel.assign(static_cast<size_t>(b.engine.latency()+1),0);}}
 void reset()noexcept{progress=1;for(auto& b:banks){b.order=requested;b.engine.reset();b.stereo.reset();b.dynamics.reset();b.dynamicEq.reset();b.stabilizer.reset();b.parallel=requestedParallel;b.present=requestedPresent;b.alignPosition=0;for(auto& module:b.alignment)for(auto& channel:module)std::fill(channel.begin(),channel.end(),0);}}
 int latency()const noexcept{return banks[active].engine.latency();}
 void parameters(float hz,float db,float q,float ceiling,bool eq,bool lim)noexcept{for(auto& b:banks)b.engine.parameters(hz,db,q,ceiling,eq,lim);}
 void limiterParameters(float drive,float release)noexcept{for(auto& b:banks)b.engine.limiterParameters(drive,release);}
 void setBand(size_t i,float hz,float db,float q)noexcept{for(auto& b:banks)b.engine.setBand(i,hz,db,q);}
 void setBandMode(size_t i,FilterType t,bool on)noexcept{for(auto& b:banks)b.engine.setBandMode(i,t,on);}
 void stereoSettings(StereoSettings c)noexcept{for(auto& b:banks)b.stereo.settings(c);}
 void dynamicsSettings(DynamicsSettings c)noexcept{for(auto& b:banks)b.dynamics.settings(c);}
 void dynamicEqSettings(DynamicEqSettings c)noexcept{for(auto& b:banks)b.dynamicEq.settings(c);}
 void stabilizerSettings(StabilizerSettings c)noexcept{for(auto& b:banks)b.stabilizer.settings(c);}
 void topology(bool parallel,std::array<bool,6> present)noexcept{requestedParallel=parallel;requestedPresent=present;}
 void routing(int code,bool legacy)noexcept{requested=decodeOrder(code,legacy);}
 void process(float* l,float* r,int count,bool /* legacy supplied through routing */,float* gainTrace)noexcept{
  for(int offset=0;offset<count;){int n=std::min(128,count-offset);
   if(progress>=1&&(banks[active].order!=requested||banks[active].parallel!=requestedParallel||(requestedParallel&&banks[active].present!=requestedPresent))){
    // Both banks were prepared with equal vector capacities. Assignment reuses
    // their storage; allocation auditing covers changes while audio is running.
    banks[1-active]=banks[active];banks[1-active].order=requested;banks[1-active].parallel=requestedParallel;banks[1-active].present=requestedPresent;progress=0;
   }
   if(progress<1){std::copy_n(l+offset,n,scratch[0].data());std::copy_n(r+offset,n,scratch[1].data());banks[1-active].process(scratch[0].data(),scratch[1].data(),n,trace.data());}
   banks[active].process(l+offset,r+offset,n,gainTrace+offset);
   if(progress<1){for(int i=0;i<n;++i){progress=std::min(1.f,progress+step);l[offset+i]+=progress*(scratch[0][i]-l[offset+i]);r[offset+i]+=progress*(scratch[1][i]-r[offset+i]);gainTrace[offset+i]+=progress*(trace[i]-gainTrace[offset+i]);}if(progress>=1)active=1-active;}
   offset+=n;
  }
 }
 std::array<float,12> stabilizerRead()const noexcept{return banks[active].stabilizer.read();}
 void stabilizerCoefficients(std::array<Biquad,12>& out)const noexcept{banks[active].stabilizer.coefficients(out);}
 float stabilizerBlend()const noexcept{return banks[active].stabilizer.blend();}
 float eqBlend()const noexcept{return banks[active].engine.eqBlend();}
 void copyEqCoefficients(std::array<Biquad,16>& out)const noexcept{banks[active].engine.copyEqCoefficients(out);}
 void copyBandBlends(std::array<float,16>& out)const noexcept{banks[active].engine.copyBandBlends(out);}
 StereoReading stereoRead()const noexcept{return banks[active].stereo.read();}
 DynamicsReading dynamicsRead()const noexcept{return banks[active].dynamics.read();}
 std::array<float,3> dynamicRead()const noexcept{return banks[active].dynamicEq.read();}
 std::array<float,3> dynamicBlends()const noexcept{return banks[active].dynamicEq.bandBlends();}
 void dynamicCoefficients(std::array<Biquad,3>& out)const noexcept{banks[active].dynamicEq.coefficients(out);}
};
}
