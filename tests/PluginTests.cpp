#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <memory>
extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(int argc,char**){
 std::unique_ptr<juce::ScopedJuceInitialiser_GUI> gui;if(argc>1)gui=std::make_unique<juce::ScopedJuceInitialiser_GUI>();
 std::unique_ptr<juce::AudioProcessor> a(createPluginFilter()),b(createPluginFilter());
 auto* gain=a->getParameters()[1];gain->setValueNotifyingHost(.72f);
 juce::MemoryBlock state;a->getStateInformation(state);b->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
 if(b->getParameters()[1]->getValue()!=gain->getValue()){std::cerr<<"state restore failed";return 1;}
 a->setRateAndBufferSizeDetails(48000,257);b->setRateAndBufferSizeDetails(48000,257);a->prepareToPlay(48000,257);b->prepareToPlay(48000,257);
 if(a->getLatencySamples()!=240){std::cerr<<"PDC mismatch";return 1;}
 juce::AudioBuffer<float> x(2,257),y(2,257);juce::MidiBuffer midi;
 for(int block=0;block<100;++block){for(int c=0;c<2;++c)for(int i=0;i<257;++i)x.setSample(c,i,.2f*std::sin((block*257+i)*.17f));y.makeCopyOf(x);a->processBlock(x,midi);b->processBlock(y,midi);
 for(int c=0;c<2;++c)for(int i=0;i<257;++i)if(!std::isfinite(x.getSample(c,i))||x.getSample(c,i)!=y.getSample(c,i)){std::cerr<<"restored render mismatch";return 1;}}
 if(argc>1){for(int iteration=0;iteration<3;++iteration){std::unique_ptr<juce::AudioProcessorEditor> editor(a->createEditor());editor->addToDesktop(0);editor->setVisible(true);juce::Thread::sleep(40);juce::Timer::callPendingTimersSynchronously();
 for(auto size:{juce::Point<int>(750,550),juce::Point<int>(1000,700),juce::Point<int>(2000,1400)}){editor->setSize(size.x,size.y);auto image=editor->createComponentSnapshot(editor->getLocalBounds());if(!image.isValid()||image.getWidth()!=size.x||image.getHeight()!=size.y){std::cerr<<"GUI snapshot failed";return 1;}if(iteration==0&&size.x==1000){auto output=juce::File::getCurrentWorkingDirectory().getChildFile("build-plugin/gui-smoke.png");output.getParentDirectory().createDirectory();juce::FileOutputStream stream(output);stream.setPosition(0);stream.truncate();juce::PNGImageFormat{}.writeImageToStream(image,stream);}
 auto background=image.getPixelAt(0,0);bool content=false;for(int y=0;y<image.getHeight();y+=10)for(int x=0;x<image.getWidth();x+=10)content|=image.getPixelAt(x,y)!=background;if(!content){std::cerr<<"blank GUI";return 1;}}
 editor->setVisible(false);editor->removeFromDesktop();}std::cout<<"Passed: editor reopen and nonblank rendering at three sizes\n";}
 a->releaseResources();b->releaseResources();std::cout<<"Passed: parameter state roundtrip, PDC, deterministic render without editor\n";
}
