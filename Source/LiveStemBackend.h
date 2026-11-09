#pragma once
#include <array>
namespace masterlab {
using LiveStemHop=std::array<std::array<float,128>,2>;
// Stable MasterLabX order: Drums, Bass, Other, Vocals; channel-major hop.
using SeparatedStemHop=std::array<LiveStemHop,4>;
class LiveStemBackend {
public:
 virtual ~LiveStemBackend()=default;
 // Worker only. A valid output describes the PREVIOUS 128-sample input hop.
 // First call after reset returns false (preroll); exceptions indicate failure.
 virtual bool process(const LiveStemHop&,SeparatedStemHop&)=0;
 virtual void reset()=0;
};
}
