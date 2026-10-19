#include "LoudnessMeter.h"
#include "LoudnessFixtures.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 if(argc!=4)return 1;int rate=std::stoi(argv[1]),kind=std::stoi(argv[2]);std::ofstream raw(argv[3],std::ios::binary);if(!raw)return 1;
 masterlab::LoudnessMeter meter;meter.prepare(rate);
 for(int i=0;i<rate*12;++i){auto sample=loudnessFixture(kind,i,rate);meter.push(sample[0],sample[1]);raw.write(reinterpret_cast<const char*>(sample.data()),sizeof(float)*2);}
 auto r=meter.read();std::cout<<r.integrated<<","<<r.momentary<<","<<r.shortTerm<<"\n";
}
