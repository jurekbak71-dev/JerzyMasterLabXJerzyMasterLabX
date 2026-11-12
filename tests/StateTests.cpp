#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/FinalTruePeak.h"
#include "../Source/AnalogExciter.h"
#include "../Source/ModuleParameters.h"
#include <iostream>
#include <memory>
#include <functional>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(){
 std::unique_ptr<juce::AudioProcessor> source(createPluginFilter()),target(createPluginFilter());
 for(const char* id:{"truePeakDensity","truePeakCompressorMix","truePeakStereoLink","truePeakDelta"}){
  bool found=false;for(auto* base:source->getParameters())if(auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(base);parameter&&parameter->paramID==id){found=true;break;}
  if(!found){std::cerr<<"missing maximizer host parameter "<<id;return 1;}
 }
 for(int i=0;i<source->getParameters().size();++i){auto* parameter=source->getParameters()[i];float value=static_cast<float>((i*17)%101)/100.f;parameter->setValueNotifyingHost(parameter->isBoolean()?(value>=.5f?1.f:0.f):value);}
 juce::MemoryBlock saved;source->getStateInformation(saved);
 auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),static_cast<int>(saved.getSize()));
 if(!xml)return 1;
 target->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
 for(int i=0;i<source->getParameters().size();++i)if(source->getParameters()[i]->getValue()!=target->getParameters()[i]->getValue()){std::cerr<<"full state mismatch: "<<i;return 1;}
 const auto original=juce::ValueTree::fromXml(*xml);
 // Old FL Studio projects used schema 24 with exactly 304 parameters.
 // New host parameters must migrate to their factory defaults, while the
 // original parameter values and their IDs are preserved byte-for-byte.
 for(int legacyCount:{304,316,318,326,330}){
  auto legacy=original.createCopy();
  if(legacy.getNumChildren()!=static_cast<int>(masterlab::parameterCount)){std::cerr<<"unexpected expanded state count";return 1;}
  // APVTS stores PARAM children in a map/key order, not necessarily in
  // parameter registration order. Remove the twelve new *IDs*, not the
  // last twelve ValueTree children, or this fixture is not a legacy preset.
  for(int i=legacyCount;i<source->getParameters().size();++i){
   auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(source->getParameters()[i]);
   if(parameter==nullptr){std::cerr<<"missing ranged parameter";return 1;}
   const auto child=legacy.getChildWithProperty("id",parameter->paramID);
   if(!child.isValid()){std::cerr<<"missing appended ID "<<parameter->paramID;return 1;}
   legacy.removeChild(child,nullptr);
  }
  if(legacy.getNumChildren()!=legacyCount){std::cerr<<"legacy fixture count mismatch";return 1;}
  auto legacyXml=legacy.createXml();juce::MemoryBlock legacyBlock;
  juce::AudioProcessor::copyXmlToBinary(*legacyXml,legacyBlock);
  target->setStateInformation(legacyBlock.getData(),static_cast<int>(legacyBlock.getSize()));
  for(int i=0;i<source->getParameters().size();++i){
   const float expected=i<legacyCount?source->getParameters()[i]->getValue():
                            source->getParameters()[i]->getDefaultValue();
   if(std::abs(target->getParameters()[i]->getValue()-expected)>1.e-6f){
    std::cerr<<"legacy state migration mismatch "<<i<<" id="<<dynamic_cast<juce::RangedAudioParameter*>(target->getParameters()[i])->paramID<<" got="<<target->getParameters()[i]->getValue()<<" expected="<<expected<<" source="<<source->getParameters()[i]->getValue()<<" targetDefault="<<target->getParameters()[i]->getDefaultValue();return 1;
   }
  }
  target->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
 }
 auto unchanged=[&]{for(int i=0;i<source->getParameters().size();++i)if(source->getParameters()[i]->getValue()!=target->getParameters()[i]->getValue())return false;return true;};
 auto reject=[&](const std::function<void(juce::ValueTree&)>& corrupt){auto tree=original.createCopy();tree.getChildWithProperty("id","q16").setProperty("value",3.f,nullptr);corrupt(tree);auto altered=tree.createXml();juce::MemoryBlock block;juce::AudioProcessor::copyXmlToBinary(*altered,block);target->setStateInformation(block.getData(),static_cast<int>(block.getSize()));return unchanged();};
 int rejected=0;
 for(auto corrupt:std::vector<std::function<void(juce::ValueTree&)>>{
  [](auto& t){t.setProperty("schemaVersion",99,nullptr);},
  [](auto& t){t.removeProperty("schemaVersion",nullptr);},
  [](auto& t){t.setProperty("eqInstanceId","unknown",nullptr);},
  [](auto& t){t.setProperty("stereoInstanceId","unknown",nullptr);},
  [](auto& t){t.setProperty("stabilizerInstanceId","unknown",nullptr);},
  [](auto& t){t.removeProperty("dynamicsInstanceId",nullptr);},
  [](auto& t){t.removeChild(0,nullptr);},
  [](auto& t){auto n=t.getChild(0).createCopy();t.removeChild(1,nullptr);t.appendChild(n,nullptr);},
  [](auto& t){t.getChild(0).setProperty("id","unknown",nullptr);},
  [](auto& t){t.getChild(0).removeProperty("value",nullptr);},
  [](auto& t){t.getChild(0).appendChild(juce::ValueTree("unexpected"),nullptr);},
  [](auto& t){t.getChild(0).setProperty("value","nan",nullptr);},
  [](auto& t){t.getChild(0).setProperty("value","inf",nullptr);},
  [](auto& t){t.getChild(0).setProperty("value","1000junk",nullptr);},
  [](auto& t){t.getChild(0).setProperty("value",30000,nullptr);},
  [](auto& t){t.getChildWithProperty("id","eq").setProperty("value",.5,nullptr);},
  [](auto& t){t.getChild(1).setProperty("value",-1000,nullptr);},
  [](auto& t){t.getChildWithProperty("id","limiterDrive").setProperty("value",25,nullptr);},
  [](auto& t){t.getChildWithProperty("id","limiterRelease").setProperty("value",9,nullptr);},
  [](auto& t){t.getChildWithProperty("id","bandType16").setProperty("value",.5,nullptr);},
  [](auto& t){t.getChildWithProperty("id","bandType1").setProperty("value",6,nullptr);},
  [](auto& t){t.getChildWithProperty("id","deqRange3").setProperty("value",13,nullptr);},
  [](auto& t){t.getChildWithProperty("id","dynamicsMode").setProperty("value",.5,nullptr);},
  [](auto& t){t.getChildWithProperty("id","stereoPresent").setProperty("value",.5,nullptr);},
  [](auto& t){t.getChildWithProperty("id","chainOrder").setProperty("value",.5,nullptr);},
  [](auto& t){t.getChildWithProperty("id","chainOrder").setProperty("value",720,nullptr);}
 }){if(!reject(corrupt)){std::cerr<<"partial application of invalid state at corruption case "<<rejected;return 1;}++rejected;}
 target->setStateInformation(nullptr,0);const char garbage[]="not a preset";target->setStateInformation(garbage,sizeof garbage);
 target->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()/2));
 juce::MemoryBlock oversized(1024*1024+1,true);target->setStateInformation(oversized.getData(),static_cast<int>(oversized.getSize()));
 if(!unchanged()){std::cerr<<"invalid binary changed settings";return 1;}
 juce::MemoryBlock after;target->getStateInformation(after);auto afterXml=juce::AudioProcessor::getXmlFromBinary(after.getData(),static_cast<int>(after.getSize()));
 if(!afterXml||afterXml->getStringAttribute("schemaVersion")!="24"){std::cerr<<"schema lost";return 1;}
 // Removed STEMS parameters must not be exposed by the current VST3.
 for(auto* parameter:target->getParameters()){
  auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(parameter);
  if(!ranged)return 1;
  if(ranged->paramID.startsWithIgnoreCase("stem")){std::cerr<<"STEMS parameter retained";return 1;}
 }
 juce::MidiBuffer midi;
 bool restoredInsert=false;for(auto* base:source->getParameters())if(auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(base);parameter&&parameter->paramID=="workMode")restoredInsert=parameter->convertFrom0to1(parameter->getValue())>.5f;
 for(double rate:{44100.,48000.,96000.,192000.}){
  source->setNonRealtime(false);target->setNonRealtime(true);
  for(auto* processor:{source.get(),target.get()}){processor->setRateAndBufferSizeDetails(rate,32);processor->prepareToPlay(rate,32);}
  const int expectedLatency=(restoredInsert?0:static_cast<int>(std::ceil(rate*.005)))+masterlab::AnalogExciter::latencySamples()+masterlab::FinalTruePeak::latencySamples;
  if(source->getLatencySamples()!=expectedLatency||source->getLatencySamples()!=target->getLatencySamples()){std::cerr<<"restored PDC mismatch";return 1;}
  for(int size:{1,257,4097}){
   juce::AudioBuffer<float> live(2,size),offline(2,size);
   for(int i=0;i<size;++i){live.setSample(0,i,.02f*std::sin(i*.17f));live.setSample(1,i,.017f*std::cos(i*.11f));}offline.makeCopyOf(live);
   source->processBlock(live,midi);target->processBlock(offline,midi);
   for(int c=0;c<2;++c)for(int i=0;i<size;++i)if(!std::isfinite(live.getSample(c,i))||live.getSample(c,i)!=offline.getSample(c,i)){std::cerr<<"offline restored render differs";return 1;}
  }
  source->releaseResources();target->releaseResources();
 }
 if(!unchanged()){std::cerr<<"offline rendering changed settings";return 1;}
 std::cout<<"Passed: restored realtime/offline renders match at four sample rates and variable block sizes; settings preserved\n";
 std::cout<<"Passed: all parameters roundtrip, "<<rejected<<" invalid trees rejected, invalid/truncated binary leaves settings unchanged\n";
}
