#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <memory>
#include <functional>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(){
 std::unique_ptr<juce::AudioProcessor> source(createPluginFilter()),target(createPluginFilter());
 for(int i=0;i<source->getParameters().size();++i){auto* parameter=source->getParameters()[i];float value=static_cast<float>((i*17)%101)/100.f;parameter->setValueNotifyingHost(parameter->isBoolean()?(value>=.5f?1.f:0.f):value);}
 juce::MemoryBlock saved;source->getStateInformation(saved);
 auto xml=juce::AudioProcessor::getXmlFromBinary(saved.getData(),static_cast<int>(saved.getSize()));
 if(!xml)return 1;
 target->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()));
 for(int i=0;i<source->getParameters().size();++i)if(source->getParameters()[i]->getValue()!=target->getParameters()[i]->getValue()){std::cerr<<"full state mismatch: "<<i;return 1;}
 const auto original=juce::ValueTree::fromXml(*xml);
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
 }){if(!reject(corrupt)){std::cerr<<"partial application of invalid state";return 1;}++rejected;}
 target->setStateInformation(nullptr,0);const char garbage[]="not a preset";target->setStateInformation(garbage,sizeof garbage);
 target->setStateInformation(saved.getData(),static_cast<int>(saved.getSize()/2));
 juce::MemoryBlock oversized(1024*1024+1,true);target->setStateInformation(oversized.getData(),static_cast<int>(oversized.getSize()));
 if(!unchanged()){std::cerr<<"invalid binary changed settings";return 1;}
 juce::MemoryBlock after;target->getStateInformation(after);auto afterXml=juce::AudioProcessor::getXmlFromBinary(after.getData(),static_cast<int>(after.getSize()));
 if(!afterXml||afterXml->getStringAttribute("schemaVersion")!="6"){std::cerr<<"schema lost";return 1;}
 // Upgrade both previous versions; newly introduced parameters use factory defaults.
 for(int version:{1,2,3,4,5}){
  const int count=version==1?52:(version==2?54:(version==3?86:(version==4?138:161)));
  auto legacy=original.createCopy();legacy.setProperty("schemaVersion",version,nullptr);
  for(int i=count;i<source->getParameters().size();++i){auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(source->getParameters()[i]);legacy.removeChild(legacy.getChildWithProperty("id",parameter->paramID),nullptr);}
  std::unique_ptr<juce::AudioProcessor> migrated(createPluginFilter());
  for(int i=count;i<migrated->getParameters().size();++i){auto* parameter=migrated->getParameters()[i];parameter->setValueNotifyingHost(parameter->isDiscrete()?(parameter->getDefaultValue()>.5f?0.f:1.f):.4f);}
  juce::MemoryBlock oldState;juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(),oldState);
  migrated->setStateInformation(oldState.getData(),static_cast<int>(oldState.getSize()));
  for(int i=0;i<count;++i)if(migrated->getParameters()[i]->getValue()!=source->getParameters()[i]->getValue()){std::cerr<<"legacy parameter changed";return 1;}
  for(int i=count;i<migrated->getParameters().size();++i)if(migrated->getParameters()[i]->getValue()!=migrated->getParameters()[i]->getDefaultValue()){std::cerr<<"legacy defaults incorrect version "<<version<<" index "<<i<<" got "<<migrated->getParameters()[i]->getValue()<<" expected "<<migrated->getParameters()[i]->getDefaultValue();return 1;}
 }
 juce::MidiBuffer midi;
 for(double rate:{44100.,48000.,96000.,192000.}){
  source->setNonRealtime(false);target->setNonRealtime(true);
  for(auto* processor:{source.get(),target.get()}){processor->setRateAndBufferSizeDetails(rate,32);processor->prepareToPlay(rate,32);}
  if(source->getLatencySamples()!=static_cast<int>(std::ceil(rate*.005))||source->getLatencySamples()!=target->getLatencySamples()){std::cerr<<"restored PDC mismatch";return 1;}
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
