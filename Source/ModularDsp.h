#pragma once
#include "Dsp.h"
namespace masterlab {
inline float dbGain(float db)noexcept{return std::pow(10.f,db*.05f);}
inline float levelDb(float level)noexcept{return 20.f*std::log10(std::max(1e-9f,level));}
struct Slew {
 float value=0,step=.001f;
 void prepare(double rate,float initial=0){step=static_cast<float>(1-std::exp(-1/(rate*.02)));value=initial;}
 float tick(float target)noexcept {value+=step*(target-value);return value;}
};
struct Envelope {
 float value=0;
 float tick(float sample,double rate,float attackMs,float releaseMs)noexcept {
  const float time=sample>value?attackMs:releaseMs;
  const float coefficient=static_cast<float>(std::exp(-1/(rate*std::max(.0001f,time*.001f))));
  value=coefficient*value+(1-coefficient)*sample;return value;
 }
};
// Cascaded complementary one-pole splits. The four bands sum exactly to input;
// the same stable split is applied to Mid and Side, preserving mono summation.
struct Split {
 float low=0,highLow=0,topLow=0;
 std::array<float,4> tick(float x,float a,float b,float c)noexcept {low+=a*(x-low);highLow+=b*(x-highLow);topLow+=c*(x-topLow);return {low,highLow-low,topLow-highLow,x-topLow};}
};
struct StereoSettings {bool enabled=false,monoBass=false,mono=false;float low=1,mid=1,high=1,lowSplit=140,highSplit=2500,balance=0,air=1,topSplit=8000;std::array<bool,4> bandEnabled{true,true,true,true},solo{};float amount=1;bool delta=false;float stereoize=0,delayMs=0,phaseRotate=0,motionRate=1,motionDepth=0;};
struct StereoReading {std::array<float,4> correlation{},width{};};
class StereoImager {
 double rate=48000;Split splitMid,splitSide,splitGenerated;
 // Two high-pass poles protect bass; first-order all-pass rotates generated Side.
 // Added Side cancels algebraically in mono; the dry Mid is never delayed.
 float bassLow1=0,bassLow2=0,allpassState=0,hpCoefficient=0,apCoefficient=0;
 std::vector<float> sideDelay;size_t sideWrite=0;float phaseMemory=0,motionPhase=0;
 Slew stereoize,delayAmount,phaseAmount,motionRateSlew,motionDepthSlew;
 Slew mix,low,mid,high,air,splitA,splitB,splitC,balance,mono,amount,delta;std::array<Slew,4> bandBlend{},audition{};
 std::array<double,4> ll{},rr{},lr{};StereoSettings config;
public:
 void prepare(double sr){rate=std::isfinite(sr)&&sr>=8000&&sr<=384000?sr:48000;hpCoefficient=static_cast<float>(1-std::exp(-6.283185307179586*300/rate));sideDelay.assign(static_cast<size_t>(rate*.031)+4,0.f);const float t=static_cast<float>(std::tan(3.141592653589793*700/rate));apCoefficient=(t-1)/(t+1);reset();}
 void reset()noexcept {splitMid={};splitSide={};splitGenerated={};bassLow1=bassLow2=allpassState=phaseMemory=motionPhase=0;sideWrite=0;std::fill(sideDelay.begin(),sideDelay.end(),0.f);stereoize.prepare(rate,config.stereoize);delayAmount.prepare(rate,config.delayMs);phaseAmount.prepare(rate,config.phaseRotate);motionRateSlew.prepare(rate,config.motionRate);motionDepthSlew.prepare(rate,config.motionDepth);ll={};rr={};lr={};mix.prepare(rate,config.enabled?1.f:0.f);low.prepare(rate,config.monoBass?0.f:config.low);mid.prepare(rate,config.mid);high.prepare(rate,config.high);air.prepare(rate,config.air);splitC.prepare(rate,config.topSplit);splitA.prepare(rate,config.lowSplit);splitB.prepare(rate,config.highSplit);balance.prepare(rate,config.balance);mono.prepare(rate,config.mono?1.f:0.f);amount.prepare(rate,config.amount);delta.prepare(rate,config.delta?1.f:0.f);const bool soloing=std::any_of(config.solo.begin(),config.solo.end(),[](bool s){return s;});for(size_t i=0;i<4;++i){bandBlend[i].prepare(rate,config.bandEnabled[i]?1.f:0.f);audition[i].prepare(rate,!soloing||config.solo[i]?1.f:0.f);}}
 void settings(StereoSettings c)noexcept {config=c;}
 void process(float* left,float* right,int n)noexcept {
  if(!config.enabled&&mix.value<1e-6f){mix.value=0;ll={};rr={};lr={};return;}
  const double meter=std::exp(-1/(rate*.4));
  const bool soloing=std::any_of(config.solo.begin(),config.solo.end(),[](bool s){return s;});
  for(int i=0;i<n;++i){const float l=clean(left[i]),r=clean(right[i]),m=(l+r)*.5f,s=(l-r)*.5f;
   const float lo=std::clamp(splitA.tick(config.lowSplit),40.f,500.f),hi=std::clamp(splitB.tick(config.highSplit),lo+100,static_cast<float>(rate*.3));
   const float a=static_cast<float>(1-std::exp(-6.283185307179586*lo/rate)),b=static_cast<float>(1-std::exp(-6.283185307179586*hi/rate));
   const float top=std::clamp(splitC.tick(config.topSplit),hi+100,static_cast<float>(rate*.45)),c=static_cast<float>(1-std::exp(-6.283185307179586*top/rate));
   auto mids=splitMid.tick(m,a,b,c),sides=splitSide.tick(s,a,b,c);
   bassLow1+=hpCoefficient*(m-bassLow1);const float hp1=m-bassLow1;
   bassLow2+=hpCoefficient*(hp1-bassLow2);const float hp2=hp1-bassLow2;
   const float rotated=apCoefficient*hp2+allpassState;allpassState=hp2-apCoefficient*rotated;
   const auto generated=splitGenerated.tick(rotated,a,b,c);
   const float creation=.5f*stereoize.tick(std::clamp(clean(config.stereoize),0.f,1.f));
   const std::array<float,4> widths{low.tick(config.monoBass?0.f:config.low),mid.tick(config.mid),high.tick(config.high),air.tick(config.air)};
   const float monoMix=mono.tick(config.mono?1.f:0.f);float outSide=0,outMid=0;const float intensity=amount.tick(config.amount);
   for(size_t band=0;band<4;++band){const float width=1+intensity*bandBlend[band].tick(config.bandEnabled[band]?1.f:0.f)*(widths[band]-1),listen=audition[band].tick(!soloing||config.solo[band]?1.f:0.f);float side=(sides[band]*width+(band==0?0.f:generated[band]*creation*intensity*bandBlend[band].value*width))*(1-monoMix);outSide+=side*listen;outMid+=mids[band]*listen;
    const double bl=mids[band]+side,br=mids[band]-side;ll[band]=meter*ll[band]+(1-meter)*bl*bl;rr[band]=meter*rr[band]+(1-meter)*br*br;lr[band]=meter*lr[band]+(1-meter)*bl*br;}
   // Haas and all-pass processing affect only Side; dry Mid remains aligned.
   // This preserves L+R of a centered image, though stereo peaks can rise.
   if(!sideDelay.empty()){
    sideDelay[sideWrite]=outSide;
    const float ms=std::clamp(delayAmount.tick(config.delayMs),0.f,30.f);
    const float sampleOffset=ms*.001f*static_cast<float>(rate);
    const float read=static_cast<float>(sideWrite)+static_cast<float>(sideDelay.size())-sampleOffset;
    const int i0=static_cast<int>(std::floor(read))%static_cast<int>(sideDelay.size());
    const int i1=(i0+1)%static_cast<int>(sideDelay.size());
    const float fraction=read-std::floor(read);
    const float delayed=sideDelay[static_cast<size_t>(i0)]*(1.f-fraction)+sideDelay[static_cast<size_t>(i1)]*fraction;
    outSide+=(delayed-outSide)*std::clamp(ms/3.f,0.f,1.f);
    sideWrite=(sideWrite+1)%sideDelay.size();
   }
   const float phaseSignal=apCoefficient*outSide+phaseMemory;
   phaseMemory=outSide-apCoefficient*phaseSignal;
   outSide+=phaseAmount.tick(config.phaseRotate)*(phaseSignal-outSide);
   motionPhase+=6.283185307179586f*std::clamp(motionRateSlew.tick(config.motionRate),.05f,8.f)/static_cast<float>(rate);
   if(motionPhase>6.283185307179586f)motionPhase-=6.283185307179586f;
   const float motion=std::clamp(motionDepthSlew.tick(config.motionDepth),0.f,1.f)*.75f*std::sin(motionPhase);
   const float pan=std::clamp(balance.tick(config.balance)+motion,-1.f,1.f),blend=mix.tick(config.enabled?1.f:0.f);float wetL=(outMid+outSide)*(1-std::max(0.f,pan)),wetR=(outMid-outSide)*(1+std::min(0.f,pan));const float monoSignal=(wetL+wetR)*.5f;wetL+=monoMix*(monoSignal-wetL);wetR+=monoMix*(monoSignal-wetR);
   const float difference=delta.tick(config.delta?1.f:0.f);wetL+=difference*(l-2*wetL);wetR+=difference*(r-2*wetR);left[i]=clean(l+blend*(wetL-l));right[i]=clean(r+blend*(wetR-r));
  }
 }
 StereoReading read()const noexcept {StereoReading out;for(size_t i=0;i<4;++i){out.correlation[i]=ll[i]*rr[i]>1e-24?static_cast<float>(std::clamp(lr[i]/std::sqrt(ll[i]*rr[i]),-1.,1.)):0;out.width[i]=static_cast<float>(std::sqrt(std::max(0.,ll[i]+rr[i]-2*lr[i])/std::max(1e-20,ll[i]+rr[i]+2*lr[i])));}return out;}
};
struct DynamicsSettings {bool enabled=false,analog=false;float threshold=-18,ratio=2,attack=15,release=150,knee=6,makeup=0,mix=1,upward=0,upwardThreshold=-36;int intensity=0;float punch=0;bool delta=false;};
struct DynamicsReading {float reduction=0,upward=0,inputDb=-180,punch=0,measuredInputDb=-180,measuredOutputDb=-180;};
class SmartCompressor {
 double rate=48000;Envelope detector,transientDetector;Slew bypass,gain,mode,wetMix,delta;DynamicsSettings config;DynamicsReading reading;
public:
 static float reduction(float db,float threshold,float ratio,float knee)noexcept {
  const float x=db-threshold,k=std::max(0.f,knee),slope=1-1/std::max(1.f,ratio);
  if(k>.001f&&std::abs(x)<k*.5f)return slope*(x+k*.5f)*(x+k*.5f)/(2*k);
  return x>=k*.5f?slope*x:0.f;
 }
 static float upward(float db,float threshold,float amount)noexcept {
  // Bounded upward compression, faded out below -90 dBFS to avoid lifting silence.
  return std::min(std::max(0.f,amount),std::max(0.f,(threshold-db)*.5f))*std::clamp((db+90)/20,0.f,1.f);
 }
 void settings(DynamicsSettings c)noexcept{config=c;}
 void prepare(double sr){rate=sr;reset();}
 void reset()noexcept{detector={};transientDetector={};reading={};bypass.prepare(rate,config.enabled?1.f:0.f);gain.prepare(rate,0);mode.prepare(rate,config.analog?1.f:0.f);wetMix.prepare(rate,config.mix);delta.prepare(rate,config.delta?1.f:0.f);}
 void process(float* left,float* right,int n)noexcept {
  if(!config.enabled&&bypass.value<1e-6f){bypass.value=0;reading={};return;}
  const float coefficient=static_cast<float>(1-std::exp(-1/(rate*.001)));
  double inputEnergy=0.,outputEnergy=0.;
  for(int i=0;i<n;++i){float l=clean(left[i]),r=clean(right[i]);const float analog=mode.tick(config.analog?1.f:0.f);
   const float amplitude=std::max(std::abs(l),std::abs(r));const float envelope=detector.tick(amplitude,rate,config.attack*(1+.5f*analog),config.release*(1+.5f*analog));
   const float db=levelDb(envelope),gr=reduction(db,config.threshold,config.ratio,config.knee+analog*3),up=upward(db,config.upwardThreshold,config.upward);const int intensity=std::clamp(config.intensity,0,3);
   const float grScale=std::array<float,4>{1.f,.8f,1.4f,1.9f}[intensity],upScale=std::array<float,4>{1.f,.85f,1.2f,1.45f}[intensity];const float transient=transientDetector.tick(amplitude,rate,.1f,std::max(10.f,config.attack*.25f));const float punchGain=std::clamp(levelDb(transient)-db,0.f,12.f)*std::clamp(config.punch,0.f,100.f)*.01f*std::array<float,4>{1.f,.7f,1.f,1.25f}[intensity];
   const float target=std::clamp(config.makeup+up*upScale-gr*grScale+punchGain,-60.f,30.f);gain.value+=coefficient*(target-gain.value);
   const float multiplier=dbGain(gain.value),blend=bypass.tick(config.enabled?1.f:0.f)*wetMix.tick(config.mix);
   auto colour=[&](float x){const float v=x*multiplier;if(analog<1e-6f)return v; // Paired biased arctangents: odd, DC-neutral colour.
    const float positive=std::atan(1.15f*v+.08f),negative=std::atan(-1.15f*v+.08f);
    const float shaped=(positive-negative)/(2.f*(1.15f/(1+.08f*.08f)));
    return v+analog*.15f*(shaped-v);};
   const float wetL=colour(l),wetR=colour(r),normalL=clean(l+blend*(wetL-l)),normalR=clean(r+blend*(wetR-r)),monitor=delta.tick(config.delta?1.f:0.f);left[i]=clean(normalL+monitor*(blend*(wetL-l)-normalL));right[i]=clean(normalR+monitor*(blend*(wetR-r)-normalR));
   inputEnergy+=.5*(static_cast<double>(l)*l+static_cast<double>(r)*r);
   outputEnergy+=.5*(static_cast<double>(normalL)*normalL+static_cast<double>(normalR)*normalR);
   reading={gr*grScale*blend,up*upScale*blend,db,punchGain*blend};
  }
  if(n>0){reading.measuredInputDb=levelDb(static_cast<float>(std::sqrt(inputEnergy/n)));
           reading.measuredOutputDb=levelDb(static_cast<float>(std::sqrt(outputEnergy/n)));}
 }
 DynamicsReading read()const noexcept{return reading;}
};
inline constexpr size_t dynamicEqBandCount=8;
inline constexpr int dynamicEqControlPeriod=32;
struct DynamicBandSettings {float frequency=1000,gain=0,q=1,threshold=-24,range=-3,attack=15,release=150;bool enabled=true;};
struct DynamicEqSettings {
 bool enabled=false,delta=false;
 std::array<DynamicBandSettings,dynamicEqBandCount> bands{{
   {150,0,1,-24,-3,25,180,true},{2500,0,1,-24,-3,10,120,true},
   {7500,0,1,-30,-3,3,80,true},{100,0,1,-24,0,15,160,false},
   {400,0,1,-24,0,15,160,false},{1200,0,1,-24,0,15,160,false},
   {4500,0,1,-24,0,15,160,false},{11000,0,1,-24,0,15,160,false}
 }};
};
class DynamicEqualizer {
 double rate=48000;DynamicEqSettings config;Slew bypass,delta;
 std::array<std::array<Biquad,dynamicEqBandCount>,2> detectors{},filters{};
 std::array<Biquad,dynamicEqBandCount> detectorTargets{},filterTargets{};
 std::array<Envelope,dynamicEqBandCount> envelopes{};
 std::array<Slew,dynamicEqBandCount> blends{},gains{};
 std::array<float,dynamicEqBandCount> activity{};
 int controlPhase=0;
 static void smoothTo(Biquad& current,const Biquad& target,float alpha)noexcept {
  current.b0+=alpha*(target.b0-current.b0);
  current.b1+=alpha*(target.b1-current.b1);
  current.b2+=alpha*(target.b2-current.b2);
  current.a1+=alpha*(target.a1-current.a1);
  current.a2+=alpha*(target.a2-current.a2);
 }
 void refreshCoefficients()noexcept {
  for(size_t i=0;i<dynamicEqBandCount;++i){
   const auto& band=config.bands[i];
   // Fully bypassed bands neither design coefficients nor process filters.
   if(!band.enabled && blends[i].value<1.e-6f)continue;
   detectorTargets[i].configure(rate,band.frequency,0,band.q,FilterType::Notch);
   const float amount=std::clamp((levelDb(envelopes[i].value)-band.threshold)/12.f,0.f,1.f);
   filterTargets[i].configure(rate,band.frequency,
       std::clamp(band.gain+band.range*amount,-24.f,24.f),band.q,FilterType::Bell);
  }
 }
public:
 void settings(DynamicEqSettings c)noexcept {config=c;}
 void prepare(double sr){rate=std::isfinite(sr)&&sr>=8000&&sr<=384000?sr:48000;reset();}
 void reset()noexcept {
  detectors={};filters={};detectorTargets={};filterTargets={};envelopes={};activity={};controlPhase=0;
  bypass.prepare(rate,config.enabled?1.f:0.f);
  delta.prepare(rate,config.delta?1.f:0.f);
  for(size_t i=0;i<dynamicEqBandCount;++i){
   blends[i].prepare(rate,config.bands[i].enabled?1.f:0.f);
   gains[i].prepare(rate,config.bands[i].gain);
  }
  refreshCoefficients();
  for(size_t i=0;i<dynamicEqBandCount;++i)
   for(int c=0;c<2;++c){detectors[c][i]=detectorTargets[i];filters[c][i]=filterTargets[i];}
 }
 void process(float* left,float* right,int n)noexcept {
  if(!config.enabled && bypass.value<1.e-6f){bypass.value=0;activity={};if(config.delta)for(int i=0;i<n;++i)left[i]=right[i]=0.f;return;}
  const float smooth=static_cast<float>(1-std::exp(-1/(rate*.005)));
  // Fixed 32-sample control period survives variable DAW buffer boundaries.
  for(int sample=0;sample<n;++sample){
   if(controlPhase==0)refreshCoefficients();
   controlPhase=(controlPhase+1)%dynamicEqControlPeriod;
   const float l=clean(left[sample]),r=clean(right[sample]);float wetL=l,wetR=r;
   for(size_t i=0;i<dynamicEqBandCount;++i){
    const auto& band=config.bands[i];
    if(!band.enabled && blends[i].value<1.e-6f){activity[i]=0;continue;}
    for(int c=0;c<2;++c)smoothTo(detectors[c][i],detectorTargets[i],smooth);
    const float dl=l-detectors[0][i].tick(l),dr=r-detectors[1][i].tick(r);
    const float detected=envelopes[i].tick(std::max(std::abs(dl),std::abs(dr)),rate,band.attack,band.release);
    const float amount=std::clamp((levelDb(detected)-band.threshold)/12.f,0.f,1.f);
    const float mix=blends[i].tick(band.enabled?1.f:0.f);
    activity[i]=band.range*amount*mix;
    gains[i].tick(band.gain+band.range*amount);
    for(int c=0;c<2;++c)smoothTo(filters[c][i],filterTargets[i],smooth);
    const float filteredL=filters[0][i].tick(wetL),filteredR=filters[1][i].tick(wetR);
    wetL+=mix*(filteredL-wetL);wetR+=mix*(filteredR-wetR);
   }
   const float mix=bypass.tick(config.enabled?1.f:0.f);
   const float normalL=clean(l+mix*(wetL-l)),normalR=clean(r+mix*(wetR-r));
   const float monitoring=delta.tick(config.delta?1.f:0.f);
   left[sample]=clean(normalL+monitoring*(l-normalL-normalL));
   right[sample]=clean(normalR+monitoring*(r-normalR-normalR));
  }
 }
 std::array<float,dynamicEqBandCount> read()const noexcept {
  auto out=activity;for(auto& x:out)x*=bypass.value;return out;
 }
 void coefficients(std::array<Biquad,dynamicEqBandCount>& out)const noexcept {out=filters[0];}
 std::array<float,dynamicEqBandCount> bandBlends()const noexcept {
  std::array<float,dynamicEqBandCount> out{};
  for(size_t i=0;i<dynamicEqBandCount;++i)out[i]=blends[i].value*bypass.value;
  return out;
 }
};

}
