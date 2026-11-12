#include "SerialGraph.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void require(bool condition,const char* message){
 if(!condition){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}
}

int main(){
 constexpr double rate=48000.;constexpr int count=24000;
 masterlab::FinalTruePeakSettings settings;settings.enabled=true;settings.ceilingDb=-6.f;
 settings.driveDb=10.f;settings.releaseMs=90.f;

 masterlab::SerialGraph graph;
 graph.parameters(1000.f,0.f,1.f,settings.ceilingDb,false,true);
 graph.finalTruePeakSettings(settings);
 graph.extendedRouting(true,masterlab::defaultFullOrder);
 graph.prepare(rate);

 masterlab::AnalogExciter colourDelay;colourDelay.prepare(rate);
 masterlab::FinalTruePeak reference;reference.settings(settings);reference.prepare(rate);
 const int engineDelay=static_cast<int>(std::ceil(rate*.005));
 std::array<std::vector<float>,2> delay{std::vector<float>(engineDelay),std::vector<float>(engineDelay)};
 size_t position=0;

 std::vector<float> graphL(count),graphR(count),referenceL(count),referenceR(count),trace(257);
 for(int i=0;i<count;++i){
  const float impulse=i%997==0?1.4f:0.f;
  const float sample=.58f*std::sin(i*.173f)+.31f*std::sin(i*.037f)+impulse;
  graphL[i]=referenceL[i]=sample;graphR[i]=referenceR[i]=-.73f*sample;
 }
 for(int offset=0;offset<count;offset+=257){
  const int n=std::min(257,count-offset);
  graph.process(graphL.data()+offset,graphR.data()+offset,n,false,trace.data());
  colourDelay.process(referenceL.data()+offset,referenceR.data()+offset,n);
  for(int i=0;i<n;++i){
   for(int channel=0;channel<2;++channel){
    float& sample=(channel==0?referenceL:referenceR)[offset+i];
    const float delayed=delay[channel][position];delay[channel][position]=sample;sample=delayed;
   }
   position=(position+1)%delay[0].size();
  }
  reference.process(referenceL.data()+offset,referenceR.data()+offset,n);
 }
 double maximumError=0.;
 for(int i=0;i<count;++i)maximumError=std::max(maximumError,
  std::max(std::abs(static_cast<double>(graphL[i]-referenceL[i])),
           std::abs(static_cast<double>(graphR[i]-referenceR[i]))));
 std::printf("single-stage reference maximum error %.9f\n",maximumError);
 require(maximumError<1.e-6,"serial final slot applies another limiter before FinalTruePeak");
}
