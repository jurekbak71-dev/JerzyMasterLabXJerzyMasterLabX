#pragma once
#include "Dsp.h"
#include <vector>

namespace masterlab {
inline constexpr size_t brainSpectrumBins=60;
inline double brainFrequency(size_t i)noexcept{return 25*std::pow(2.,static_cast<double>(i)/6);}
struct BrainDetail {
 bool analysed=false;std::array<double,brainSpectrumBins> power{};
 std::array<float,brainSpectrumBins> prominence{},persistence{};
 float coverage=0,sectionLowDb=-120,sectionHighDb=-120,sectionSpreadDb=0,sectionJumpDb=0,windowSpreadDb=0;
 std::vector<float> sectionLevelsDb; // Chronological ~3-second RMS sections, computed off audio thread.

};
inline std::array<float,brainSpectrumBins> spectralProminence(const std::array<double,brainSpectrumBins>& power){
 std::array<float,brainSpectrumBins> db{},out{};
 for(size_t i=0;i<db.size();++i)db[i]=static_cast<float>(10*std::log10(std::max(1e-30,power[i])));
 for(size_t i=9;i+9<db.size();++i){std::array<float,6> left{},right{};const bool high=brainFrequency(i)>4000;const size_t count=high?4:6,offset=high?6:9;
  for(size_t k=0;k<count;++k){left[k]=db[i-offset+k];right[k]=db[i+(high?3:4)+k];}std::sort(left.begin(),left.begin()+count);std::sort(right.begin(),right.begin()+count);
  const float l=(left[count/2-1]+left[count/2])*.5f,r=(right[count/2-1]+right[count/2])*.5f;
  // Avoid treating a codec/Nyquist roll-off as a resonant rise. Narrower high
  // frequency neighbourhoods retain local maxima next to a steep roll-off.
  const float baseline=std::max((l+r)*.5f,std::max(l,r)-4);out[i]=db[i]-baseline;
 }

 return out;
}
class BrainDetailAnalyzer {
 double rate=48000,windowPower=0,sectionPower=0;size_t windowCount=0,sectionCount=0,windowSize=19200,sectionSize=144000,frames=0;
 std::array<double,brainSpectrumBins> energy{};std::array<size_t,brainSpectrumBins> present{};
 std::vector<float> windows,sections;double coverage=0;
 std::array<double,16385> prefix{};
 static float percentile(std::vector<float> values,float fraction){if(values.empty())return -120;std::sort(values.begin(),values.end());float position=fraction*(values.size()-1);size_t i=static_cast<size_t>(position);return values[i]+(position-i)*(values[std::min(i+1,values.size()-1)]-values[i]);}
 static void append(std::vector<float>& out,double sum,size_t count){if(count)out.push_back(static_cast<float>(10*std::log10(std::max(1e-30,sum/count))));}
public:
 void prepare(double sr,double seconds){*this=BrainDetailAnalyzer{};rate=sr;windowSize=static_cast<size_t>(std::ceil(sr*.4));sectionSize=static_cast<size_t>(std::ceil(sr*3));windows.reserve(static_cast<size_t>(seconds/.4)+2);sections.reserve(static_cast<size_t>(seconds/3)+2);}
 // Worker thread only: the audio callback still only writes fixed FIFO packets.
 void push(float l,float r){const double p=(static_cast<double>(l)*l+static_cast<double>(r)*r)*.5;windowPower+=p;sectionPower+=p;if(++windowCount==windowSize){append(windows,windowPower,windowCount);windowPower=0;windowCount=0;}if(++sectionCount==sectionSize){append(sections,sectionPower,sectionCount);sectionPower=0;sectionCount=0;}}
 void spectrum(const float* l,const float* r,int n){
  const int half=n/2;prefix[0]=0;for(int i=0;i<half;++i)prefix[static_cast<size_t>(i)+1]=prefix[static_cast<size_t>(i)]+(i?static_cast<double>(l[i])*l[i]+static_cast<double>(r[i])*r[i]:0);
  if(prefix[half]<n*static_cast<double>(n)*.375*1e-9)return;
  auto integral=[&](double hz){double position=std::clamp(hz*n/rate+.5,0.,static_cast<double>(half));int i=static_cast<int>(position);return prefix[i]+(i<half?(position-i)*(prefix[i+1]-prefix[i]):0);};
  std::array<double,brainSpectrumBins> frame{};double maxPower=0;
  for(size_t i=0;i<frame.size();++i){const double f=brainFrequency(i);if(f>std::min(20000.,rate*.45))continue;frame[i]=std::max(0.,integral(f*std::pow(2.,1./12))-integral(f/std::pow(2.,1./12)))/(n*static_cast<double>(n)*.375);energy[i]+=frame[i];maxPower=std::max(maxPower,frame[i]);}
  const auto excess=spectralProminence(frame);size_t occupied=0;for(size_t i=0;i<frame.size();++i){if(frame[i]>maxPower*std::pow(10.,-3.5)){++occupied;if(excess[i]>5)++present[i];}}coverage+=occupied;++frames;
 }
 BrainDetail finish()const{
  BrainDetail out;out.analysed=frames>=4;for(size_t i=0;i<energy.size();++i){out.power[i]=frames?energy[i]/frames:0;out.persistence[i]=frames?static_cast<float>(present[i])/frames:0;}out.prominence=spectralProminence(out.power);out.coverage=static_cast<float>(coverage/std::max(size_t{1},frames));
  auto levels=sections;append(levels,sectionPower,sectionCount);auto shortLevels=windows;append(shortLevels,windowPower,windowCount);
  auto gate=[](std::vector<float>& values){if(values.empty())return;const float floor=std::max(-70.f,*std::max_element(values.begin(),values.end())-40);values.erase(std::remove_if(values.begin(),values.end(),[&](float db){return db<floor;}),values.end());};gate(levels);gate(shortLevels);
  out.sectionLevelsDb=sections;
  if(sectionCount)append(out.sectionLevelsDb,sectionPower,sectionCount);
  out.sectionLowDb=percentile(levels,.1f);out.sectionHighDb=percentile(levels,.9f);out.sectionSpreadDb=std::max(0.f,out.sectionHighDb-out.sectionLowDb);for(size_t i=1;i<levels.size();++i)out.sectionJumpDb=std::max(out.sectionJumpDb,std::abs(levels[i]-levels[i-1]));out.windowSpreadDb=std::max(0.f,percentile(shortLevels,.9f)-percentile(shortLevels,.1f));return out;
 }
};
}
