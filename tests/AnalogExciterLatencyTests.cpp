#include "AnalogExciter.h"
#include <array>
#include <cmath>
#include <cstdio>

namespace {
constexpr int count=256;

bool close(float a,float b,float tolerance=2.e-5f){return std::abs(a-b)<=tolerance;}

masterlab::AnalogExciter makeProcessor(float mix,bool delta,bool enabled){
 masterlab::AnalogExciter dsp;
 masterlab::AnalogizerSettings colour;colour.enabled=enabled;colour.driveDb=12.f;colour.warmth=.6f;colour.mix=1.f;
 dsp.colourSettings(colour);dsp.monitor(mix,delta);dsp.prepare(48000.);return dsp;
}
}

int main(){
 if(masterlab::AnalogExciter::latencySamples()!=48){std::puts("Analog Exciter must declare its fixed 4x latency");return 1;}

 std::array<float,count> input{},left{},right{};input[0]=.25f;input[73]=-.1f;
 auto dry=makeProcessor(0.f,false,false);left=input;right=input;dry.process(left.data(),right.data(),count);
 for(int i=0;i<count;++i){const float expected=i>=48?input[i-48]:0.f;if(!close(left[i],expected)||!close(right[i],expected)){std::printf("unaligned dry at %d: %.9f expected %.9f\n",i,left[i],expected);return 2;}}

 auto wet=makeProcessor(1.f,false,true);auto delta=makeProcessor(1.f,true,true);
 std::array<float,count> wetL=input,wetR=input,deltaL=input,deltaR=input;
 wet.process(wetL.data(),wetR.data(),count);delta.process(deltaL.data(),deltaR.data(),count);
 for(int i=0;i<count;++i){const float delayed=i>=48?input[i-48]:0.f;if(!close(wetL[i]+deltaL[i],delayed,8.e-5f)){std::printf("delta null failed at %d: %.9f expected %.9f\n",i,wetL[i]+deltaL[i],delayed);return 3;}}

 auto whole=makeProcessor(.63f,false,true),partitioned=makeProcessor(.63f,false,true);
 std::array<float,count> a=input,b=input;a[1]=b[1]=.17f;
 std::array<float,count> ar=a,br=b;
 whole.process(a.data(),ar.data(),count);
 for(int offset=0;offset<count;){const int n=std::min(17,count-offset);partitioned.process(b.data()+offset,br.data()+offset,n);offset+=n;}
 for(int i=0;i<count;++i)if(!close(a[i],b[i],2.e-6f)){std::printf("partition mismatch at %d\n",i);return 4;}
}
