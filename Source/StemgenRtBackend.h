#pragma once
#include "LiveStemBackend.h"
#include <filesystem>
#include <memory>
namespace masterlab {
// Noncommercial StemgenRT v0.7/0.8 graph adapter, not upstream plugin code.
// Construction, inference, reset and destruction are worker/control-thread only.
// Model native rate is 44100 Hz, hop128, output refers to previous input hop.
class StemgenRtBackend final:public LiveStemBackend {
public:
 explicit StemgenRtBackend(const std::filesystem::path& modelDirectory,bool threeWorkers=true);
 ~StemgenRtBackend() override;
 bool process(const LiveStemHop&,SeparatedStemHop&) override;
 void reset() override;
 StemgenRtBackend(const StemgenRtBackend&)=delete;
 StemgenRtBackend& operator=(const StemgenRtBackend&)=delete;
private:
 struct Impl;
 std::unique_ptr<Impl> impl_;
};
}
