#pragma once
#include <array>
#include <cstddef>
#include <string>
namespace masterlab {
inline constexpr size_t parameterCount=179;
inline constexpr std::array<const char*,93> moduleParameterIds{
 "eqPresent","limPresent","stereoPresent","stereoOn","stereoLowWidth","stereoMidWidth","stereoHighWidth","stereoLowSplit","stereoHighSplit","stereoMonoBass","stereoMono","stereoBalance",
 "dynamicsPresent","dynamicsOn","dynamicsMode","dynThreshold","dynRatio","dynAttack","dynRelease","dynKnee","dynMakeup","dynMix","upwardAmount","upwardThreshold",
 "dynamicEqPresent","dynamicEqOn",
 "deqFreq1","deqGain1","deqQ1","deqThreshold1","deqRange1","deqAttack1","deqRelease1","deqBandOn1",
 "deqFreq2","deqGain2","deqQ2","deqThreshold2","deqRange2","deqAttack2","deqRelease2","deqBandOn2",
 "deqFreq3","deqGain3","deqQ3","deqThreshold3","deqRange3","deqAttack3","deqRelease3","deqBandOn3","stereoAirWidth","stereoTopSplit","chainOrder","stereoBandOn1","stereoSolo1","stereoBandOn2","stereoSolo2","stereoBandOn3","stereoSolo3","stereoBandOn4","stereoSolo4","stereoAmount","stereoDelta","stabilizerPresent","stabilizerOn","stabilizerMode","stabilizerProfile","stabilizerAmount","stabilizerSpeed","stabilizerSmoothing","stabilizerLow","stabilizerMid","stabilizerHigh","stabilizerTame","stabilizerDelta","routingMode","stereoizeAmount",
 "parallelCompensated","parallelTrimDynamicEq","parallelMixDynamicEq","parallelTrimDynamics","parallelMixDynamics","parallelTrimStereo","parallelMixStereo","parallelTrimEq","parallelMixEq","parallelTrimLimiter","parallelMixLimiter","parallelTrimStabilizer","parallelMixStabilizer","dynamicsIntensity","dynPunch","dynDelta"};
inline std::string parameterId(size_t i){
 if(i<7)return std::array<const char*,7>{"frequency","gain","q","ceiling","eq","lim","order"}[i];
 if(i<52)return std::string(std::array<const char*,3>{"frequency","gain","q"}[(i-7)%3])+std::to_string(2+(i-7)/3);
 if(i<54)return i==52?"limiterDrive":"limiterRelease";
 if(i<86)return std::string(i%2==0?"bandType":"bandEnabled")+std::to_string(1+(i-54)/2);
 return moduleParameterIds[i-86];
}
}
