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
  std::array<uint8_t,128> state{};uint32_t written=0;
  if(api.save_state(instance,state.data(),uint32_t(state.size()),&written)!=MLP_OK)return false;
  if(api.set_parameter(instance,8,100)!=MLP_OK||api.load_state(instance,state.data(),written)!=MLP_OK)return false;
  double restored=0;return api.get_parameter(instance,8,&restored)==MLP_OK&&restored==15000;
 };
 const bool pass=verify();library.destroy(instance);
 if(pass)std::cout<<"PASS stereo width 0/100/200%, mono, Side, crossover automation, bypass and state\n";
 return pass?0:4;
}
