#include "ModuleLoader.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <chrono>

int main(int argc, char** argv) {
 if (argc != 2) return 1;
 ml::ModuleLibrary library;
 const auto path = std::filesystem::path(argv[1]);
 std::string error;
 if (!library.open(path, path.parent_path(), error)) {
  std::cerr << error << '\n';
  return 2;
 }
 const auto& api = *library.api();
 auto* instance = library.create(error);
 if (!instance) return 3;
 auto check = [&] {
  if (api.prepare(instance, 48000, 128, 2) != MLP_OK) return false;
  std::array<float, 128> l{}, r{};
  float* channels[]{l.data(), r.data()};
  MlpProcessContext block{sizeof(block), 128, 2, 0, channels};
  auto run = [&](float input, int blocks) {
   for (int i=0; i<blocks; ++i) {
    l.fill(input); r.fill(input);
    if (api.process(instance, &block) != MLP_OK) return std::numeric_limits<float>::quiet_NaN();
   }
   return l.back();
  };
  if (std::abs(run(0.5f, 8)-0.5f)>1e-6f) return false;
  // SUM: threshold −20 dB, ratio 2:1, hard knee. A −6.02 dB input should exit at −13.01 dB.
  if (api.set_parameter(instance, 103, -20)!=MLP_OK || api.set_parameter(instance, 113, 2)!=MLP_OK ||
      api.set_parameter(instance, 143, 0)!=MLP_OK) return false;
  const auto output = run(0.5f, 400);
  std::cout << "Static output: " << 20*std::log10(output) << " dBFS\n";
  if (!(std::abs(output-0.223607f)<0.0003f)) return false;
  if (api.set_parameter(instance, 6, 1)!=MLP_OK) return false;
  if (std::abs(run(0.5f, 80)-0.5f)>=1e-6f || api.get_latency_samples(instance)!=0) return false;
  const auto meter=library.gainReduction();
  if(!meter||meter(instance,3)!=0) return false;
  if(api.set_parameter(instance,6,0)!=MLP_OK) return false;
  run(.5f,200);
  if(std::abs(meter(instance,3)-6.9897f)>.03f) return false;
  // Configure a 10 dB soft knee. At threshold the gain computer applies 0.625 dB.
  api.set_parameter(instance,143,10);api.reset(instance);
  if(std::abs(20*std::log10(run(.1f,200))+20.625f)>.03f)return false;
  // At 0% Mix the compression stage is dry, including while detection continues.
  api.set_parameter(instance,153,0);api.reset(instance);
  if(std::abs(run(.5f,10)-.5f)>1e-6f)return false;
  std::array<unsigned char,256> state{};uint32_t written=0;
  if(api.save_state(instance,state.data(),uint32_t(state.size()),&written)!=MLP_OK)return false;
  api.set_parameter(instance,103,-40);
  if(api.load_state(instance,state.data(),written)!=MLP_OK)return false;
  double restored=0;
  if(api.get_parameter(instance,103,&restored)!=MLP_OK||restored!=-20)return false;
  api.set_parameter(instance,153,100);
  api.set_parameter(instance,143,0);
  api.set_parameter(instance,123,20);
  api.set_parameter(instance,133,100);
  api.reset(instance);
  run(.5f,7);
  const float attackReading=meter(instance,3);
  run(.5f,400);
  run(0,37);
  const float releaseReading=meter(instance,3);
  if(std::abs(attackReading-4.24f)>.12f||std::abs(releaseReading-2.60f)>.1f)return false;
  api.set_parameter(instance,163,0);api.reset(instance);
  auto stereo=[&]{
   for(int i=0;i<400;++i){l.fill(.5f);r.fill(.02f);if(api.process(instance,&block)!=MLP_OK)return false;}
   return true;
  };
  if(!stereo()||std::abs(r.back()-.02f)>1e-6f)return false;
  api.set_parameter(instance,163,100);
  if(!stereo()||std::abs(r.back()-.008944f)>.00005f)return false;
  api.set_parameter(instance,113,1);
  auto harmonic=[&](int mode){
   api.set_parameter(instance,5,mode);api.reset(instance);
   double fundamental=0, third=0, maxUs=0;
   for(int b=0;b<150;++b){
    for(size_t i=0;i<l.size();++i)r[i]=l[i]=float(.9*std::sin(2*std::numbers::pi*double(b*128+int(i))/48));
    const auto start=std::chrono::steady_clock::now();
    if(api.process(instance,&block)!=MLP_OK)return -1.;
    maxUs=std::max(maxUs,std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count());
    for(size_t i=0;i<l.size();++i){
     const auto phase=2*std::numbers::pi*double(b*128+int(i))/48;
     fundamental+=l[i]*std::sin(phase);third+=l[i]*std::sin(3*phase);
    }
   }
   const double ratio=std::abs(third/fundamental);
   std::cout<<"Mode "<<mode<<" H3/fundamental: "<<ratio<<" max 128-frame block (us): "<<maxUs<<'\n';
   return ratio;
  };
  const auto digital=harmonic(0),analog=harmonic(1);
  if(digital<0||digital>1e-6||analog<.001||analog>.01)return false;
  api.set_parameter(instance,5,0);api.reset(instance);
  l.fill(0);r.fill(0);l[0]=.5f;r[0]=-.25f;
  if(api.process(instance,&block)!=MLP_OK||l[0]!=.5f||r[0]!=-.25f)return false;
  for(size_t i=1;i<l.size();++i)if(l[i]!=0||r[i]!=0)return false;
  std::cout<<"Attack reading at 18.7 ms: "<<attackReading<<" dB; release at 98.7 ms: "<<releaseReading<<" dB\n";
  for(double rate:{44100.,48000.,88200.,96000.,176400.,192000.}){
   for(uint32_t size:{1u,17u,64u,127u,128u}){
    if(api.prepare(instance,rate,size,2)!=MLP_OK)return false;
    block.frames=size;
    l.fill(1e-38f);r.fill(100.f);
    l[0]=std::numeric_limits<float>::quiet_NaN();r[0]=std::numeric_limits<float>::infinity();
    if(api.process(instance,&block)!=MLP_OK||l[0]!=0||r[0]!=0)return false;
    for(uint32_t i=0;i<size;++i)if(!std::isfinite(l[i])||!std::isfinite(r[i]))return false;
   }
  }
  return true;
 };
 const bool passed = check();
 library.destroy(instance);
 return passed ? 0 : 4;
}
