#include "GraphConfiguration.h"
#include "ModuleParameters.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

// The final slot's OFF switch must bypass both constituent limiters even
// when an old limiter enable remains in the current parameter state.
static std::vector<float> render(bool oldLimiterOn, bool finalOn, int block) {
    std::array<float, masterlab::parameterCount> values{};
    values[0]=1000; values[2]=1; values[3]=-12;
    values[5]=oldLimiterOn; values[87]=1;
    values[53]=120;
    values[221]=1; values[222]=finalOn; values[223]=-12; values[224]=120;
    for (size_t i=0;i<masterlab::defaultFullOrder.size();++i)
        values[227+i]=static_cast<float>(masterlab::defaultFullOrder[i]);
    masterlab::SerialGraph graph;
    masterlab::configureGraph(graph,[&](size_t i){return values[i];});
    graph.prepare(48000);
    std::vector<float> left(12000),right(left.size()),trace(block);
    for(size_t i=0;i<left.size();++i) left[i]=right[i]=.8f*std::sin(i*.13f);
    for(int i=0;i<static_cast<int>(left.size());i+=block)
        graph.process(left.data()+i,right.data()+i,std::min(block,static_cast<int>(left.size())-i),false,trace.data());
    return left;
}
int main(){
    for(int block:{1,257,4097}) {
        const auto bypass=render(false,false,block);
        const auto legacy=render(true,false,block);
        const auto enabled=render(true,true,block);
        double error=0; float onPeak=0,offPeak=0;
        for(size_t i=4096;i<bypass.size();++i){
            error=std::max(error,static_cast<double>(std::abs(bypass[i]-legacy[i])));
            onPeak=std::max(onPeak,std::abs(enabled[i]));
            offPeak=std::max(offPeak,std::abs(bypass[i]));
        }
        std::printf("block=%d OFF legacy-toggle delta=%.9f OFF peak=%.6f ON peak=%.6f\n",block,error,offPeak,onPeak);
        if(error>1.e-6 || offPeak<.79f || onPeak>.2512f || onPeak<.05f) return 1;
    }
}
