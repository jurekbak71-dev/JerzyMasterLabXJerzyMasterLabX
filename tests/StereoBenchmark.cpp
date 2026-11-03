#include "ModularDsp.h"
#include <chrono>
#include <iostream>
int main(){for(int rate:{44100,48000,96000}){
 masterlab::StereoImager dsp;masterlab::StereoSettings c;c.enabled=true;
#ifdef STEREOIZE
 c.stereoize=.7f;
#endif
 dsp.settings(c);dsp.prepare(rate);std::array<float,257> l{},r{};double sum=0;auto begin=std::chrono::steady_clock::now();
 for(int block=0;block<2000;++block){for(int i=0;i<257;++i)l[i]=r[i]=.1f*std::sin((block*257+i)*.03f);dsp.process(l.data(),r.data(),257);sum+=l[0];}
 double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();std::cout<<rate<<','<<elapsed<<','<<(2000.*257/rate)/elapsed<<','<<sum<<'\n';}}
