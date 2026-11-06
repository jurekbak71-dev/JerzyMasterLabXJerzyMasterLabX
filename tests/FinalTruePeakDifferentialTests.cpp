#include "FinalTruePeak.h"
#include "FinalTruePeakReference.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(bool condition,const char* label){
 if(!condition){std::fprintf(stderr,"FAIL: %s\n",label);std::abort();}
}
int main(){
 for(double rate:{44100.,48000.,96000.,192000.}){
  for(int mode=0;mode<4;++mode){
   masterlab::FinalTruePeak optimized;
   masterlab_reference::FinalTruePeak reference;
   masterlab::FinalTruePeakSettings s;
   s.enabled=mode!=0;
   s.ceilingDb=mode==3?-11.f:-1.f;
   s.driveDb=mode==0?0.f:(mode==1?5.f:18.f);
   s.releaseMs=mode==3?10.f:135.f;
   masterlab_reference::FinalTruePeakSettings old;
   old.enabled=s.enabled;old.ceilingDb=s.ceilingDb;old.driveDb=s.driveDb;old.releaseMs=s.releaseMs;
   optimized.settings(s);reference.settings(old);
   optimized.prepare(rate);reference.prepare(rate);
   constexpr int total=24000;
   std::vector<float> left(total),right(total),leftReference(total),rightReference(total);
   for(int i=0;i<total;++i){
    const float impulse=((i%697)==0?2.5f:0.f);
    left[i]=static_cast<float>(std::sin(i*.9234)+.34*std::sin(i*.1723))+impulse;
    right[i]=static_cast<float>(.75*std::sin(i*.3729)-.21*std::cos(i*.0351))-impulse*.5f;
   }
   leftReference=left;rightReference=right;
   for(int offset=0;offset<total;){
    int n=std::min(1+(offset%511),total-offset);
    optimized.process(left.data()+offset,right.data()+offset,n);
    reference.process(leftReference.data()+offset,rightReference.data()+offset,n);
    for(int i=offset;i<offset+n;++i){
     check(std::abs(left[i]-leftReference[i])<1e-6f,
           "left render deviates from preoptimization reference");
     check(std::abs(right[i]-rightReference[i])<1e-6f,
           "right render deviates from preoptimization reference");
    }
    const auto newReading=optimized.reading();
    const auto oldReading=reference.reading();
    check(std::abs(newReading.gainReductionDb-oldReading.gainReductionDb)<1e-5f,
          "gain reduction telemetry changed");
    check(std::abs(newReading.detectedTruePeak-oldReading.detectedTruePeak)<1e-5f,
          "detected true peak telemetry changed");
    offset+=n;
   }
   // Ensure automation at block boundary does not change reference behavior.
   s.ceilingDb=-.3f;s.releaseMs=800.f;s.driveDb=3.7f;s.enabled=true;
   old.ceilingDb=s.ceilingDb;old.releaseMs=s.releaseMs;old.driveDb=s.driveDb;old.enabled=s.enabled;
   optimized.settings(s);reference.settings(old);
   std::array<float,257> a{},b{},c{},d{};
   for(int i=0;i<257;++i){a[i]=c[i]=i%5?-.8f:1.4f;b[i]=d[i]=i%7?.2f:-1.6f;}
   optimized.process(a.data(),b.data(),257);
   reference.process(c.data(),d.data(),257);
   for(int i=0;i<257;++i)check(a[i]==c[i]&&b[i]==d[i],"automation output diverged");
  }
  std::printf("%.0f: old and optimized limiter outputs match in all four modes\n",rate);
 }
}
