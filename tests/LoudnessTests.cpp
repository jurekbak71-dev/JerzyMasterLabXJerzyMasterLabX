#include "LoudnessMeter.h"
#include "LoudnessFixtures.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
 std::ifstream input(MASTERLAB_LOUDNESS_REFERENCE);check(static_cast<bool>(input),"reference data missing");std::string line;std::getline(input,line);int cases=0;
 while(std::getline(input,line)){
  for(auto& c:line)if(c==',')c=' ';std::istringstream row(line);int rate,kind;float integrated,momentary,shortTerm;check(static_cast<bool>(row>>rate>>kind>>integrated>>momentary>>shortTerm),"bad reference row");
  masterlab::LoudnessMeter meter;meter.prepare(rate);
  for(int i=0;i<rate*12;++i){auto sample=loudnessFixture(kind,i,rate);meter.push(sample[0],sample[1]);}
  auto actual=meter.read();check(std::abs(actual.integrated-integrated)<=.11f&&std::abs(actual.momentary-momentary)<=.11f&&std::abs(actual.shortTerm-shortTerm)<=.11f,"FFmpeg ebur128 reference mismatch");++cases;
 }
 check(cases==16,"expected 16 independent reference cases");
 masterlab::LoudnessMeter meter;meter.prepare(48000);
 for(int i=0;i<48000*4;++i){auto sample=loudnessFixture(0,i,48000);meter.push(sample[0],sample[1]);}
 check(meter.read().integrated>-21&&meter.read().integrated<-19,"steady integrated calibration");meter.reset();
 for(int i=0;i<48000*4;++i)meter.push(0,0);
 check(meter.read().integrated==-120&&meter.read().momentary==-120&&meter.read().shortTerm==-120,"silence after reset retains old loudness");
 for(int block:{1,257,4097}){
  meter.prepare(48000);
  for(int offset=0;offset<48000*12;offset+=block)for(int i=offset;i<std::min(48000*12,offset+block);++i){auto sample=loudnessFixture(2,i,48000);meter.push(sample[0],sample[1]);}
  check(std::abs(meter.read().integrated+12.3f)<.11f,"host buffer affects integrated loudness");
 }
 std::cout<<"Passed: 16 FFmpeg ebur128 reference cases, M/S/I, four sample rates, dual gating, silence, reset and variable blocks (0.11 LU tolerance)\n";
}
