// Real ONNX + MasterLabX callback/worker boundary; synthetic host, not FL Studio.
#include "StemgenRtBackend.h"
#include "LiveStemWorker.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using Clock=std::chrono::steady_clock;
static void sleepUntil(Clock::time_point deadline){
#ifdef _WIN32
 struct Timer {
  HANDLE handle=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
  ~Timer(){if(handle)CloseHandle(handle);}
 };
 static thread_local Timer ownedTimer;
 const auto timer=ownedTimer.handle;
 const auto ns=std::chrono::duration_cast<std::chrono::nanoseconds>(deadline-Clock::now()).count();
 if(ns<=0)return;
 if(!timer)throw std::runtime_error("High-resolution callback timer unavailable");
 LARGE_INTEGER due;due.QuadPart=-std::max<LONGLONG>(1,ns/100);
 if(!SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE)||WaitForSingleObject(timer,INFINITE)!=WAIT_OBJECT_0)
  throw std::runtime_error("Callback timer failed");
#else
 std::this_thread::sleep_until(deadline);
#endif
}
int main(int argc,char** argv){try{
 if(argc<2)return 2;
 const int hops=argc>2?std::stoi(argv[2]):1000;
 const int seconds=argc>3?std::stoi(argv[3]):10;
 if(hops<100||seconds<1||seconds>600)return 2;
 std::cout<<"kind,block,latency_samples,hops,mean_us,p99_us,max_us,inference_rtf,missing_frames,worker_drops,worker_failures,callback_p99_us,callback_late_over_1ms,max_reconstruction_error\n";
 for(bool parallel:{false,true}){
  masterlab::StemgenRtBackend backend(argv[1],parallel);
  masterlab::LiveStemHop input{},previous{};masterlab::SeparatedStemHop output{};
  std::vector<double> duration;duration.reserve(hops);double total=0,maxError=0;
  for(int hop=0;hop<hops+100;++hop){
   for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<128;++i)input[c][i]=.2f*std::sin(float(hop*128+i)*(.017f+float(c)*.003f));
   const auto start=Clock::now();const auto valid=backend.process(input,output);
   const auto us=std::chrono::duration<double,std::micro>(Clock::now()-start).count();
   if(hop>=100){duration.push_back(us);total+=us;}
   if(valid)for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<128;++i){float sum=0;for(const auto& stem:output)sum+=stem[c][i];maxError=std::max(maxError,double(std::abs(sum-previous[c][i])));}
   previous=input;
  }
  std::sort(duration.begin(),duration.end());
  std::cout<<(parallel?"neural_three_workers":"neural_one_worker")<<",128,128,"<<hops<<','<<total/hops<<','<<duration[duration.size()*99/100]<<','<<duration.back()<<','<<total/1.e6/(hops*128./44100)<<",NA,NA,NA,NA,NA,"<<maxError<<std::endl;
 }
 for(std::size_t block:{128u,256u,512u}){
  masterlab::LiveStemEngine engine;if(!engine.prepare(44100,4096,220))return 1;
  masterlab::LiveStemWorker worker(engine,std::make_unique<masterlab::StemgenRtBackend>(argv[1],true));
  std::vector<float> l(block),r(block);std::vector<double> callbacks;
  const auto samples=static_cast<std::uint64_t>(44100*seconds);std::uint64_t late=0;
  const auto origin=Clock::now();
  for(std::uint64_t at=0;at<samples;at+=block){
   const auto due=origin+std::chrono::nanoseconds(static_cast<std::int64_t>(1.e9*at/44100));sleepUntil(due);
   if(Clock::now()-due>std::chrono::milliseconds(1))++late;
   for(std::size_t i=0;i<block;++i){l[i]=.2f*std::sin(float(at+i)*.017f);r[i]=.2f*std::sin(float(at+i)*.020f);}
   const auto begin=Clock::now();engine.process(l.data(),r.data(),block,{static_cast<std::int64_t>(at),true,false,true});
   callbacks.push_back(std::chrono::duration<double,std::micro>(Clock::now()-begin).count());
  }
  const auto w=worker.statistics();const auto e=engine.statistics();std::sort(callbacks.begin(),callbacks.end());
  std::cout<<"paced_neural,"<<block<<",4096,"<<w.hops<<','<<(w.hops?w.totalInferenceNanoseconds/1000./w.hops:0)<<",NA,"<<w.maxInferenceNanoseconds/1000.<<','<<(w.hops?w.totalInferenceNanoseconds/1.e9/(w.hops*128./44100):0)<<','<<e.missingFrames<<','<<w.discardedFrames+e.droppedInput<<','<<w.failures<<','<<callbacks[callbacks.size()*99/100]<<','<<late<<",NA"<<std::endl;
 }
 return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
