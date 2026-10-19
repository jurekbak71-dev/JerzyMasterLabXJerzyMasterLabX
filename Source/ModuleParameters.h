#pragma once
#include <array>
#include <cstddef>
namespace masterlab {
inline constexpr size_t parameterCount=161;
inline constexpr std::array<const char*,75> moduleParameterIds{
 "eqPresent","limPresent","stereoPresent","stereoOn","stereoLowWidth","stereoMidWidth","stereoHighWidth","stereoLowSplit","stereoHighSplit","stereoMonoBass","stereoMono","stereoBalance",
 "dynamicsPresent","dynamicsOn","dynamicsMode","dynThreshold","dynRatio","dynAttack","dynRelease","dynKnee","dynMakeup","dynMix","upwardAmount","upwardThreshold",
 "dynamicEqPresent","dynamicEqOn",
 "deqFreq1","deqGain1","deqQ1","deqThreshold1","deqRange1","deqAttack1","deqRelease1","deqBandOn1",
 "deqFreq2","deqGain2","deqQ2","deqThreshold2","deqRange2","deqAttack2","deqRelease2","deqBandOn2",
 "deqFreq3","deqGain3","deqQ3","deqThreshold3","deqRange3","deqAttack3","deqRelease3","deqBandOn3","stereoAirWidth","stereoTopSplit","chainOrder","stereoBandOn1","stereoSolo1","stereoBandOn2","stereoSolo2","stereoBandOn3","stereoSolo3","stereoBandOn4","stereoSolo4","stereoAmount","stereoDelta","stabilizerPresent","stabilizerOn","stabilizerMode","stabilizerProfile","stabilizerAmount","stabilizerSpeed","stabilizerSmoothing","stabilizerLow","stabilizerMid","stabilizerHigh","stabilizerTame","stabilizerDelta"};
}
