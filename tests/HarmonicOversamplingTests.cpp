#include "HarmonicOversampler.h"
#include <cmath>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <cstdlib>
using namespace masterlab;
constexpr double pi=3.141592653589793;
static double amp(const std::vector<float>& v,double hz,double sr,int start){double re=0,im=0;for(int i=start;i<(int)v.size();++i){auto th=2*pi*hz*i/sr;re+=v[i]*cos(th);im+=v[i]*sin(th);}return 2*hypot(re,im)/(v.size()-start);}
int main(){
 for(double sr:{44100.,48000.,96000.,192000.}){
  HarmonicOversampler2x os; std::vector<float> identity((int)sr);
  for(int i=0;i<(int)identity.size();++i) identity[i]=os.tick(.2f*sin(2*pi*1000*i/sr),[](float x){return x;});
  auto a=amp(identity,1000,sr,(int)sr/2);printf("sr %.0f identity amplitude %.6f\n",sr,a);if(std::abs(a-.2)>0.005) return 1;
 }
 const double sr=48000, hz=10000; const int n=48000;std::vector<float> base(n),over(n);
 HarmonicOversampler2x os;
 for(int i=0;i<n;++i){float x=.5f*sin(2*pi*hz*i/sr);base[i]=x*x*x;over[i]=os.tick(x,[](float v){return v*v*v;});}
 auto a=amp(base,18000,sr,n/2), b=amp(over,18000,sr,n/2);
 printf("alias 18k: raw %.8f oversampled %.8f ratio %.5f\n",a,b,b/a);
 if(!(b<a*.25))return 2;
 os.reset();for(int i=0;i<1000;++i){float y=os.tick(0.f,[](float v){return v*v*v;});if(!std::isfinite(y)||std::abs(y)>1e-8f)return 3;}
 return 0;
}
