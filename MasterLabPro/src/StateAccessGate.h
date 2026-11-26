#pragma once
#include <atomic>
#include <thread>
namespace ml {
// One audio consumer; control callers serialize externally. Audio never waits.
class StateAccessGate {
 std::atomic<unsigned> state{0};
public:
 static_assert(std::atomic<unsigned>::is_always_lock_free);
 bool tryEnterAudio() noexcept {unsigned idle=0;return state.compare_exchange_strong(idle,1,std::memory_order_acquire,std::memory_order_relaxed);}
 void leaveAudio() noexcept {state.fetch_and(~1u,std::memory_order_release);}
 void enterControl() noexcept {state.fetch_or(2u,std::memory_order_acq_rel);while(state.load(std::memory_order_acquire)&1u)std::this_thread::yield();}
 void leaveControl() noexcept {state.store(0,std::memory_order_release);}
 class ControlGuard {StateAccessGate& gate;public:explicit ControlGuard(StateAccessGate& g):gate(g){gate.enterControl();}~ControlGuard(){gate.leaveControl();}ControlGuard(const ControlGuard&)=delete;};
 class AudioGuard {StateAccessGate& gate;bool entered;public:explicit AudioGuard(StateAccessGate& g):gate(g),entered(g.tryEnterAudio()){}~AudioGuard(){if(entered)gate.leaveAudio();}explicit operator bool()const noexcept{return entered;}AudioGuard(const AudioGuard&)=delete;};
};
}
