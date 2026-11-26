#include "ModuleLoader.h"
#include "ModularGraph.h"
#include "../modules/InputCompressorCrossover.h"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <chrono>
#include <fstream>
#include <thread>
#include <atomic>
#include <complex>

int main(int argc, char** argv) {
 if (argc < 2 || argc > 3) return 1;
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
   std::array<double,20> harmonics{},quadrature{};
   double maxUs=0;
   for(int b=0;b<150;++b){
    for(size_t i=0;i<l.size();++i)r[i]=l[i]=float(.9*std::sin(2*std::numbers::pi*double(b*128+int(i))/48));
    const auto start=std::chrono::steady_clock::now();
    if(api.process(instance,&block)!=MLP_OK)return -1.;
    maxUs=std::max(maxUs,std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count());
    for(size_t i=0;i<l.size();++i){
     const auto phase=2*std::numbers::pi*double(b*128+int(i))/48;
     for(size_t h=0;h<harmonics.size();++h){
      harmonics[h]+=l[i]*std::sin(double(h+1)*phase);
      quadrature[h]+=l[i]*std::cos(double(h+1)*phase);
     }
    }
   }
   const double fundamental=std::hypot(harmonics[0],quadrature[0]);
   const double ratio=std::hypot(harmonics[2],quadrature[2])/fundamental;
   const double second=std::hypot(harmonics[1],quadrature[1])/fundamental;
   double harmonicPower=0;
   for(size_t h=1;h<harmonics.size();++h)harmonicPower+=harmonics[h]*harmonics[h]+quadrature[h]*quadrature[h];
   const double thd=std::sqrt(harmonicPower)/fundamental;
   std::cout<<"Mode "<<mode<<" H2/fundamental: "<<second<<" H3/fundamental: "<<ratio<<" THD (H2–H20): "<<thd<<" max 128-frame block (us): "<<maxUs<<'\n';
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
  if(api.prepare(instance,48000,128,2)!=MLP_OK)return false;
  block.frames=128;
  const auto* descriptor=api.get_descriptor();
  if(descriptor->parameter_count!=46)return false;
  for(uint32_t i=0;i<descriptor->parameter_count;++i)api.set_parameter(instance,descriptor->parameters[i].id,descriptor->parameters[i].neutral_value);
  api.set_parameter(instance,173,6);api.reset(instance);
  if(std::abs(run(.2f,20)-.2f*std::pow(10.f,.3f))>1e-5f)return false;
  api.set_parameter(instance,183,0);api.reset(instance);
  if(std::abs(run(.2f,20)-.2f)>1e-6f)return false;
  api.set_parameter(instance,183,1);api.set_parameter(instance,173,0);api.set_parameter(instance,193,0);api.reset(instance);
  l.fill(.3f);r.fill(-.1f);if(api.process(instance,&block)!=MLP_OK||std::abs(l.back()-.1f)>1e-6f||std::abs(r.back()-.1f)>1e-6f)return false;
  api.set_parameter(instance,193,200);api.reset(instance);
  l.fill(.3f);r.fill(-.1f);if(api.process(instance,&block)!=MLP_OK||std::abs(l.back()-.5f)>1e-6f||std::abs(r.back()+.3f)>1e-6f)return false;
  api.set_parameter(instance,193,100);api.reset(instance);
  std::cout<<"PASS independent makeup, band enable and stereo width\n";
  std::atomic<bool> writtenAll{false},valid{true};
  std::thread automation([&] {
   for(int pass=0;pass<100;++pass)for(uint32_t i=0;i<descriptor->parameter_count;++i){
    const auto& parameter=descriptor->parameters[i];
    const auto value=pass%2==0?parameter.maximum:parameter.minimum;
    double actual=0;
    if(api.set_parameter(instance,parameter.id,value)!=MLP_OK||
       api.get_parameter(instance,parameter.id,&actual)!=MLP_OK||std::abs(actual-value)>1e-5)
     valid.store(false,std::memory_order_relaxed);
   }
   writtenAll.store(true,std::memory_order_release);
  });
  do {
   l.fill(.32f);r.fill(-.24f);
   if(api.process(instance,&block)!=MLP_OK||!std::isfinite(l.back())||!std::isfinite(r.back()))
    valid.store(false,std::memory_order_relaxed);
  }while(!writtenAll.load(std::memory_order_acquire));
  automation.join();
  if(!valid.load(std::memory_order_relaxed))return false;
  for(uint32_t i=0;i<descriptor->parameter_count;++i){
   const auto& parameter=descriptor->parameters[i];api.set_parameter(instance,parameter.id,parameter.neutral_value);
  }
  api.set_parameter(instance,103,-20);api.set_parameter(instance,113,2);
  api.reset(instance);
  ml::ModularGraph graph;
  graph.prepare(128);
  ml::GraphSnapshot snapshot;
  snapshot.count=1;
  snapshot.nodes[0]={1,&api,instance,false};
  graph.installWhenStopped(snapshot);
  std::array<float,128> delta{};
  for(int i=0;i<400;++i){
   l.fill(.5f);r.fill(.5f);
   if(!graph.process(l.data(),r.data(),128,delta.data()))return false;
  }
  if(std::abs(delta.back()+.276393f)>.0004f||graph.diagnostics().latency!=0)return false;
  graph.resetWhenStopped();
  ml::CompressorCrossover crossover;
  const float low=ml::CompressorCrossover::coefficient(180,48000);
  const float high=ml::CompressorCrossover::coefficient(3200,48000);
  std::array<std::array<float,8192>,3> impulse{};
  float reconstructionError=0;
  for(size_t i=0;i<impulse[0].size();++i){
   auto bands=crossover.process(i==0?1.f:0.f,low,high);
   for(size_t b=0;b<bands.size();++b)impulse[b][i]=bands[b];
   reconstructionError=std::max(reconstructionError,std::abs(bands[0]+bands[1]+bands[2]-(i==0?1.f:0.f)));
  }
  if(reconstructionError>1e-6f)return false;
  auto response=[&](double hz,size_t band){
   std::complex<double> value{};
   const auto phase=-2*std::numbers::pi*hz/48000;
   for(size_t i=0;i<impulse[band].size();++i)value+=double(impulse[band][i])*std::polar(1.,phase*double(i));
   return value;
  };
  if(std::abs(std::abs(response(180,0))-.707107)>.001||
     std::abs(std::abs(response(3200,2))-.707107)>.001)return false;
  if(argc==3){
   api.set_parameter(instance,103,-20);api.set_parameter(instance,113,4);
   std::ofstream curve(argv[2]);
   curve<<"mode,knee_db,input_dbfs,output_dbfs\n";
   for(int mode:{0,1})for(int knee:{0,12})for(int level=-45;level<=0;++level){
    api.set_parameter(instance,5,mode);api.set_parameter(instance,143,knee);api.reset(instance);
    const float result=run(std::pow(10.f,float(level)/20),325);
    if(!std::isfinite(result)||result<=0)return false;
    curve<<mode<<','<<knee<<','<<level<<','<<20*std::log10(result)<<'\n';
   }
   if(!curve.good())return false;
   std::ofstream bands(std::string(argv[2])+".bands.csv");
   bands<<"hz,low_db,mid_db,high_db,low_phase_deg,mid_phase_deg,high_phase_deg\n";
   for(int i=0;i<=100;++i){
    const double hz=20*std::pow(1000.,double(i)/100);
    std::array<std::complex<double>,3> measured{response(hz,0),response(hz,1),response(hz,2)};
    bands<<hz;
    for(const auto& v:measured)bands<<','<<20*std::log10(std::abs(v));
    for(const auto& v:measured)bands<<','<<std::arg(v)*180/std::numbers::pi;
    bands<<'\n';
   }
   if(!bands.good())return false;
  }
  return true;
 };
 const bool passed = check();
 library.destroy(instance);
 return passed ? 0 : 4;
}
