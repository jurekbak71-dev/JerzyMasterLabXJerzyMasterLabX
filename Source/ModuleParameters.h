#pragma once
#include <array>
#include <cstddef>
#include <string>
namespace masterlab {
// Current host parameter table without the removed LIVE/OFFLINE STEMS module.
inline constexpr size_t parameterCount=278;
inline constexpr std::array<const char*,140> moduleParameterIds{
 "eqPresent","limPresent","stereoPresent","stereoOn","stereoLowWidth","stereoMidWidth","stereoHighWidth","stereoLowSplit","stereoHighSplit","stereoMonoBass","stereoMono","stereoBalance",
 "dynamicsPresent","dynamicsOn","dynamicsMode","dynThreshold","dynRatio","dynAttack","dynRelease","dynKnee","dynMakeup","dynMix","upwardAmount","upwardThreshold",
 "dynamicEqPresent","dynamicEqOn",
 "deqFreq1","deqGain1","deqQ1","deqThreshold1","deqRange1","deqAttack1","deqRelease1","deqBandOn1",
 "deqFreq2","deqGain2","deqQ2","deqThreshold2","deqRange2","deqAttack2","deqRelease2","deqBandOn2",
 "deqFreq3","deqGain3","deqQ3","deqThreshold3","deqRange3","deqAttack3","deqRelease3","deqBandOn3","stereoAirWidth","stereoTopSplit","chainOrder","stereoBandOn1","stereoSolo1","stereoBandOn2","stereoSolo2","stereoBandOn3","stereoSolo3","stereoBandOn4","stereoSolo4","stereoAmount","stereoDelta","stabilizerPresent","stabilizerOn","stabilizerMode","stabilizerProfile","stabilizerAmount","stabilizerSpeed","stabilizerSmoothing","stabilizerLow","stabilizerMid","stabilizerHigh","stabilizerTame","stabilizerDelta","routingMode","stereoizeAmount",
 "parallelCompensated","parallelTrimDynamicEq","parallelMixDynamicEq","parallelTrimDynamics","parallelMixDynamics","parallelTrimStereo","parallelMixStereo","parallelTrimEq","parallelMixEq","parallelTrimLimiter","parallelMixLimiter","parallelTrimStabilizer","parallelMixStabilizer","dynamicsIntensity","dynPunch","dynDelta","stabilizerIntensity",
 "exciterPresent","exciterOn","exciterAmountLow","exciterAmountMid","exciterAmountHigh","exciterAmountAir","exciterEven","exciterOdd","exciterDrive","exciterMix","exciterOutput",
 "analogPresent","analogOn","analogMode","analogDrive","analogBias","analogTone","analogDynamics","analogMix","analogOutput","analogAutoGain","exciterImpact","analogImpact","bassPresent","bassOn","bassSub","bassBody","bassHarmonics","bassMono","bassMix","bassOutput","transientPresent","transientOn","transientAttackLow","transientAttackMid","transientAttackHigh","transientSustainLow","transientSustainMid","transientSustainHigh","transientMix","transientOutput","truePeakPresent","truePeakOn","truePeakCeiling","truePeakRelease","truePeakDrive"};
inline std::string parameterId(size_t i){
 if(i<7)return std::array<const char*,7>{"frequency","gain","q","ceiling","eq","lim","order"}[i];
 if(i<52)return std::string(std::array<const char*,3>{"frequency","gain","q"}[(i-7)%3])+std::to_string(2+(i-7)/3);
 if(i<54)return i==52?"limiterDrive":"limiterRelease";
 if(i<86)return std::string(i%2==0?"bandType":"bandEnabled")+std::to_string(1+(i-54)/2);
 if(i<226)return moduleParameterIds[i-86];
 if(i==226)return "fullRouterOn";
 if(i<238)return "fullRouteSlot"+std::to_string(i-226);
 if(i<278){constexpr std::array<const char*,8> fields{"deqFreq","deqGain","deqQ","deqThreshold","deqRange","deqAttack","deqRelease","deqBandOn"};return std::string(fields[(i-238)%8])+std::to_string(4+(i-238)/8);}
 return "unknown";

}
}
