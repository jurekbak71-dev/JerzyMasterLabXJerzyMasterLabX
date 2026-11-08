#pragma once
#include "LiveStemEngine.h"
#include "LiveStemBackend.h"
#include <atomic>
#include <memory>
#include <thread>
namespace masterlab {
struct LiveStemWorkerStatistics {
 std::uint64_t hops{},outputFrames{},failures{},discardedFrames{},totalInferenceNanoseconds{},maxInferenceNanoseconds{};
 std::uint64_t maxBetweenInferenceNanoseconds{},maxInputBacklogFrames{};
 std::uint64_t skippedExpiredHops{}; // worker avoided ONNX for results past playback deadline
};
// All lifecycle calls are control-thread-only. Engine must outlive this worker.
// Stop/join this object before preparing or destroying its engine.
class LiveStemWorker final {
public:
 LiveStemWorker(LiveStemEngine&,std::unique_ptr<LiveStemBackend>);
 ~LiveStemWorker();
 LiveStemWorkerStatistics statistics()const noexcept;
private:
 LiveStemEngine& engine_;
 std::unique_ptr<LiveStemBackend> backend_;
 std::atomic<std::uint64_t> hops_{0},outputFrames_{0},failures_{0},discardedFrames_{0},totalNs_{0},maxNs_{0};
 std::atomic<std::uint64_t> maxBetweenNs_{0},maxBacklog_{0},skippedExpiredHops_{0};
 std::jthread thread_;
 void run(std::stop_token);
};
}
