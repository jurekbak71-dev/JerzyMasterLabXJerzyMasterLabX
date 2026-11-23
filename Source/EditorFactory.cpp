#include "Processor.h"
#include "Editor.h"

juce::AudioProcessorEditor* Processor::createEditor(){return new Editor(*this);}
