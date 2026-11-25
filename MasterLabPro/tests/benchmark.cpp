#include "AudioRack.h"
#include "SpectralAnalysis.h"
#include <array>
#include <chrono>
#include <complex>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <numbers>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace {
using Clock=std::chrono::steady_clock;

template<std::size_t N> class CandidateFft {
 std::vector<std::complex<double>> work=std::vector<std::complex<double>>(N);
 std::vector<std::complex<double>> roots=std::vector<std::complex<double>>(N/2);
 std::vector<std::size_t> reversed=std::vector<std::size_t>(N);
 std::vector<double> window=std::vector<double>(N);
public:
 CandidateFft(){
  for(std::size_t i=0;i<N;++i){
   window[i]=.5-.5*std::cos(2*std::numbers::pi*double(i)/double(N));
   std::size_t x=i,r=0;for(std::size_t n=N;n>1;n>>=1){r=(r<<1)|(x&1);x>>=1;}reversed[i]=r;
  }
  for(std::size_t i=0;i<roots.size();++i)roots[i]=std::polar(1.,-2*std::numbers::pi*double(i)/double(N));
 }
 double run(){
  for(std::size_t i=0;i<N;++i)work[reversed[i]]={std::sin(double(i)*.071)*window[i],0.};
  for(std::size_t len=2;len<=N;len*=2){const auto stride=N/len;for(std::size_t i=0;i<N;i+=len)for(std::size_t j=0;j<len/2;++j){auto u=work[i+j],v=work[i+j+len/2]*roots[j*stride];work[i+j]=u+v;work[i+j+len/2]=u-v;}}
  return std::abs(work[N/7]);
 }
};

template<std::size_t N> double fftMicros(){
 CandidateFft<N> fft;volatile double guard=0;constexpr int runs=40;
 const auto start=Clock::now();for(int i=0;i<runs;++i)guard=guard+fft.run();
 const auto us=std::chrono::duration<double,std::micro>(Clock::now()-start).count()/runs;
 (void)guard;return us;
}

std::uint64_t residentBytes(){
#if defined(_WIN32)
 PROCESS_MEMORY_COUNTERS info{};info.cb=sizeof(info);
 return GetProcessMemoryInfo(GetCurrentProcess(),&info,sizeof(info))?info.WorkingSetSize:0;
#elif defined(__linux__)
 std::ifstream stat("/proc/self/statm");std::uint64_t total=0,resident=0;stat>>total>>resident;
 return resident*static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
#else
 return 0;
#endif
}

double rackNanoseconds(double rate,std::size_t block){
 ml::AudioRack rack;rack.prepare(rate);ml::RackConfig config;
 for(std::size_t i=0;i<10;++i)config.stages[i]={i+1,float((int(i)%3)-1)*1.5f};
 if(!rack.submit(config))return -1;
 std::vector<float> left(block,.05f),right(block,-.05f);constexpr int runs=1000;
 const auto start=Clock::now();for(int i=0;i<runs;++i)rack.process(left.data(),right.data(),block);
 return std::chrono::duration<double,std::nano>(Clock::now()-start).count()/(runs*block);
}
}

int main(){
 const auto f2048=fftMicros<2048>(),f4096=fftMicros<4096>(),f8192=fftMicros<8192>(),f16384=fftMicros<16384>();
 const auto r441=rackNanoseconds(44100.,257),r48=rackNanoseconds(48000.,64),r96=rackNanoseconds(96000.,511),r192=rackNanoseconds(192000.,1024);
 if(f2048<=0||f4096<=0||f8192<=0||f16384<=0||r441<=0||r48<=0||r96<=0||r192<=0)return 1;
 std::cout<<"{\"fft_us\":{\"2048\":"<<f2048<<",\"4096\":"<<f4096<<",\"8192\":"<<f8192<<",\"16384\":"<<f16384
          <<"},\"rack_ns_per_sample_10_gain\":{\"44100_block257\":"<<r441<<",\"48000_block64\":"<<r48
          <<",\"96000_block511\":"<<r96<<",\"192000_block1024\":"<<r192<<"},\"resident_bytes\":"<<residentBytes()<<",\"pdc_samples\":0}\n";
}
