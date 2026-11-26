#include "StateAccessGate.h"
#include <atomic>
#include <cassert>
#include <thread>
int main(){
 ml::StateAccessGate gate;
 {ml::StateAccessGate::ControlGuard c(gate);ml::StateAccessGate::AudioGuard a(gate);assert(!a);}
 {ml::StateAccessGate::AudioGuard a(gate);assert(a);}
 std::atomic<bool> stop{false},started{false};std::atomic<unsigned> errors{0};unsigned protectedValue=0;
 std::thread audio([&]{while(!stop.load()){ml::StateAccessGate::AudioGuard a(gate);if(a){const auto v=protectedValue;if(v!=protectedValue)++errors;started.store(true);}}});
 while(!started.load())std::this_thread::yield();
 for(unsigned i=0;i<5000;++i){ml::StateAccessGate::ControlGuard c(gate);protectedValue=i;ml::StateAccessGate::AudioGuard a(gate);assert(!a);}
 stop.store(true);audio.join();assert(errors.load()==0);
}
