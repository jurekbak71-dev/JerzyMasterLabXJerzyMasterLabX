#pragma once
#include "ModularDsp.h"
namespace masterlab {
struct StabilizerSettings {
 bool enabled=false,cut=false,tame=true,delta=false;int profile=0;
 float amount=25,speed=50,smoothing=50,low=100,mid=100,high=100;int intensity=0;
};
// Original octave-band adaptive equalizer. Profiles describe relative band
// energies, not a learned reference or a proprietary competitor algorithm.
class Stabilizer {
 double rate=48000;StabilizerSettings config;Slew bypass,delta;
 std::array<std::array<Biquad,12>,2> detectors{},filters{};
 std::array<double,12> energy{};std::array<float,12> gains{},targets{};
 double fullEnergy=0;float peak=0;unsigned counter=0;
public:
 static constexpr std::array<float,12> frequencies{40,70,125,220,400,700,1250,2200,4000,7000,12000,17000};
 void settings(StabilizerSettings c)noexcept{config=c;}
 void prepare(double sr){rate=sr;reset();}
 void reset()noexcept{detectors={};filters={};energy={};gains={};targets={};fullEnergy=0;peak=0;counter=0;bypass.prepare(rate,config.enabled?1.f:0.f);delta.prepare(rate,config.delta?1.f:0.f);for(auto& channel:detectors)for(size_t i=0;i<12;++i)channel[i].configure(rate,frequencies[i],0,1.4,FilterType::Notch);}
 void process(float* l,float* r,int count)noexcept{
  if(!config.enabled&&bypass.value<1e-6f){bypass.value=0;return;}
  const double average=std::exp(-1/(rate*(.1+config.speed*.009))),gainSlew=1-std::exp(-64/(rate*(.05+config.speed*.0095))),peakRelease=std::exp(-1/(rate*.01));
  for(int sample=0;sample<count;++sample){const float left=clean(l[sample]),right=clean(r[sample]);const double power=(left*left+right*right)*.5;
   fullEnergy=average*fullEnergy+(1-average)*power;peak=std::max(std::max(std::abs(left),std::abs(right)),static_cast<float>(peak*peakRelease));
   for(size_t i=0;i<12;++i){const float dl=left-detectors[0][i].tick(left),dr=right-detectors[1][i].tick(right);energy[i]=average*energy[i]+(1-average)*(dl*dl+dr*dr)*.5;}
   if(counter++%64==0){
    double sum=0,maxEnergy=0;size_t valid=0;for(size_t i=0;i<12;++i)if(frequencies[i]<rate*.42){sum+=energy[i];maxEnergy=std::max(maxEnergy,energy[i]);++valid;}
    const double mean=sum/std::max<size_t>(1,valid);std::array<float,12> raw{};
    const int intensity=std::clamp(config.intensity,0,3);const float limit=std::array<float,4>{6.f,4.f,9.f,12.f}[intensity],authority=std::array<float,4>{1.f,.75f,1.35f,1.8f}[intensity];
    const float support=intensity==0?1.f:(sum>1e-12?std::clamp(static_cast<float>((1-maxEnergy/sum)*1.35),.35f,1.f):0.f);
    for(size_t i=0;i<12;++i){
     // Neutral, bass-forward Electronic, and relaxed high-frequency Acoustic.
     float tilt=config.profile==1?(i<4?1.6f:(i>8?.85f:1.f)):(config.profile==2?(i>7?.65f:1.f):1.f);
     if(mean>1e-12&&energy[i]>mean*1e-4&&frequencies[i]<rate*.42){float correction=static_cast<float>(5*std::log10(std::max(1e-18,mean*tilt)/energy[i]));raw[i]=intensity==0?std::clamp(correction,-6.f,config.cut?0.f:6.f):std::clamp(correction*authority,-limit,config.cut?0.f:limit);}
    }
    for(size_t i=0;i<12;++i){float sumGain=raw[i],weights=1;for(int distance=1;distance<=2;++distance){const float weight=config.smoothing*.01f/(distance+1);if(i>=static_cast<size_t>(distance)){sumGain+=raw[i-distance]*weight;weights+=weight;}if(i+distance<12){sumGain+=raw[i+distance]*weight;weights+=weight;}}
     const float region=i<4?config.low:(i<8?config.mid:config.high),crest=peak/static_cast<float>(std::sqrt(std::max(1e-18,fullEnergy))),protection=intensity==0?(config.tame&&crest>3?.25f:1.f):(config.tame?std::clamp(1.25f-.25f*crest,.25f,1.f):1.f);
     targets[i]=sumGain/weights*config.amount*.01f*region*.01f*protection*support;gains[i]+=static_cast<float>(gainSlew)*(targets[i]-gains[i]);if(intensity>0)gains[i]=std::clamp(gains[i],-limit,config.cut?0.f:limit);
     for(auto& channel:filters)channel[i].configure(rate,frequencies[i],gains[i],1.4,FilterType::Bell);
    }
   }
   float wetL=left,wetR=right;for(size_t i=0;i<12;++i){wetL=filters[0][i].tick(wetL);wetR=filters[1][i].tick(wetR);}
   const float difference=delta.tick(config.delta?1.f:0.f);wetL+=difference*(left-2*wetL);wetR+=difference*(right-2*wetR);const float mix=bypass.tick(config.enabled?1.f:0.f);l[sample]=clean(left+mix*(wetL-left));r[sample]=clean(right+mix*(wetR-right));
  }
 }
 std::array<float,12> read()const noexcept{return gains;}
 void coefficients(std::array<Biquad,12>& out)const noexcept{out=filters[0];}
 float blend()const noexcept{return bypass.value;}
};
}
