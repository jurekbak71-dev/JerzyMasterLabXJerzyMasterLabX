#include "../Source/Processor.h"
#include <iostream>
#include <cstdlib>
void check(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
std::vector<float> values(Processor& p){std::vector<float> result;for(auto* parameter:p.getParameters())result.push_back(parameter->getValue());return result;}
void set(Processor& p,const char* id,float value){auto* parameter=p.state.getParameter(id);parameter->setValueNotifyingHost(parameter->convertTo0to1(value));}
int main(){
 struct Temp {juce::File directory=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("MasterLabPresetTest-"+juce::Uuid{}.toString());~Temp(){directory.deleteRecursively();}} temp;
 check(temp.directory.createDirectory().wasOk(),"temporary directory failed");
 Processor source,target;set(source,"stereoizeAmount",.65f);set(source,"frequency16",3333);set(source,"gain16",6);set(source,"bandType16",2);set(source,"bandEnabled15",0);set(source,"limiterDrive",9);set(source,"limiterRelease",180);
 set(target,"limiterDrive",17);set(target,"gain",-7);target.state.copyState();target.undoManager.clearUndoHistory();
 auto before=values(target),expected=values(source);auto file=temp.directory.getChildFile("test.jmlx");
 check(source.savePreset(file)&&file.getSize()>0,"preset save failed");
 check(target.loadPreset(file)&&values(target)==expected,"file preset roundtrip failed");
 check(target.undo()&&values(target)==before,"preset load is not a single undo transaction");
 check(target.redo()&&values(target)==expected,"preset redo failed");
 auto invalid=temp.directory.getChildFile("invalid.jmlx");check(invalid.replaceWithText("not a preset"),"cannot write corrupt fixture");
 check(!target.loadPreset(invalid)&&values(target)==expected,"invalid preset changed parameters");
 check(!target.loadPreset(temp.directory.getChildFile("missing.jmlx"))&&values(target)==expected,"missing preset changed parameters");
 juce::MemoryBlock oversized(1024*1024+1,true);check(invalid.replaceWithData(oversized.getData(),oversized.getSize()),"cannot write oversized fixture");
 check(!target.loadPreset(invalid)&&values(target)==expected,"oversized preset changed parameters");
 // A manual two-parameter change is one operation, with redo invalidated by a subsequent edit.
 target.beginUiTransaction("Pair");set(target,"frequency16",1200);set(target,"gain16",-3);target.finishUiTransaction();
 check(target.undo()&&values(target)==expected,"two-parameter undo failed");
 target.beginUiTransaction("New edit");set(target,"gain16",-8);target.finishUiTransaction();check(!target.redo(),"new edit did not invalidate redo");
 juce::MemoryBlock host;source.getStateInformation(host);target.setStateInformation(host.getData(),static_cast<int>(host.getSize()));check(!target.undoManager.canUndo(),"host state restore left stale undo history");
 std::cout<<"Passed: preset file roundtrip, atomic rejection, missing/oversized files, grouped undo/redo, redo invalidation and host restore history reset\n";
}
