#include "BassArchitect.h"
#include "TransientSculptor.h"
#include "FinalTruePeak.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

template<class Processor,class Settings>
auto render(int block, double rate) {
    Processor dsp; Settings settings; settings.enabled=true;
    dsp.settings(settings); dsp.prepare(rate);
    std::vector<float> l(12017),r(l.size());
    for(size_t i=0;i<l.size();++i) l[i]=r[i]=.2f*std::sin(6.283185307179586*997*i/rate);
    for(int i=0;i<static_cast<int>(l.size());i+=block)
        dsp.process(l.data()+i,r.data()+i,std::min(block,static_cast<int>(l.size())-i));
    return dsp.reading();
}
template<class Processor,class Settings>
bool check(const char* name,double rate) {
    const auto one=render<Processor,Settings>(1,rate);
    double worst=0,energy=0;
    for(int block:{17,128,257,4097}) {
        const auto other=render<Processor,Settings>(block,rate);
        for(size_t i=0;i<one.inputWave.size();++i) {
            worst=std::max(worst,static_cast<double>(std::abs(one.inputWave[i]-other.inputWave[i])));
            worst=std::max(worst,static_cast<double>(std::abs(one.outputWave[i]-other.outputWave[i])));
        }
    }
    for(float x:one.inputWave) energy+=x*x;
    std::printf("%s rate=%.0f scope partition error=%.9f energy=%.6f\n",name,rate,worst,energy);
    return worst<1.e-6 && energy>.01;
}
template<class Processor,class Settings>
bool resetCheck(const char* name){
    Processor dsp;Settings s;s.enabled=true;dsp.settings(s);dsp.prepare(48000);
    std::vector<float> l(12000,.2f),r(l);
    dsp.process(l.data(),r.data(),static_cast<int>(l.size()));
    dsp.reset();
    std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);
    dsp.process(l.data(),r.data(),static_cast<int>(l.size()));
    auto reading=dsp.reading();
    for(float x:reading.inputWave)if(x!=0)return false;
    for(float x:reading.outputWave)if(x!=0)return false;
    std::printf("%s reset/silence clears capture PASS\n",name);return true;
}
template<class Processor,class Settings>
bool bypassCaptureCheck(const char* name){
    Processor dsp;Settings s;s.enabled=true;dsp.settings(s);dsp.prepare(48000);
    std::vector<float> l(24000,.2f),r(l);
    dsp.process(l.data(),r.data(),static_cast<int>(l.size()));
    s.enabled=false;dsp.settings(s);
    std::fill(l.begin(),l.end(),.1f);std::fill(r.begin(),r.end(),.1f);
    dsp.process(l.data(),r.data(),static_cast<int>(l.size()));
    s.enabled=true;dsp.settings(s);float a=.3f,b=.3f;dsp.process(&a,&b,1);
    for(float x:dsp.reading().inputWave)if(x!=0){
        std::printf("FAIL %s capture spans bypass discontinuity\n",name);return false;
    }
    return true;
}
int main(){
    bool ok=true;
    {
        masterlab::FinalTruePeak limiter;
        limiter.prepare(48000);
        float l=0,r=0;limiter.process(&l,&r,1);
        for(float gain:limiter.reading().gainEnvelope)if(gain!=1.f){
            std::puts("FAIL: inactive limiter scope reports non-unity gain");ok=false;break;
        }
    }
    for(double rate:{44100.,48000.,96000.,192000.}) {
        ok=check<masterlab::BassArchitect,masterlab::BassArchitectSettings>("Bass",rate)&&ok;
        ok=check<masterlab::TransientSculptor,masterlab::TransientSculptorSettings>("Transient",rate)&&ok;
        ok=check<masterlab::FinalTruePeak,masterlab::FinalTruePeakSettings>("Limiter",rate)&&ok;
    }
    ok=resetCheck<masterlab::BassArchitect,masterlab::BassArchitectSettings>("Bass")&&ok;
    ok=resetCheck<masterlab::TransientSculptor,masterlab::TransientSculptorSettings>("Transient")&&ok;
    ok=resetCheck<masterlab::FinalTruePeak,masterlab::FinalTruePeakSettings>("Limiter")&&ok;
    ok=bypassCaptureCheck<masterlab::BassArchitect,masterlab::BassArchitectSettings>("Bass")&&ok;
    ok=bypassCaptureCheck<masterlab::TransientSculptor,masterlab::TransientSculptorSettings>("Transient")&&ok;
    ok=bypassCaptureCheck<masterlab::FinalTruePeak,masterlab::FinalTruePeakSettings>("Limiter")&&ok;
    return ok?0:1;
}
