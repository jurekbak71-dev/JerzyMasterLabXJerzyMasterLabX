#include "Processor.h"
#include "Editor.h"

juce::AudioProcessorEditor* createMasterLabEditor(Processor& processor){return new Editor(processor);}
