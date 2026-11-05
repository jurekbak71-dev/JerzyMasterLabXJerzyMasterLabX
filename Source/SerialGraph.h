#pragma once
#include "Stabilizer.h"
#include "HarmonicExciter.h"
#include "AnalogColor.h"
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
struct ParallelReading {
 std::array<float,6> rms{},trimDb{},mix{};
 std::array<bool,6> present{},enabled{};
 bool active=false,compensated=false,limiterBypassed=false;
 int branches=0;
};
class SerialGraph {
 struct Bank {
  Engine engine;StereoImager stereo;SmartCompressor dynamics;DynamicEqualizer dynamicEq;Stabilizer stabilizer;
  ModuleOrder order{5,0,1,2,3,4};
  bool parallel=false,compensated=false;std::array<bool,6> present{},enabled{true,true,true,true,true,true};
  std::array<float,6> targetTrimDb{},targetMix{1,1,1,1,1,1},branchEnergy{};
  std::array<Slew,6> trimSlew{},mixSlew{};Slew compensationSlew;float meterCoefficient=.999f;
  std::array<std::array<std::vector<float>,2>,6> alignment;size_t alignPosition=0;
  std::array<std::array<float,128>,2> branch{},dry{},sum{};std::array<float,128> branchTrace{},weightSum{},limiterDelta{};
  void prepareParallel(double rate)noexcept{meterCoefficient=static_cast<float>(std::exp(-1/(rate*.4)));branchEnergy={};compensationSlew.prepare(rate,compensated?1.f:0.f);for(size_t i=0;i<6;++i){trimSlew[i].prepare(rate,dbGain(targetTrimDb[i]));mixSlew[i].prepare(rate,targetMix[i]);}}
  void parallelSettings(bool normalize,std::array<float,6> trims,std::array<float,6> mixes)noexcept{compensated=normalize;targetTrimDb=trims;targetMix=mixes;}
  void parallelProcess(float* l,float* r,int n,float* trace)noexcept{
   for(auto& channel:sum)std::fill_n(channel.data(),n,0.f);std::fill_n(trace,n,1.f);std::fill_n(branchTrace.data(),n,1.f);std::fill_n(weightSum.data(),n,0.f);std::fill_n(limiterDelta.data(),n,0.f);int branches=0;
   for(int kind:order)if(present[kind]){++branches;std::copy_n(l,n,branch[0].data());std::copy_n(r,n,branch[1].data());std::copy_n(l,n,dry[0].data());std::copy_n(r,n,dry[1].data());
    auto* a=branch[0].data();auto* b=branch[1].data();
    switch(kind){case 5:stabilizer.process(a,b,n);break;case 0:dynamicEq.process(a,b,n);break;case 1:dynamics.process(a,b,n);break;case 2:stereo.process(a,b,n);break;case 3:engine.process(a,b,n,false,nullptr,1);break;case 4:engine.process(a,b,n,false,branchTrace.data(),2);break;}
    size_t position=alignPosition;for(int i=0;i<n;++i){const size_t next=(position+1)%alignment[kind][0].size();const float wet=mixSlew[kind].tick(std::clamp(targetMix[kind],0.f,1.f));const float weight=trimSlew[kind].tick(dbGain(std::clamp(targetTrimDb[kind],-24.f,12.f)));double energy=0;
     for(int c=0;c<2;++c){float drySample=dry[c][i];if(kind==4){float delayed=alignment[kind][c][next];alignment[kind][c][position]=drySample;drySample=delayed;}float mixed=wet>=.999999f?branch[c][i]:(wet<=.000001f?drySample:drySample+wet*(branch[c][i]-drySample));if(kind!=4){float delayed=alignment[kind][c][next];alignment[kind][c][position]=mixed;mixed=delayed;}const float contribution=mixed*weight;sum[c][i]+=contribution;energy+=static_cast<double>(contribution)*contribution;}
     branchEnergy[kind]=meterCoefficient*branchEnergy[kind]+(1-meterCoefficient)*static_cast<float>(energy*.5);weightSum[i]+=weight;if(kind==4)limiterDelta[i]=weight*wet*(branchTrace[i]-1);position=next;}
   }
   if(!branches){engine.process(l,r,n,false,trace,2);return;}
   for(int i=0;i<n;++i){const float normalization=compensationSlew.tick(compensated?1.f:0.f);const float denominator=std::max(1e-6f,static_cast<float>(branches)+normalization*(weightSum[i]-branches));l[i]=clean(sum[0][i]/denominator);r[i]=clean(sum[1][i]/denominator);trace[i]=1+limiterDelta[i]/denominator;}
   alignPosition=(alignPosition+static_cast<size_t>(n))%alignment[0][0].size();
  }
  ParallelReading parallelRead()const noexcept{ParallelReading out;out.active=parallel;out.compensated=compensated;out.present=present;out.enabled=enabled;for(size_t i=0;i<6;++i){out.rms[i]=std::sqrt(std::max(0.f,branchEnergy[i]));out.trimDb[i]=levelDb(trimSlew[i].value);out.mix[i]=mixSlew[i].value;if(present[i])++out.branches;}out.limiterBypassed=parallel&&out.branches>1&&present[4]&&enabled[4];return out;}
  void process(float* l,float* r,int n,float* trace) noexcept {
   if(parallel){parallelProcess(l,r,n,trace);return;}
   for(int kind:order)switch(kind){
    case 5:stabilizer.process(l,r,n);break;case 0:dynamicEq.process(l,r,n);break;case 1:dynamics.process(l,r,n);break;case 2:stereo.process(l,r,n);break;
    case 3:engine.process(l,r,n,false,nullptr,1);break;case 4:engine.process(l,r,n,false,trace,2);break;
   }
  }
 };
 // Fixed post-graph slot: does NOT participate in the legacy six-family 720 permutations.
 HarmonicExciter harmonicExciter;
 AnalogColor analogColor;
 std::array<Bank,2> banks;int active=0;float progress=1,step=1.f/960;
 ModuleOrder requested{5,0,1,2,3,4};bool requestedParallel=false;std::array<bool,6> requestedPresent{},requestedEnabled{true,true,true,true,true,true};
 std::array<std::array<float,128>,2> scratch{};std::array<float,128> trace{};
public:
 void prepare(double rate,bool legacy=false,int code=0){rate=std::isfinite(rate)&&rate>=8000&&rate<=384000?rate:48000;harmonicExciter.prepare(rate);analogColor.prepare(rate);requested=decodeOrder(code,legacy);active=0;progress=1;step=static_cast<float>(1/(rate*.02));for(auto& b:banks){b.order=requested;b.engine.prepare(rate);b.stereo.prepare(rate);b.dynamics.prepare(rate);b.dynamicEq.prepare(rate);b.stabilizer.prepare(rate);b.parallel=requestedParallel;b.present=requestedPresent;b.enabled=requestedEnabled;b.prepareParallel(rate);b.alignPosition=0;for(auto& module:b.alignment)for(auto& channel:module)channel.assign(static_cast<size_t>(b.engine.latency()+1),0);}}
 void reset()noexcept{harmonicExciter.reset();analogColor.reset();progress=1;for(auto& b:banks){b.engine.reset();b.stereo.reset();b.dynamics.reset();b.dynamicEq.reset();b.stabilizer.reset();b.parallel=requestedParallel;b.present=requestedPresent;b.branchEnergy={};b.compensationSlew.value=b.compensated?1.f:0.f;b.alignPosition=0;for(size_t i=0;i<6;++i){b.trimSlew[i].value=dbGain(b.targetTrimDb[i]);b.mixSlew[i].value=b.targetMix[i];}for(auto& module:b.alignment)for(auto& channel:module)std::fill(channel.begin(),channel.end(),0);}}
 int latency()const noexcept{return banks[active].engine.latency();}
 void parameters(float hz,float db,float q,float ceiling,bool eq,bool lim)noexcept{for(auto& b:banks)b.engine.parameters(hz,db,q,ceiling,eq,lim);}
 void limiterParameters(float drive,float release)noexcept{for(auto& b:banks)b.engine.limiterParameters(drive,release);}
 void setBand(size_t i,float hz,float db,float q)noexcept{for(auto& b:banks)b.engine.setBand(i,hz,db,q);}
 void setBandMode(size_t i,FilterType t,bool on)noexcept{for(auto& b:banks)b.engine.setBandMode(i,t,on);}
 void stereoSettings(StereoSettings c)noexcept{for(auto& b:banks)b.stereo.settings(c);}
 void dynamicsSettings(DynamicsSettings c)noexcept{for(auto& b:banks)b.dynamics.settings(c);}
 void dynamicEqSettings(DynamicEqSettings c)noexcept{for(auto& b:banks)b.dynamicEq.settings(c);}
 void stabilizerSettings(StabilizerSettings c)noexcept{for(auto& b:banks)b.stabilizer.settings(c);}
 void harmonicExciterSettings(HarmonicExciterSettings c)noexcept{c.oversample2x=false;harmonicExciter.settings(c);}
 HarmonicExciterReading harmonicExciterRead()const noexcept{return harmonicExciter.reading();}
 void analogColorSettings(AnalogColorSettings c)noexcept{analogColor.settings(c);}
 AnalogColorReading analogColorRead()const noexcept{return analogColor.reading();}
 void topology(bool parallel,std::array<bool,6> present)noexcept{topology(parallel,present,{true,true,true,true,true,true});}
 void topology(bool parallel,std::array<bool,6> present,std::array<bool,6> enabled)noexcept{requestedParallel=parallel;requestedPresent=present;requestedEnabled=enabled;for(auto& b:banks)b.enabled=enabled;}
 void parallelSettings(bool compensated,std::array<float,6> trims,std::array<float,6> mixes)noexcept{for(auto& b:banks)b.parallelSettings(compensated,trims,mixes);}
 void routing(int code,bool legacy)noexcept{requested=decodeOrder(code,legacy);}
 void process(float* l,float* r,int count,bool /* legacy supplied through routing */,float* gainTrace)noexcept{
  for(int offset=0;offset<count;){int n=std::min(128,count-offset);
   if(progress>=1&&(banks[active].order!=requested||banks[active].parallel!=requestedParallel||(requestedParallel&&banks[active].present!=requestedPresent))){
    // Both banks were prepared with equal vector capacities. Assignment reuses
    // their storage; allocation auditing covers changes while audio is running.
    const auto previousPresent=banks[active].present;banks[1-active]=banks[active];banks[1-active].order=requested;banks[1-active].parallel=requestedParallel;banks[1-active].present=requestedPresent;for(size_t kind=0;kind<6;++kind)if(requestedPresent[kind]&&!previousPresent[kind])for(auto& channel:banks[1-active].alignment[kind])std::fill(channel.begin(),channel.end(),0.f);progress=0;
   }
   if(progress<1){std::copy_n(l+offset,n,scratch[0].data());std::copy_n(r+offset,n,scratch[1].data());banks[1-active].process(scratch[0].data(),scratch[1].data(),n,trace.data());}
   banks[active].process(l+offset,r+offset,n,gainTrace+offset);
   if(progress<1){for(int i=0;i<n;++i){progress=std::min(1.f,progress+step);l[offset+i]+=progress*(scratch[0][i]-l[offset+i]);r[offset+i]+=progress*(scratch[1][i]-r[offset+i]);gainTrace[offset+i]+=progress*(trace[i]-gainTrace[offset+i]);}if(progress>=1)active=1-active;}
   // Run only after the six-family graph has produced a fully mixed output.
   // In Parallel this is post-sum, not a seventh normalized branch.
   harmonicExciter.process(l+offset,r+offset,n);
   analogColor.process(l+offset,r+offset,n);
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
 ParallelReading parallelRead()const noexcept{return banks[active].parallelRead();}
};
}
