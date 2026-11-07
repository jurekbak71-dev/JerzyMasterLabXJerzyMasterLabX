#pragma once
// Offline, genuine AI-separated four-stem audio is loaded here; no frequency-filter
// approximation is presented as separation.  Immutable buffers are published to
// the audio callback atomically.  No file reads, inference or allocations there.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <string>
#include <algorithm>
namespace masterlab {
inline constexpr std::array<const char*,4> stemNames{"drums","bass","other","vocals"};
struct StemBank {
 std::array<juce::AudioBuffer<float>,4> audio;
 std::array<float,4> rms{},peak{};
 double sampleRate=44100.0;
 int samples=0;
 juce::String folder;
};
class StemRemix {
 std::atomic<std::shared_ptr<const StemBank>> current_;
 std::array<std::array<float,2>,4> lowpass_{};
 std::int64_t fallbackSamples_=0;
public:
 StemRemix()=default;
 std::shared_ptr<const StemBank> snapshot() const {return current_.load(std::memory_order_acquire);}
 void unload(){current_.store({},std::memory_order_release);reset();}
 void reset(){for(auto& pair:lowpass_)pair.fill(0.f);fallbackSamples_=0;}
 juce::String loadFolder(const juce::File& directory){
  if(!directory.isDirectory())return "Choose a folder containing four separated WAV files.";
  juce::AudioFormatManager formats;formats.registerBasicFormats();
  auto bank=std::make_shared<StemBank>();
  bank->folder=directory.getFullPathName();
  std::int64_t totalSamples=0;
  for(int stem=0;stem<4;++stem){
   juce::File file=directory.getChildFile(juce::String(stemNames[stem])+".wav");
   if(!file.existsAsFile()){
    const auto matches=directory.findChildFiles(juce::File::findFiles,false,
                            "*"+juce::String(stemNames[stem])+"*.wav");
    if(matches.isEmpty())return "Missing real "+juce::String(stemNames[stem])+" stem WAV.";
    file=matches[0];
   }
   std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
   if(!reader||reader->lengthInSamples<=0||reader->numChannels<1||reader->numChannels>2)
    return "Unreadable stem: "+file.getFileName();
   if(reader->lengthInSamples>reader->sampleRate*600.0)
    return "Track exceeds the 10-minute in-memory safety limit.";
   if(reader->lengthInSamples>std::numeric_limits<int>::max())
    return "Audio file too long.";
   if(stem==0){
    bank->sampleRate=reader->sampleRate;
    bank->samples=static_cast<int>(reader->lengthInSamples);
   }else if(std::abs(bank->sampleRate-reader->sampleRate)>.01 ||
            bank->samples!=static_cast<int>(reader->lengthInSamples))
    return "Stem timing/sample rates differ. Export aligned stems with the same duration.";
   totalSamples+=reader->lengthInSamples;
   if(totalSamples>600LL*192000*4LL || totalSamples*8LL>950LL*1024*1024)
    return "Stems exceed 950 MB RAM safety limit.";
   auto& out=bank->audio[static_cast<size_t>(stem)];
   out.setSize(2,bank->samples,false,true,true);
   if(!reader->read(&out,0,bank->samples,0,true,reader->numChannels==2))
    return "Failed to read "+file.getFileName();
   if(reader->numChannels==1)
    out.copyFrom(1,0,out,0,0,bank->samples);
   double energy=0.;
   for(int i=0;i<bank->samples;++i){
    const auto l=out.getSample(0,i),r=out.getSample(1,i);
    if(!std::isfinite(l)||!std::isfinite(r))return "Stem contains invalid audio values.";
    energy+=(double(l)*l+double(r)*r)*.5;
    bank->peak[static_cast<size_t>(stem)]=std::max(bank->peak[static_cast<size_t>(stem)],
                                                  std::max(std::abs(l),std::abs(r)));
   }
   bank->rms[static_cast<size_t>(stem)]=
       std::sqrt(static_cast<float>(energy/std::max(1,bank->samples)));
  }
  current_.store(bank,std::memory_order_release);
  reset();
  return {};
 }
 // Render existing stem files at the DAW position. Values: 0 input, 1 replace, 2 mix.
 // Current state is snapshotted once per block; no model or disk is accessed.
 void render(juce::AudioBuffer<float>& b,double hostSampleRate,
             std::int64_t hostPosition,bool playing,const std::array<float,21>& v) noexcept {
  const int mode=static_cast<int>(std::round(v[0]));
  auto bank=current_.load(std::memory_order_acquire);
  if(mode==0||!bank||bank->samples==0||hostSampleRate<=0)return;
  if(!playing){if(mode==1)b.clear();return;}
  const bool solo=v[5]>.5f||v[10]>.5f||v[15]>.5f||v[20]>.5f;
  const double ratio=bank->sampleRate/hostSampleRate;
  const double coeff=1.0-std::exp(-2.0*juce::MathConstants<double>::pi*
                                 1800.0/hostSampleRate);
  for(int i=0;i<b.getNumSamples();++i){
   const double at=static_cast<double>(hostPosition+i)*ratio;
   const auto index=static_cast<std::int64_t>(at);
   float left=0.f,right=0.f;
   if(index>=0&&index<bank->samples){
    const int j=static_cast<int>(index),k=std::min(j+1,bank->samples-1);
    const float alpha=static_cast<float>(at-index);
    for(int stem=0;stem<4;++stem){
     const int p=1+stem*5;
     if(v[p+3]>.5f||(solo&&v[p+4]<.5f))continue;
     const auto& audio=bank->audio[static_cast<size_t>(stem)];
     float l=juce::jmap(alpha,audio.getSample(0,j),audio.getSample(0,k));
     float r=juce::jmap(alpha,audio.getSample(1,j),audio.getSample(1,k));
     const float tone=v[p+1],gain=juce::Decibels::decibelsToGain(v[p]);
     for(int channel=0;channel<2;++channel){
      const float input=channel==0?l:r;
      auto& smooth=lowpass_[static_cast<size_t>(stem)][static_cast<size_t>(channel)];
      smooth+=static_cast<float>(coeff)*(input-smooth);
      const float shaped=(input+tone*(input-smooth))*gain;
      if(channel==0)l=shaped;else r=shaped;
     }
     const float mid=(l+r)*.5f,side=(l-r)*.5f*v[p+2];
     left+=mid+side;right+=mid-side;
    }
   }
   if(mode==1){b.setSample(0,i,left);b.setSample(1,i,right);}
   else{b.addSample(0,i,left);b.addSample(1,i,right);}
  }
  fallbackSamples_=hostPosition+b.getNumSamples();
 }
 std::int64_t fallbackPosition()const noexcept{return fallbackSamples_;}
};
} // namespace masterlab
