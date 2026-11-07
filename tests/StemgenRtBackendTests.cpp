#include "StemgenRtBackend.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#define CHECK(x) do {if(!(x))throw std::runtime_error(#x);}while(false)
int main(int argc,char** argv){
 if(argc!=2)return 2;
 using namespace masterlab;
 StemgenRtBackend serial(argv[1],false),parallel(argv[1],true);
 LiveStemHop input{},previous{};SeparatedStemHop reference{},actual{};
 for(int hop=0;hop<48;++hop){
  for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<128;++i)
   input[c][i]=.2f*std::sin(float(hop*128+i)*(.017f+float(c)*.003f));
  const auto a=serial.process(input,reference),b=parallel.process(input,actual);
  CHECK(a==b);CHECK(a==(hop>0));
  if(a)for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<128;++i){
   float sum=0;
   for(std::size_t s=0;s<4;++s){CHECK(std::isfinite(actual[s][c][i]));CHECK(std::abs(actual[s][c][i]-reference[s][c][i])<2.e-5f);sum+=actual[s][c][i];}
   CHECK(std::abs(sum-previous[c][i])<2.e-6f);
  }
  previous=input;
 }
 parallel.reset();serial.reset();input={};
 CHECK(!parallel.process(input,actual));CHECK(!serial.process(input,reference));
 CHECK(parallel.process(input,actual));
 for(const auto& stem:actual)for(const auto& channel:stem)for(float x:channel)CHECK(std::isfinite(x));
 input[0][0]=std::numeric_limits<float>::quiet_NaN();bool threw=false;
 try{parallel.process(input,actual);}catch(const std::invalid_argument&){threw=true;}CHECK(threw);
 parallel.reset();input={};CHECK(!parallel.process(input,actual));
 std::cout<<"StemgenRtBackend: actual ONNX serial/parallel waveform parity, previous-hop reconstruction, reset, silence and invalid input passed\n";
}
