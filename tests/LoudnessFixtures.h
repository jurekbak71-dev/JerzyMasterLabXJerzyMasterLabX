#pragma once
#include <array>
#include <cmath>
inline std::array<float,2> loudnessFixture(int kind,int sample,double rate){
 const double t=sample/rate,tau=6.283185307179586;
 double left=.1*std::sin(tau*1000*t),right=left;
 if(kind==1){left=.08*std::sin(tau*440*t)+.03*std::sin(tau*1999*t);right=.06*std::sin(tau*700*t)+.04*std::cos(tau*3700*t);}
 if(kind==2){double a=t<3?.02:(t<8?.25:.04);left=a*std::sin(tau*1000*t);right=left;}
 if(kind==3){double a=t<4?.18:(t<8?0:.00001);left=a*std::sin(tau*83*t);right=-left;}
 return {static_cast<float>(left),static_cast<float>(right)};
}
