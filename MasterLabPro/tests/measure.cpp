#include "MeasurementModel.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main(){float a[]{1,-1,1,-1},b[]{.5f,-.5f,.5f,-.5f};auto m=ml::measure(a,b,4);assert(m.clip[0]&&!m.clip[1]);assert(m.peak[0]==0);assert(std::abs(m.rms[1]+6.0206f)<.001f);assert(ml::measure(nullptr,nullptr,0).peak[0]==-120);assert(ml::frequencyX(20)==0);assert(std::abs(ml::frequencyX(20000)-1)<.0001);assert(ml::levelDifference(-30,-24)==6);assert(ml::dbY(12)==0);std::cout<<"PASS measurement model\n";}
