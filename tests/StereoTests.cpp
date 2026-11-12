#include "ModularDsp.h"
#include "LoudnessMeter.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cstdlib>
using Audio=std::array<std::vector<float>,2>;
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
Audio stimulus(int rate,int kind){
 Audio x;for(auto& c:x)c.resize(rate*4);unsigned seed=43;
 for(int i=0;i<rate*4;++i){double t=double(i)/rate;seed=1664525*seed+1013904223;
  float noise=(float(seed>>8)/8388608.f-1)*.035f;
  float mid=.12f*std::sin(6.283185307*83*t)+.08f*std::sin(6.283185307*713*t)+.04f*std::sin(6.283185307*4103*t)+noise;
  float side=.07f*std::sin(6.283185307*1307*t)+noise;
  if(kind==0)side=0; // Mono delivered on the supported two-channel bus.
  if(kind==1)side*=.1f;
  if(kind==3){mid=0;side=.15f*std::sin(6.283185307*45*t);}
  x[0][i]=mid+side;x[1][i]=mid-side;
 }return x;
}
Audio render(const Audio& in,int rate,masterlab::StereoSettings c,int block,masterlab::StereoReading* reading=nullptr){
 auto out=in;masterlab::StereoImager dsp;dsp.settings(c);dsp.prepare(rate);
 for(int i=0;i<int(out[0].size());i+=block)dsp.process(out[0].data()+i,out[1].data()+i,std::min(block,int(out[0].size())-i));
 if(reading)*reading=dsp.read();return out;
}
struct Metrics {double width=0,corr=0,peak=0,lufs=0;};
Metrics measure(const Audio& x,int rate){
 double m=0,s=0,ll=0,rr=0,lr=0;Metrics out;masterlab::LoudnessMeter meter;meter.prepare(rate);
 for(size_t i=0;i<x[0].size();++i){double l=x[0][i],r=x[1][i];meter.push(float(l),float(r));if(i<size_t(rate))continue;m+=(l+r)*(l+r);s+=(l-r)*(l-r);ll+=l*l;rr+=r*r;lr+=l*r;out.peak=std::max({out.peak,std::abs(l),std::abs(r)});}
 out.width=std::sqrt(s/std::max(1e-30,m));out.corr=lr/std::max(1e-30,std::sqrt(ll*rr));out.lufs=meter.read().integrated;return out;
}
double matchedDelta(const Audio& dry,const Audio& wet,int rate){
 auto a=measure(dry,rate),b=measure(wet,rate);double g=std::pow(10.,(a.lufs-b.lufs)/20),d=0,e=0;
 for(size_t i=rate;i<dry[0].size();++i)for(int c=0;c<2;++c){double v=wet[c][i]*g-dry[c][i];d+=v*v;e+=dry[c][i]*dry[c][i];}return std::sqrt(d/std::max(1e-30,e));
}
void wav(const std::filesystem::path& path,const Audio& x,int rate,double gain=1){
 std::ofstream f(path,std::ios::binary);auto u16=[&](unsigned v){f.put(char(v));f.put(char(v>>8));};auto u32=[&](unsigned v){u16(v);u16(v>>16);};
 unsigned bytes=unsigned(x[0].size()*4);f.write("RIFF",4);u32(36+bytes);f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(rate);u32(rate*4);u16(4);u16(16);f.write("data",4);u32(bytes);
 for(size_t i=0;i<x[0].size();++i)for(int c=0;c<2;++c){check(std::abs(x[c][i]*gain)<1,"evidence would clip");u16(static_cast<unsigned>(static_cast<short>(std::lround(x[c][i]*gain*32767))));}check(bool(f),"WAV write failed");
}
int main(int argc,char** argv){
 std::filesystem::path dir;if(argc>1){dir=argv[1];std::filesystem::create_directories(dir);}
 std::ofstream bandCsv;if(!dir.empty()){bandCsv.open(dir/"bands.csv");bandCsv<<"rate,input,band,width,correlation\n";}
 std::cout<<"rate,input,legacy_width,new_width,correlation,LUFS_in,LUFS_out,peak_out,matched_delta_ratio\n";
 for(int rate:{44100,48000,96000})for(int kind=0;kind<4;++kind){
  auto dry=stimulus(rate,kind);masterlab::StereoSettings c;c.enabled=true;
  auto neutral=render(dry,rate,c,257);double nullError=0;for(size_t i=0;i<dry[0].size();++i)for(int ch=0;ch<2;++ch)nullError=std::max(nullError,double(std::abs(dry[ch][i]-neutral[ch][i])));check(nullError<1e-6,"neutral fails null");
  c.low=c.mid=c.high=c.air=2;auto legacy=render(dry,rate,c,257);
  if(kind==0)check(measure(legacy,rate).width==0,"legacy unexpectedly stereoizes mono");
  if(kind==1||kind==2)check(std::abs(measure(legacy,rate).width/measure(dry,rate).width-2)<.001,"legacy width not predictable");
  c={};c.enabled=true;c.stereoize=.7f;masterlab::StereoReading reading;auto wet=render(dry,rate,c,257,&reading),single=render(dry,rate,c,1),odd=render(dry,rate,c,4097);
  check(wet==single&&wet==odd,"Stereoize depends on host block size");
  for(size_t i=0;i<dry[0].size();++i)check(std::abs(wet[0][i]+wet[1][i]-dry[0][i]-dry[1][i])<1e-6,"Stereoize changes mono sum");
  auto a=measure(dry,rate),b=measure(wet,rate);double delta=matchedDelta(dry,wet,rate);
  if(kind<2)check(delta>.08&&b.width>a.width+.03,"Stereoize has insufficient matched-level effect");
  if(kind==3)check(wet==neutral,"Stereoize changes pure opposite-phase bass");
  std::cout<<rate<<','<<kind<<','<<measure(legacy,rate).width<<','<<b.width<<','<<b.corr<<','<<a.lufs<<','<<b.lufs<<','<<b.peak<<','<<delta<<'\n';
  for(int band=0;band<4;++band)check(std::isfinite(reading.width[band])&&std::isfinite(reading.correlation[band]),"invalid band telemetry");
  if(bandCsv)for(int band=0;band<4;++band)bandCsv<<rate<<','<<kind<<','<<band<<','<<reading.width[band]<<','<<reading.correlation[band]<<'\n';
  if(kind==0){check(reading.width[0]==0&&reading.width[1]>.05f,"per-band telemetry misses generated Side");}
  if(!dir.empty()&&rate==48000&&kind<3){wav(dir/("input-"+std::to_string(kind)+".wav"),dry,rate);wav(dir/("stereoize-matched-"+std::to_string(kind)+".wav"),wet,rate,std::pow(10.,(a.lufs-b.lufs)/20));}
  if(kind==0){
   auto d=c;d.delta=true;auto deltaAudio=render(dry,rate,d,257);Audio solos;for(auto& ch:solos)ch.assign(dry[0].size(),0);
   for(int band=0;band<4;++band){auto setting=c;setting.solo[band]=true;auto solo=render(dry,rate,setting,257);for(size_t i=0;i<dry[0].size();++i)for(int ch=0;ch<2;++ch)solos[ch][i]+=solo[ch][i];}
   for(size_t i=0;i<dry[0].size();++i)for(int ch=0;ch<2;++ch){check(std::abs(deltaAudio[ch][i]-(dry[ch][i]-wet[ch][i]))<1e-6,"Delta omits generated Side");check(std::abs(solos[ch][i]-wet[ch][i])<1e-6,"four solos fail reconstruction");}
  }
  // Amount=0, band bypass, Delta null and mono preview include the new path.
  for(int mode=0;mode<5;++mode){auto setting=c;if(mode==0)setting.amount=0;if(mode==1)setting.bandEnabled.fill(false);if(mode==2){setting.stereoize=0;setting.delta=true;}if(mode==3)setting.mono=true;if(mode==4)setting.enabled=false;
   auto out=render(dry,rate,setting,257);
   for(size_t i=rate;i<dry[0].size();++i)for(int ch=0;ch<2;++ch)check(std::abs(out[ch][i]-(mode==2?0:mode==3?(dry[0][i]+dry[1][i])*.5f:dry[ch][i]))<1e-6,"audition/bypass/amount failure");
  }
 }
 for(int rate:{44100,48000,96000}){
  masterlab::StereoImager dsp;masterlab::StereoSettings c;c.enabled=true;dsp.settings(c);dsp.prepare(rate);
  float previous=0;double jump=0;
  for(int i=0;i<rate;++i){if(i==rate/4)c.stereoize=1;if(i==rate/2)c.enabled=false;if(i==rate*3/4)c.enabled=true;dsp.settings(c);float l=.1f,r=.1f;dsp.process(&l,&r,1);if(i>100)jump=std::max(jump,double(std::abs(l-previous)));previous=l;check(std::isfinite(l)&&std::isfinite(r),"automation produced nonfinite sample");}
  check(jump<.001,"Stereoize/bypass automation clicks on DC");
 }
 // Bass protection is measured with mono LF, independently of existing Side.
 for(int rate:{44100,48000,96000}){Audio bass;for(auto& c:bass)c.resize(rate*2);for(int i=0;i<rate*2;++i)bass[0][i]=bass[1][i]=.2f*std::sin(6.283185307*45*i/rate);masterlab::StereoSettings c;c.enabled=true;c.stereoize=1;check(measure(render(bass,rate,c,127),rate).width<.01,"generated bass Side exceeds -40 dB");}
 // Side-only Haas and phase rotation must audibly decorrelate existing Side
 // while preserving Mid (the two-channel mono sum) at every sample.
 {
  constexpr int rate=48000;
  const auto dry=stimulus(rate,2);
  masterlab::StereoSettings c;c.enabled=true;c.delayMs=11.f;c.phaseRotate=.8f;c.stereoize=.2f;
  const auto shifted=render(dry,rate,c,257),single=render(dry,rate,c,1);
  check(shifted==single,"Haas and phase rotation depend on DAW block size");
  double changed=0,monoDry=0,monoWet=0;
  for(size_t i=rate;i<dry[0].size();++i){
   changed+=std::abs(shifted[0][i]-dry[0][i]);
   const double drySum=dry[0][i]+dry[1][i],wetSum=shifted[0][i]+shifted[1][i];
   monoDry+=drySum*drySum;monoWet+=wetSum*wetSum;
  }
  check(changed/(dry[0].size()-rate)>.01,"Haas/phase DSP has negligible effect");
  check(monoWet/monoDry>.25&&monoWet/monoDry<2.5,"channel Haas causes unsafe mono energy loss or gain");
  c.motionRate=.8f;c.motionDepth=.6f;
  const auto moving=render(dry,rate,c,257),movingSingle=render(dry,rate,c,1);
  check(moving==movingSingle,"Stereo Motion LFO depends on DAW buffer size");
  double motion=0;
  for(size_t i=rate;i<dry[0].size();++i){
   check(std::isfinite(moving[0][i])&&std::isfinite(moving[1][i]),
         "Stereo Motion produced a nonfinite sample");
   motion+=std::abs(moving[0][i]-shifted[0][i]);
  }
  check(motion/(dry[0].size()-rate)>.005,"Stereo Motion control has no actual action");
 }

 // Approved Haas/Motion contract: channel delay must create stereo from a
 // centered transient, and Motion must move the delayed side between L/R.
 {
  constexpr int rate=48000,count=rate*2;
  Audio impulses;for(auto& channel:impulses)channel.assign(count,0.f);
  for(int i=2400;i<count;i+=4800)impulses[0][i]=impulses[1][i]=.5f;
  masterlab::StereoSettings fixed;fixed.enabled=true;fixed.delayMs=10.f;
  const auto haas=render(impulses,rate,fixed,257);
  double sideEnergy=0.;for(int i=0;i<count;++i){const double side=haas[0][i]-haas[1][i];sideEnergy+=side*side;}
  check(sideEnergy>.1,"Haas does not delay one real channel on centered transients");

  auto moving=fixed;moving.motionRate=2.f;moving.motionDepth=1.f;
  const auto pingPong=render(impulses,rate,moving,257);
  bool leftDelayed=false,rightDelayed=false;
  for(int impulse=2400;impulse<count;impulse+=4800){
   double earlyL=0,earlyR=0,lateL=0,lateR=0;
   for(int i=impulse;i<impulse+240;++i){earlyL+=std::abs(pingPong[0][i]);earlyR+=std::abs(pingPong[1][i]);}
   for(int i=impulse+240;i<impulse+720;++i){lateL+=std::abs(pingPong[0][i]);lateR+=std::abs(pingPong[1][i]);}
   leftDelayed|=lateL>lateR*1.2&&earlyR>earlyL*1.2;
   rightDelayed|=lateR>lateL*1.2&&earlyL>earlyR*1.2;
  }
  check(leftDelayed&&rightDelayed,"Stereo Motion is not a channel-delay ping-pong");
 }

}
