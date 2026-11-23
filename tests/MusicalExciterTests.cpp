#include "AnalogExciter.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace masterlab;
static constexpr double pi=3.14159265358979323846;
static void require(bool ok,const char* message){
 if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::abort();}
}
static double component(const std::vector<float>& signal,double rate,int multiple,int offset){
 const double hz=997.*multiple;double re=0.,im=0.;
 for(int i=offset;i<static_cast<int>(signal.size());++i){
  const double angle=2.*pi*hz*i/rate;
  re+=signal[static_cast<size_t>(i)]*std::cos(angle);
  im+=signal[static_cast<size_t>(i)]*std::sin(angle);
 }
 return 2.*std::hypot(re,im)/static_cast<double>(signal.size()-offset);
}
static std::vector<float> render(double rate,AnalogizerMode mode,float excite,int block){
 AnalogExciter fx;
 HarmonicExciterSettings h;h.adaa=true;h.enabled=false;
 fx.harmonicSettings(h);
 AnalogizerSettings colour;colour.enabled=true;colour.mode=mode;colour.mix=.65f;
 colour.autoGain=true;
 fx.colourSettings(colour);
 fx.excitement(excite);
 fx.prepare(rate);
 const int samples=static_cast<int>(rate*.4);
 std::vector<float> left(samples),right(samples);
 for(int i=0;i<samples;++i){
  const float x=.18f*static_cast<float>(std::sin(2.*pi*997.*i/rate));
  left[static_cast<size_t>(i)]=right[static_cast<size_t>(i)]=x;
 }
 for(int pos=0;pos<samples;pos+=block){
  const int n=std::min(block,samples-pos);
  fx.process(left.data()+pos,right.data()+pos,n);
 }
 for(int i=0;i<samples;++i)
  require(std::isfinite(left[static_cast<size_t>(i)])&&
          left[static_cast<size_t>(i)]==right[static_cast<size_t>(i)],
          "EXCITE produced invalid or nonlinked stereo output");
 return left;
}
int main(){
 for(const double rate:{44100.,48000.,96000.}){
  std::array<std::vector<float>,3> modes{};
  for(int mode=0;mode<3;++mode){
   const auto m=static_cast<AnalogizerMode>(mode==0?0:(mode==1?1:2));
   modes[static_cast<size_t>(mode)]=render(rate,m,1.f,257);
   const auto sampleBySample=render(rate,m,1.f,1);
   const auto wide=render(rate,m,1.f,4097);
   double energy=0.,maxPeak=0.;
   for(size_t i=0;i<sampleBySample.size();++i){
    const float y=modes[static_cast<size_t>(mode)][i];
    require(std::abs(y-sampleBySample[i])<2.e-6f&&
            std::abs(y-wide[i])<2.e-6f,"EXCITE changes with DAW buffer size");
    if(i>=4096){energy+=static_cast<double>(y)*y;maxPeak=std::max(maxPeak,std::abs(static_cast<double>(y)));}
   }
   const double rms=std::sqrt(energy/(sampleBySample.size()-4096));
   const double inputRms=.18/std::sqrt(2.);
   require(rms>inputRms*.40&&rms<inputRms*2.,"EXCITE changes loudness excessively");
   require(maxPeak<.85,"EXCITE unexpectedly overdrives a moderate source");
   const auto& audio=modes[static_cast<size_t>(mode)];
   const double fundamental=component(audio,rate,1,4096);
   double harmonicPower=0.;
   std::array<double,4> harmonics{};
   for(int h=2;h<=5;++h){
    harmonics[static_cast<size_t>(h-2)]=component(audio,rate,h,4096);
    harmonicPower+=harmonics[static_cast<size_t>(h-2)]*harmonics[static_cast<size_t>(h-2)];
   }
   const double thd5=std::sqrt(harmonicPower)/std::max(1.e-8,fundamental);
   require(std::isfinite(thd5)&&thd5<.25,
           "EXCITE adds excessive H2-H5 distortion at moderate input");
   std::printf("%.0f Hz model %d: RMS %.6f peak %.6f H2 %.6f H3 %.6f H4 %.6f H5 %.6f THD5 %.3f%%\n",
               rate,mode,rms,maxPeak,harmonics[0],harmonics[1],
               harmonics[2],harmonics[3],thd5*100.);
  }
  for(int a=0;a<3;++a)for(int b=a+1;b<3;++b){
   double delta=0.;
   for(size_t i=4096;i<modes[static_cast<size_t>(a)].size();++i){
    const double d=modes[static_cast<size_t>(a)][i]-modes[static_cast<size_t>(b)][i];
    delta+=d*d;
   }
   require(delta>1.e-5,"Tube/FET/Tape are not sonically distinguishable");
  }
  const auto off=render(rate,AnalogizerMode::Tube,0.f,257);
  const auto on=render(rate,AnalogizerMode::Tube,1.f,257);
  double difference=0.;
  for(size_t i=4096;i<on.size();++i){
   const double d=on[i]-off[i];difference+=d*d;
  }
  require(difference>1.e-6,"EXCITE knob fails to change actual DSP");
 }
 std::puts("Unified Analog Exciter sonic safety tests PASS");
}
