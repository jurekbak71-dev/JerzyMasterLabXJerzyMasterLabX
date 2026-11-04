#include "SerialGraph.h"
#include "LoudnessMeter.h"
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using Audio=std::array<std::vector<float>,2>;
Audio stimulus(int rate){Audio x;for(auto& c:x)c.resize(rate*5);unsigned seed=71;for(int i=0;i<rate*5;++i){seed=1664525*seed+1013904223;const float noise=(float(seed>>8)/8388608.f-1)*.025f;const double t=double(i)/rate;const float mid=.12f*std::sin(6.283185307*83*t)+.07f*std::sin(6.283185307*713*t)+.04f*std::sin(6.283185307*4103*t)+noise;const float side=.035f*std::sin(6.283185307*1307*t)+noise*.35f;x[0][i]=mid+side;x[1][i]=mid-side;}return x;}
Audio render(const Audio& input,int rate,int mode){
 Audio out=input;masterlab::SerialGraph graph;graph.parameters(1200,12,.7f,-1,mode!=0,false);masterlab::DynamicsSettings dynamics;dynamics.enabled=false;graph.dynamicsSettings(dynamics);masterlab::StereoSettings stereo;stereo.enabled=false;graph.stereoSettings(stereo);
 if(mode){std::array<float,6> trims{},mixes{1,1,1,1,1,1};if(mode==2){trims[1]=-6;trims[2]=-6;trims[3]=6;}graph.parallelSettings(mode==2,trims,mixes);graph.topology(true,{false,true,true,true,false,false});}
 graph.prepare(rate);std::array<float,257> trace{};for(int offset=0;offset<int(out[0].size());offset+=257){const int n=std::min(257,int(out[0].size())-offset);graph.process(out[0].data()+offset,out[1].data()+offset,n,false,trace.data());}return out;
}
struct Metrics{double lufs=-120,peak=0,corr=0;};
Metrics measure(const Audio& audio,int rate){masterlab::LoudnessMeter meter;meter.prepare(rate);double ll=0,rr=0,lr=0;Metrics out;for(size_t i=0;i<audio[0].size();++i){const double l=audio[0][i],r=audio[1][i];meter.push(float(l),float(r));if(i>=size_t(rate)){ll+=l*l;rr+=r*r;lr+=l*r;out.peak=std::max({out.peak,std::abs(l),std::abs(r)});}}out.lufs=meter.read().integrated;out.corr=lr/std::max(1e-30,std::sqrt(ll*rr));return out;}
double matchedDelta(const Audio& dry,const Audio& wet,int rate,double gain){double delta=0,energy=0;for(size_t i=rate;i<dry[0].size();++i)for(int c=0;c<2;++c){const double d=wet[c][i]*gain-dry[c][i];delta+=d*d;energy+=static_cast<double>(dry[c][i])*dry[c][i];}return std::sqrt(delta/std::max(1e-30,energy));}
void wav(const std::filesystem::path& path,const Audio& audio,int rate,double gain){std::ofstream f(path,std::ios::binary);auto u16=[&](unsigned v){f.put(char(v));f.put(char(v>>8));};auto u32=[&](unsigned v){u16(v);u16(v>>16);};const unsigned bytes=unsigned(audio[0].size()*4);f.write("RIFF",4);u32(36+bytes);f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(rate);u32(rate*4);u16(4);u16(16);f.write("data",4);u32(bytes);for(size_t i=0;i<audio[0].size();++i)for(int c=0;c<2;++c){const float x=std::clamp(float(audio[c][i]*gain),-.999f,.999f);u16(static_cast<unsigned>(static_cast<short>(std::lround(x*32767))));}}
int main(int argc,char** argv){if(argc!=2){std::cerr<<"usage: parallel_evidence output-directory\n";return 2;}const int rate=48000;std::filesystem::path directory=argv[1];std::filesystem::create_directories(directory);const auto source=stimulus(rate),dry=render(source,rate,0),legacy=render(source,rate,1),focused=render(source,rate,2);const auto dryM=measure(dry,rate),legacyM=measure(legacy,rate),focusedM=measure(focused,rate);const double legacyGain=std::pow(10.,(dryM.lufs-legacyM.lufs)/20),focusedGain=std::pow(10.,(dryM.lufs-focusedM.lufs)/20);wav(directory/"parallel-dry.wav",dry,rate,1);wav(directory/"parallel-legacy-1-over-n-matched.wav",legacy,rate,legacyGain);wav(directory/"parallel-focused-compensated-matched.wav",focused,rate,focusedGain);std::ostringstream report;report<<"render,lufs_raw,peak_raw,correlation,match_gain_db,matched_delta_ratio\n";report<<"dry,"<<dryM.lufs<<','<<dryM.peak<<','<<dryM.corr<<",0,0\n";report<<"legacy_1_over_n,"<<legacyM.lufs<<','<<legacyM.peak<<','<<legacyM.corr<<','<<20*std::log10(legacyGain)<<','<<matchedDelta(dry,legacy,rate,legacyGain)<<'\n';report<<"focused_compensated,"<<focusedM.lufs<<','<<focusedM.peak<<','<<focusedM.corr<<','<<20*std::log10(focusedGain)<<','<<matchedDelta(dry,focused,rate,focusedGain)<<'\n';std::ofstream(directory/"parallel-metrics.csv")<<report.str();std::cout<<report.str();}
