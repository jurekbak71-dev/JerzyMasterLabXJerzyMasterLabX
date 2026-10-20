#include "../../Source/Processor.h"
#include <chrono>
#include <thread>
#include <iostream>

// Control-thread bridge to the real Processor/MasterBrain implementation.
// Does not substitute or strengthen any recommended parameter.
int main(int argc,char** argv){
 try{
  if(argc!=3)throw std::runtime_error("Usage: brain_demo input.wav output-directory");
  const juce::File input=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
  const juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
  if(!output.createDirectory())throw std::runtime_error("Cannot create output directory");
  juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(input));
  if(!reader||reader->sampleRate!=48000||reader->numChannels!=2||reader->lengthInSamples<48000*16)throw std::runtime_error("Expected stereo 48 kHz WAV, at least 16 seconds");
  juce::AudioBuffer<float> audio(2,static_cast<int>(reader->lengthInSamples));if(!reader->read(&audio,0,audio.getNumSamples(),0,true,true))throw std::runtime_error("Cannot read input");
  Processor p;p.prepareToPlay(48000,256);std::vector<float> baseline;for(auto* parameter:p.getParameters())baseline.push_back(parameter->getValue());
  masterlab::BrainConfig config;config.genre=0;config.style=0;config.seconds=16;config.targetLufs=-16;config.ceiling=-1;config.intensity=1.5f;config.automaticModules=true;
  p.startBrainUI(config);
  auto waitFor=[&](masterlab::BrainPhase phase){for(int i=0;i<10000;++i){auto snapshot=p.brain.readUI();if(snapshot.phase==phase)return snapshot;if(snapshot.phase==masterlab::BrainPhase::Failed)throw std::runtime_error(snapshot.message);std::this_thread::sleep_for(std::chrono::milliseconds(2));}throw std::runtime_error("Brain timeout");};
  waitFor(masterlab::BrainPhase::Analysing);juce::MidiBuffer midi;
  for(int offset=0;offset<48000*16;){int n=std::min(256,48000*16-offset);juce::AudioBuffer<float> block(2,n);for(int channel=0;channel<2;++channel)block.copyFrom(channel,0,audio,channel,offset,n);p.processBlock(block,midi);offset+=n;
   // Pace the offline producer so the realtime capture queue cannot overrun.
   for(;;){auto state=p.brain.readUI();if(state.phase==masterlab::BrainPhase::Failed)throw std::runtime_error(state.message);if(state.phase==masterlab::BrainPhase::Ready||offset-state.progress*(48000*16)<24000)break;std::this_thread::sleep_for(std::chrono::milliseconds(2));}
  }
  const auto snapshot=waitFor(masterlab::BrainPhase::Ready);if(!snapshot.proposal.valid)throw std::runtime_error("No valid Brain proposal");
  for(int i=0;i<p.getParameters().size();++i)if(p.getParameters()[i]->getValue()!=baseline[static_cast<size_t>(i)])throw std::runtime_error("Analysis changed parameters before Apply");
  if(!p.applyBrainUI())throw std::runtime_error("Brain Apply failed");
  auto root=std::make_unique<juce::DynamicObject>();root->setProperty("implementation","Actual Processor::startBrainUI / MasterBrain worker / applyBrainUI; no proposal edits");root->setProperty("analysis_seconds",snapshot.features.seconds);root->setProperty("input_lufs",snapshot.features.integrated);root->setProperty("input_peak",snapshot.features.peak);root->setProperty("input_rms",snapshot.features.rms);root->setProperty("input_correlation",snapshot.features.correlation);root->setProperty("genre","Rock");root->setProperty("style","Transparent");root->setProperty("intensity",1.5);root->setProperty("target_lufs",-16);
  auto array=[](const auto& values){juce::Array<juce::var> result;for(auto value:values)result.add(value);return juce::var(result);};
  root->setProperty("band_correlation",array(snapshot.features.bandCorrelation));root->setProperty("suggested_eq_gains",array(snapshot.proposal.gains));
  juce::Array<juce::var> reasons,warnings,parameters;
  for(const auto& s:snapshot.proposal.reasons)reasons.add(juce::String(s));for(const auto& s:snapshot.proposal.warnings)warnings.add(juce::String(s));root->setProperty("reasons",reasons);root->setProperty("warnings",warnings);
  for(auto* base:p.getParameters()){auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(base);if(!parameter)continue;auto item=std::make_unique<juce::DynamicObject>();item->setProperty("id",parameter->paramID);item->setProperty("name",parameter->getName(256));item->setProperty("normalized",parameter->getValue());item->setProperty("raw",parameter->convertFrom0to1(parameter->getValue()));parameters.add(juce::var(item.release()));}root->setProperty("parameters",parameters);
  if(!output.getChildFile("brain-proposal.json").replaceWithText(juce::JSON::toString(juce::var(root.release()),true)))throw std::runtime_error("Cannot save JSON");
  if(!p.savePreset(output.getChildFile("BRAIN.jmlx")))throw std::runtime_error("Cannot save Brain preset");
  p.releaseResources();std::cout<<"PASS: actual Brain captured 16 s, proposed settings without changes, Apply succeeded and preset exported\n";return 0;
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
