#include "AudioRack.h"
#include "AnalysisHistory.h"
#include <memory>
#include <iostream>
int main(){
 auto h=std::make_unique<ml::AnalysisHistory>();
 auto f=std::make_unique<ml::AnalysisFrame>();f->sampleRate=48000;f->endSample=4096;
 for(std::size_t i=0;i<ml::fftSize;++i){
  float x=float(.25*std::sin(2*std::numbers::pi*128*i/ml::fftSize));
  for(int c=0;c<2;++c){f->samples[0][c][i]=x;f->samples[1][c][i]=2*x;f->samples[2][c][i]=x;}
 }
 if(!h->accept(*f)||!h->column(299).valid||h->column(298).valid)return 1;
 if(std::abs(h->spectra[1][128]-h->spectra[0][128]-6.0206f)>.001f)return 2;
 if(h->accept(*f))return 3;
 f->endSample+=4800;if(!h->accept(*f)||h->column(298).valid||!h->column(299).valid||!h->column(296).valid)return 4;
 f->sourceId=2;++f->epoch;f->samples={};if(!h->accept(*f)||h->column(298).valid)return 5;
 for(float x:h->column(299).values[2])if(x!=0)return 6;
 auto rack=std::make_unique<ml::AudioRack>();rack->prepare(48000);
 ml::RackConfig c;c.stages[0]={1,6.0205999f};c.listenDelta=true;
 if(!rack->submit(c))return 7;
 std::array<float,1920> l,r;l.fill(.125f);r.fill(-.25f);rack->process(l.data(),r.data(),l.size());
 if(std::abs(l.back()-.125f)>1e-5f||std::abs(r.back()+.25f)>1e-5f)return 8;
 ml::RackMeters m;if(!rack->poll(m)||std::abs(m.output.peak[0]+12.0412f)>.001f)return 9;
 c.listenDelta=false;if(!rack->submit(c))return 10;
 l.fill(.125f);r.fill(-.25f);rack->process(l.data(),r.data(),l.size());
 if(std::abs(l.back()-.25f)>1e-5f)return 11;
 std::cout<<"PASS spectral history, gaps, source reset, silence, delta audition, independent meters\n";
}
