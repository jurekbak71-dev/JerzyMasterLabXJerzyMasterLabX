#include "Processor.h"

juce::AudioProcessorEditor* createMasterLabEditor(Processor&);
juce::AudioProcessorEditor* Processor::createEditor(){return createMasterLabEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new Processor;}
