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
  void process(float* l,float* r,int n,float* trace) noexcept {
   for(int kind:order)switch(kind){
    case 5:stabilizer.process(l,r,n);break;case 0:dynamicEq.process(l,r,n);break;case 1:dynamics.process(l,r,n);break;case 2:stereo.process(l,r,n);break;
    case 3:engine.process(l,r,n,false,nullptr,1);break;case 4:engine.process(l,r,n,false,trace,2);break;
   }
  }
 };
 std::array<Bank,2> banks;int active=0;float progress=1,step=1.f/960;
 ModuleOrder requested{5,0,1,2,3,4};
 std::array<std::array<float,128>,2> scratch{};std::array<float,128> trace{};
public:
 void prepare(double rate,bool legacy=false,int code=0){rate=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:48000;requested=decodeOrder(code,legacy);active=0;progress=1;step=static_cast<float>(1/(rate*.02));for(auto& b:banks){b.order=requested;b.engine.prepare(rate);b.stereo.prepare(rate);b.dynamics.prepare(rate);b.dynamicEq.prepare(rate);b.stabilizer.prepare(rate);}}
 void reset()noexcept{progress=1;for(auto& b:banks){b.order=requested;b.engine.reset();b.stereo.reset();b.dynamics.reset();b.dynamicEq.reset();b.stabilizer.reset();}}
 int latency()const noexcept{return banks[active].engine.latency();}
 void parameters(float hz,float db,float q,float ceiling,bool eq,bool lim)noexcept{for(auto& b:banks)b.engine.parameters(hz,db,q,ceiling,eq,lim);}
 void limiterParameters(float drive,float release)noexcept{for(auto& b:banks)b.engine.limiterParameters(drive,release);}
 void setBand(size_t i,float hz,float db,float q)noexcept{for(auto& b:banks)b.engine.setBand(i,hz,db,q);}
 void setBandMode(size_t i,FilterType t,bool on)noexcept{for(auto& b:banks)b.engine.setBandMode(i,t,on);}
 void stereoSettings(StereoSettings c)noexcept{for(auto& b:banks)b.stereo.settings(c);}
 void dynamicsSettings(DynamicsSettings c)noexcept{for(auto& b:banks)b.dynamics.settings(c);}
 void dynamicEqSettings(DynamicEqSettings c)noexcept{for(auto& b:banks)b.dynamicEq.settings(c);}
 void stabilizerSettings(StabilizerSettings c)noexcept{for(auto& b:banks)b.stabilizer.settings(c);}
 void routing(int code,bool legacy)noexcept{requested=decodeOrder(code,legacy);}
 void process(float* l,float* r,int count,bool /* legacy supplied through routing */,float* gainTrace)noexcept{
  for(int offset=0;offset<count;){int n=std::min(128,count-offset);
   if(progress>=1&&banks[active].order!=requested){
    // Both banks were prepared with equal vector capacities. Assignment reuses
    // their storage; allocation auditing covers changes while audio is running.
    banks[1-active]=banks[active];banks[1-active].order=requested;progress=0;
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
