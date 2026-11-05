#include "HarmonicExciter.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
using masterlab::HarmonicExciter;
using masterlab::HarmonicExciterSettings;
static constexpr double pi=3.14159265358979323846;
static void require(bool ok,const char* label) {
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",label);std::abort();}
}
static double amplitude(const std::vector<float>& v,int harmonic,double frequency,double rate,size_t offset) {
    double real=0.,imag=0.;
    for(size_t i=offset;i<v.size();++i) {
        const double phase=2*pi*frequency*harmonic*static_cast<double>(i)/rate;
        real+=v[i]*std::cos(phase);imag+=v[i]*std::sin(phase);
    }
    return 2*std::hypot(real,imag)/static_cast<double>(v.size()-offset);
}
static std::vector<float> render(double sr,int frames,HarmonicExciterSettings cfg,int block) {
    HarmonicExciter effect;effect.settings(cfg);effect.prepare(sr);
    std::vector<float> l(frames),r(frames);
    constexpr double frequency=1000.;
    for(int i=0;i<frames;++i)
        l[i]=r[i]=0.2f*static_cast<float>(std::sin(2*pi*frequency*i/sr));
    for(int i=0;i<frames;i+=block)
        effect.process(l.data()+i,r.data()+i,std::min(frames-i,block));
    for(int i=0;i<frames;++i)
        require(std::isfinite(l[i])&&l[i]==r[i],"finite and dual mono");
    return l;
}
static std::vector<float> renderTone(double sr,double hz,int frames,HarmonicExciterSettings cfg,int block){
    HarmonicExciter effect;effect.settings(cfg);effect.prepare(sr);
    std::vector<float> l(frames),r(frames);
    for(int i=0;i<frames;++i)
        l[i]=r[i]=.5f*static_cast<float>(std::sin(2*pi*hz*i/sr));
    for(int i=0;i<frames;i+=block)
        effect.process(l.data()+i,r.data()+i,std::min(frames-i,block));
    return l;
}
int main() {
    for(double sr:{44100.,48000.,96000.,192000.}) {
        const int n=static_cast<int>(sr),offset=n/2;
        HarmonicExciterSettings cfg;
        auto off=render(sr,n,cfg,257);
        for(int i=0;i<n;++i)
            require(std::abs(off[i]-.2f*std::sin(2*pi*1000*i/sr))<2e-6,"disabled exact bypass");
        cfg.enabled=true;cfg.amount={1,1,1,1};cfg.even=1;cfg.odd=0;cfg.driveDb=12;
        auto even=render(sr,n,cfg,128);
        const double h2=amplitude(even,2,1000,sr,offset);
        require(h2>0.002,"even generates second harmonic");
        cfg.even=0;cfg.odd=1;
        auto odd=render(sr,n,cfg,128);
        const double h3=amplitude(odd,3,1000,sr,offset);
        require(h3>0.0004,"odd generates third harmonic");
        require(amplitude(odd,2,1000,sr,offset)<h2*.2,"odd excludes second harmonic");
        cfg.odd=0;
        auto neutral=render(sr,n,cfg,128);
        for(int i=0;i<n;++i)require(std::abs(neutral[i]-off[i])<1e-6,"zero harmonics null");
        cfg.even=1;
        auto small=render(sr,n,cfg,1),large=render(sr,n,cfg,4097);
        for(int i=0;i<n;++i)require(std::abs(small[i]-large[i])<1e-6,"block independence");
        std::printf("%.0f Hz: H2 %.7f H3 %.7f blocks null OK\n",sr,h2,h3);
    }
    // Multirate 2x stability and reported fixed internal latency.
    for(double sr:{44100.,48000.,96000.,192000.}){
        HarmonicExciterSettings settings;
        settings.enabled=true;settings.oversample2x=true;
        settings.amount={1,1,1,1};settings.even=.75f;settings.odd=1.f;settings.driveDb=12.f;
        HarmonicExciter effect;effect.settings(settings);effect.prepare(sr);
        require(effect.latencySamples()==32,"2x multirate latency");
        const int frames=static_cast<int>(sr/2);
        std::vector<float> left(frames),right(frames);
        for(int i=0;i<frames;++i)left[i]=right[i]=.2f*static_cast<float>(std::sin(2*pi*1000*i/sr));
        for(int i=0;i<frames;i+=257)effect.process(left.data()+i,right.data()+i,std::min(257,frames-i));
        for(int i=0;i<frames;++i)require(std::isfinite(left[i])&&std::isfinite(right[i]),"2x multirate finite");
    }
    // Full-core 2x mode: verified latency alignment and reduced folded H3 at 10 kHz.
    {
        HarmonicExciterSettings cfg;cfg.enabled=true;cfg.oversample2x=true;
        cfg.amount={0,0,0,0};cfg.even=0;cfg.odd=0;
        HarmonicExciter effect;effect.settings(cfg);effect.prepare(48000);
        require(effect.latencySamples()==32,"2x latency declared");
        constexpr int frames=1024;std::vector<float> l(frames),r(frames),dry(frames);
        for(int i=0;i<frames;++i){dry[i]=.2f*std::sin(2*pi*1000*i/48000);l[i]=r[i]=dry[i];}
        effect.process(l.data(),r.data(),frames);
        for(int i=32;i<frames;++i)require(std::abs(l[i]-dry[i-32])<1e-6f,"delayed dry exactly aligned");
        cfg.amount={1,1,1,1};cfg.odd=1;cfg.driveDb=12.f;
        cfg.oversample2x=false;
        const auto raw=renderTone(48000,10000,48000,cfg,257);
        cfg.oversample2x=true;
        const auto filtered=renderTone(48000,10000,48000,cfg,257);
        const double aliasedRaw=amplitude(raw,18,1000,48000,24000);
        const double aliasedFiltered=amplitude(filtered,18,1000,48000,24000);
        std::printf("Full exciter 10k -> 18k fold: raw=%f 2x=%f\\n",aliasedRaw,aliasedFiltered);
        require(aliasedRaw>1e-4,"alias test meaningful");
        require(aliasedFiltered<aliasedRaw*.25,"full exciter suppresses alias");
        const auto blocks=renderTone(48000,10000,48000,cfg,1);
        for(size_t i=0;i<blocks.size();++i)require(std::abs(blocks[i]-filtered[i])<1e-6f,"2x block independence");
    }
    // Automated enable/bypass and drive changes should be bounded and converge.
    {
        HarmonicExciter e;HarmonicExciterSettings cfg;cfg.enabled=false;
        cfg.amount={1,1,1,1};cfg.even=1;cfg.odd=1;cfg.driveDb=18;
        e.settings(cfg);e.prepare(48000);
        std::array<float,4096> left{},right{};
        for(size_t i=0;i<left.size();++i)left[i]=right[i]=.3f*std::sin(2*pi*1000*i/48000);
        e.process(left.data(),right.data(),static_cast<int>(left.size()));
        cfg.enabled=true;e.settings(cfg);
        std::array<float,4096> wetLeft{},wetRight{};
        for(size_t i=0;i<wetLeft.size();++i)wetLeft[i]=wetRight[i]=.3f*std::sin(2*pi*1000*i/48000);
        const float original=wetLeft[0];e.process(wetLeft.data(),wetRight.data(),4096);
        require(std::abs(wetLeft[0]-original)<.0002f,"enable ramp begins near dry");
        for(float v:wetLeft)require(std::isfinite(v),"automated enable finite");
        cfg.driveDb=0;cfg.enabled=false;e.settings(cfg);
        std::array<float,4096> bypassLeft{},bypassRight{};
        for(size_t i=0;i<bypassLeft.size();++i)bypassLeft[i]=bypassRight[i]=.3f*std::sin(2*pi*1000*i/48000);
        // Time-aligned continuous wet reference is essential for a valid click test.
        HarmonicExciter held=e;
        cfg.enabled=true;
        held.settings(cfg);
        std::array<float,4096> heldLeft=bypassLeft,heldRight=bypassRight;
        held.process(heldLeft.data(),heldRight.data(),4096);
        e.process(bypassLeft.data(),bypassRight.data(),4096);
        require(std::abs(bypassLeft[0]-heldLeft[0])<.002f,"disable starts near continuous wet reference");
        for(float v:bypassLeft)require(std::isfinite(v),"automated disable finite");
    }
    HarmonicExciter effect;HarmonicExciterSettings on;on.enabled=true;on.amount={1,1,1,1};
    effect.settings(on);effect.prepare(48000);
    std::array<float,128> l{},r{};effect.process(l.data(),r.data(),128);
    for(float x:l)require(x==0.f,"silence stability");
    std::puts("Harmonic Exciter prototype: PASS");
}
