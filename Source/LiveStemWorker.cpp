#include "LiveStemWorker.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>
namespace masterlab {
LiveStemWorker::LiveStemWorker(LiveStemEngine& engine,std::unique_ptr<LiveStemBackend> backend)
 :engine_(engine),backend_(std::move(backend)){
 if(!backend_)throw std::invalid_argument("Missing live backend");
 thread_=std::jthread([this](std::stop_token stop){run(stop);});
}
LiveStemWorker::~LiveStemWorker()=default;
void LiveStemWorker::run(std::stop_token stop){
 using Clock=std::chrono::steady_clock;
 LiveStemHop input{};SeparatedStemHop output{};
 StereoLiveFrame frame{};std::size_t filled=0;std::uint32_t epoch=0;
 std::uint64_t next=0,start=0;
 bool resetRequired=true;
 auto previousEnd=Clock::now();
 while(!stop.stop_requested()){
  if(!engine_.popInput(frame)){std::this_thread::sleep_for(std::chrono::microseconds(100));continue;}
  if(frame.epoch!=engine_.currentEpoch()){discardedFrames_.fetch_add(1,std::memory_order_relaxed);resetRequired=true;continue;}
  if(resetRequired||frame.epoch!=epoch||frame.position!=next){
   try{backend_->reset();}catch(...){failures_.fetch_add(1,std::memory_order_relaxed);continue;}
   filled=0;epoch=frame.epoch;resetRequired=false;
  }
  if(filled==0)start=frame.position;
  input[0][filled]=frame.left;input[1][filled]=frame.right;++filled;next=frame.position+1;
  if(filled<128)continue;
  filled=0;const auto begin=Clock::now();bool valid=false;
  const auto between=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(begin-previousEnd).count());
  maxBetweenNs_.store(std::max(maxBetweenNs_.load(std::memory_order_relaxed),between),std::memory_order_relaxed);
  maxBacklog_.store(std::max(maxBacklog_.load(std::memory_order_relaxed),static_cast<std::uint64_t>(engine_.pendingInputFrames())),std::memory_order_relaxed);
  try{valid=backend_->process(input,output);}catch(...){failures_.fetch_add(1,std::memory_order_relaxed);resetRequired=true;}
  const auto ns=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now()-begin).count());
  totalNs_.fetch_add(ns,std::memory_order_relaxed);maxNs_.store(std::max(maxNs_.load(std::memory_order_relaxed),ns),std::memory_order_relaxed);
  if(valid&&start>=128&&epoch==engine_.currentEpoch()){
   bool finite=true;for(const auto& stem:output)for(const auto& channel:stem)for(float x:channel)finite=finite&&std::isfinite(x);
   if(!finite){failures_.fetch_add(1,std::memory_order_relaxed);resetRequired=true;}
   else for(std::size_t i=0;i<128;++i){
    SeparatedLiveFrame separated{};separated.epoch=epoch;separated.position=start-128+i;
    for(std::size_t s=0;s<4;++s)for(std::size_t c=0;c<2;++c)separated.channels[s*2+c]=output[s][c][i];
    if(engine_.pushOutput(separated))outputFrames_.fetch_add(1,std::memory_order_relaxed);
    else discardedFrames_.fetch_add(1,std::memory_order_relaxed);
   }
  }
  // Publication after output allows control/test observers to wait for a complete hop.
  hops_.fetch_add(1,std::memory_order_release);
  previousEnd=Clock::now();
 }
}
LiveStemWorkerStatistics LiveStemWorker::statistics()const noexcept{
 return {hops_.load(std::memory_order_acquire),outputFrames_.load(std::memory_order_relaxed),failures_.load(std::memory_order_relaxed),
 discardedFrames_.load(std::memory_order_relaxed),totalNs_.load(std::memory_order_relaxed),maxNs_.load(std::memory_order_relaxed),
 maxBetweenNs_.load(std::memory_order_relaxed),maxBacklog_.load(std::memory_order_relaxed)};
}
}
