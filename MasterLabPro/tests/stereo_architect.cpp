#include "ModuleLoader.h"
#include <array>
#include <bit>
#include <cmath>
#include <iostream>
#include <numbers>

int main(int argc,char** argv){
 if(argc!=2)return 1;
 const auto path=std::filesystem::path(argv[1]);std::string error;
 ml::ModuleLibrary library;
 if(!library.open(path,path.parent_path(),error))return 2;
 auto* instance=library.create(error);
 if(!instance)return 3;
 const auto& api=*library.api();
 const auto read=library.dynamics();
 auto verify=[&]{
  if(!read||api.get_latency_samples(instance)!=0)return false;
  std::array<float,511> l{},r{};
  float* channels[]{l.data(),r.data()};
  for(double rate:{44100.,48000.,96000.,192000.})for(uint32_t block:{17u,128u,511u}){
   if(api.prepare(instance,rate,block,2)!=MLP_OK)return false;
   MlpProcessContext context{sizeof(context),block,2,0,channels};
   for(int width:{0,100,200}){
    for(uint32_t id=1;id<=7;++id)if(api.set_parameter(instance,id,width)!=MLP_OK)return false;
    api.reset(instance);
    l.fill(.2f);r.fill(.2f);
    if(api.process(instance,&context)!=MLP_OK)return false;
    if(l[0]!=.2f||r[0]!=.2f)return false;
    for(uint32_t i=0;i<block;++i){
     l[i]=float(.2*std::cos(2*std::numbers::pi*1200*i/rate));r[i]=-l[i];
    }
    const auto original=l;
    if(api.process(instance,&context)!=MLP_OK)return false;
    for(uint32_t i=0;i<block;++i)if(std::abs(l[i]-original[i]*float(width)*.01f)>1e-6f||std::abs(l[i]+r[i])>1e-6f)return false;
   }
   if(api.set_parameter(instance,8,15000)!=MLP_OK||api.set_parameter(instance,9,40)!=MLP_OK)return false;
   l.fill(.4f);r.fill(-.2f);
   if(api.process(instance,&context)!=MLP_OK||!std::isfinite(l[block-1]))return false;
   if(api.set_parameter(instance,14,1)!=MLP_OK)return false;
   api.reset(instance);l.fill(.45f);r.fill(-.15f);
   if(api.process(instance,&context)!=MLP_OK||l[block-1]!=.45f||r[0]!=-.15f)return false;
   if(api.set_parameter(instance,14,0)!=MLP_OK)return false;
  }
  if(api.set_parameter(instance,8,60)!=MLP_OK||api.set_parameter(instance,9,250)!=MLP_OK||
     api.prepare(instance,48000,128,2)!=MLP_OK)return false;
  for(uint32_t id=1;id<=7;++id)if(api.set_parameter(instance,id,id==1?0:100)!=MLP_OK)return false;
  auto sideRatio=[&](double hz){
   api.reset(instance);
   double sum=0,input=0;
   MlpProcessContext context{sizeof(context),128,2,0,channels};
   for(int b=0;b<160;++b){
    for(int i=0;i<128;++i){l[i]=float(.2*std::sin(2*std::numbers::pi*hz*double(i+b*128)/48000));r[i]=-l[i];if(b>=40)input+=l[i]*l[i];}
    if(api.process(instance,&context)!=MLP_OK)return -1.;
    if(b>=40)for(int i=0;i<128;++i)sum+=l[i]*l[i];
   }
   return std::sqrt(sum/input);
  };
  const double bass=sideRatio(30),treble=sideRatio(8000);
  if(bass<.43||bass>.47||treble<.99||treble>1.01)return false;
  const float correlation=std::bit_cast<float>(uint32_t(read(instance,0)));
  if(std::abs(correlation+1)>1e-5f)return false;
  std::cout<<"SUB width 0%: side gain at 30 Hz = "<<bass<<", at 8 kHz = "<<treble<<'\n';
  auto modeRatio=[&](int mode,bool enabled){
   if(api.prepare(instance,48000,128,2)!=MLP_OK)return -1.;
   for(uint32_t id=1;id<=7;++id)if(api.set_parameter(instance,id,200)!=MLP_OK)return -1.;
   if(api.set_parameter(instance,15,mode)!=MLP_OK)return -1.;
   for(uint32_t band=0;band<7;++band)if(api.set_parameter(instance,16+band,enabled?1:0)!=MLP_OK)return -1.;
   api.reset(instance);double in=0,out=0;
   MlpProcessContext context{sizeof(context),128,2,0,channels};
   for(int block=0;block<100;++block){
    for(int i=0;i<128;++i){const double phase=2*std::numbers::pi*1200.*double(block*128+i)/48000.;l[i]=float(.1*std::sin(phase));r[i]=-l[i];}
    if(block>=60)for(int i=0;i<128;++i)in+=double(l[i])*l[i];
    if(api.process(instance,&context)!=MLP_OK)return -1.;
    if(block>=60)for(int i=0;i<128;++i)out+=double(l[i])*l[i];
   }
   return std::sqrt(out/in);
  };
  const double natural=modeRatio(0,true),wide=modeRatio(1,true),ultra=modeRatio(2,true),disabled=modeRatio(2,false);
  std::cout<<"1200 Hz side gain at 200%: NATURAL="<<natural<<", WIDE="<<wide<<", ULTRA="<<ultra<<"; all bands disabled="<<disabled<<'\n';
  if(!std::isfinite(natural)||!std::isfinite(wide)||!std::isfinite(ultra)||!std::isfinite(disabled)||
     std::abs(natural-2.)>.03||std::abs(wide-3.)>.04||std::abs(ultra-4.)>.05||std::abs(disabled-1.)>.02)return false;
  constexpr std::array<double,7> bandProbeHz{40.,150.,375.,1000.,3000.,6000.,12000.};
  std::cout<<"ULTRA 200% individual band enable ratios (SUB..AIR):";
  for(uint32_t band=0;band<7;++band){
   auto selectedBandRatio=[&](bool enabled){
    if(api.prepare(instance,48000,128,2)!=MLP_OK)return -1.;
    for(uint32_t id=1;id<=7;++id)if(api.set_parameter(instance,id,id==band+1?200.:100.)!=MLP_OK)return -1.;
    if(api.set_parameter(instance,15,2)!=MLP_OK)return -1.;
    for(uint32_t index=0;index<7;++index)if(api.set_parameter(instance,16+index,index==band&&enabled?1.:0.)!=MLP_OK)return -1.;
    api.reset(instance);double inputEnergy=0,outputEnergy=0;
    MlpProcessContext context{sizeof(context),128,2,0,channels};
    for(int block=0;block<100;++block){
     for(int i=0;i<128;++i){const double phase=2*std::numbers::pi*bandProbeHz[band]*double(block*128+i)/48000.;l[i]=float(.1*std::sin(phase));r[i]=-l[i];}
     if(block>=60)for(int i=0;i<128;++i)inputEnergy+=double(l[i])*l[i];
     if(api.process(instance,&context)!=MLP_OK)return -1.;
     if(block>=60)for(int i=0;i<128;++i)outputEnergy+=double(l[i])*l[i];
    }
    return std::sqrt(outputEnergy/inputEnergy);
   };
   const double on=selectedBandRatio(true),offBand=selectedBandRatio(false);
   std::cout<<' '<<on<<'/'<<offBand;
   if(!std::isfinite(on)||!std::isfinite(offBand)||on<1.01||std::abs(offBand-1.)>.02)return false;
  }
  std::cout<<'\n';
  auto stereoizeRatio=[&](double compatibility,bool enabled){
   if(api.prepare(instance,48000,128,2)!=MLP_OK)return -1.;
   for(uint32_t id=1;id<=7;++id)if(api.set_parameter(instance,id,100)!=MLP_OK)return -1.;
   if(api.set_parameter(instance,15,0)!=MLP_OK||api.set_parameter(instance,23,enabled?1:0)!=MLP_OK||
      api.set_parameter(instance,24,100)!=MLP_OK||api.set_parameter(instance,25,100)!=MLP_OK||
      api.set_parameter(instance,26,50)!=MLP_OK||api.set_parameter(instance,27,compatibility)!=MLP_OK)return -1.;
   api.reset(instance);double midEnergy=0,sideEnergy=0,midError=0;
   MlpProcessContext context{sizeof(context),128,2,0,channels};
   for(int block=0;block<100;++block){
    for(int i=0;i<128;++i){const double phase=2*std::numbers::pi*997.*double(block*128+i)/48000.;l[i]=r[i]=float(.2*std::sin(phase));}
    const auto original=l;
    if(api.process(instance,&context)!=MLP_OK)return -1.;
    if(block>=60)for(int i=0;i<128;++i){
     const double mid=.5*(l[i]+r[i]),side=.5*(l[i]-r[i]);
     midEnergy+=mid*mid;sideEnergy+=side*side;midError+=(mid-original[i])*(mid-original[i]);
    }
   }
   if(std::sqrt(midError/midEnergy)>1e-6)return -2.;
   return std::sqrt(sideEnergy/midEnergy);
  };
  const double stereoized=stereoizeRatio(0,true),protectedStereo=stereoizeRatio(100,true),off=stereoizeRatio(0,false);
  std::cout<<"Stereoize mono Side/Mid ratio: off="<<off<<", enabled="<<stereoized<<", compatibility=100% "<<protectedStereo<<'\n';
  if(!std::isfinite(stereoized)||!std::isfinite(protectedStereo)||!std::isfinite(off)||stereoized<.1||
     std::abs(protectedStereo-stereoized*.5)>.01||off>1e-6)return false;
  if(api.set_parameter(instance,8,15000)!=MLP_OK)return false;
  std::array<uint8_t,128> state{};uint32_t written=0;
  if(api.save_state(instance,state.data(),uint32_t(state.size()),&written)!=MLP_OK)return false;
  if(written!=112||api.set_parameter(instance,8,100)!=MLP_OK||api.set_parameter(instance,15,0)!=MLP_OK||
     api.load_state(instance,state.data(),written)!=MLP_OK)return false;
  double restored=0;if(api.get_parameter(instance,8,&restored)!=MLP_OK||restored!=15000)return false;
  std::array<uint8_t,60> legacy{};
  const auto write=[&](size_t position,uint32_t value){for(unsigned byte=0;byte<4;++byte)legacy[position+byte]=uint8_t(value>>(8*byte));};
  write(0,0x31414c4d);
  for(uint32_t id=1;id<=14;++id){double value=0;if(api.get_parameter(instance,id,&value)!=MLP_OK)return false;write(4+4*(id-1),std::bit_cast<uint32_t>(float(id==8?123:value)));}
  if(api.set_parameter(instance,15,2)!=MLP_OK||api.set_parameter(instance,16,0)!=MLP_OK||api.load_state(instance,legacy.data(),uint32_t(legacy.size()))!=MLP_OK)return false;
  double migratedMode=-1,migratedBand=0,migratedCrossover=0;
  return api.get_parameter(instance,8,&migratedCrossover)==MLP_OK&&migratedCrossover==123&&
         api.get_parameter(instance,15,&migratedMode)==MLP_OK&&migratedMode==0&&
         api.get_parameter(instance,16,&migratedBand)==MLP_OK&&migratedBand==1;
 };
 const bool pass=verify();library.destroy(instance);
 if(pass)std::cout<<"PASS stereo modes, seven independent bands, Stereoize, mono, crossover automation, bypass and legacy state migration\n";
 return pass?0:4;
}
