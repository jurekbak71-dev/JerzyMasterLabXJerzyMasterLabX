#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <charconv>
#include <cmath>
#include <set>
#include "ModuleParameters.h"

namespace masterlab {
// State restoration is a host/control-thread operation, never an audio callback operation.
// Validate the whole candidate before touching APVTS so a bad preset is atomic: no changes.
inline juce::ValueTree validateState(const juce::ValueTree& candidate,
                                    juce::AudioProcessorValueTreeState& parameters,
                                    const juce::Array<juce::AudioProcessorParameter*>& expected) {
    const auto schema=candidate["schemaVersion"].toString();
    const int sourceCount=schema=="1"?52:(schema=="2"?54:(schema=="3"?86:(schema=="4"?138:(schema=="5"?161:(schema=="6"?162:(schema=="7"?163:(schema=="8"?176:(schema=="9"?static_cast<int>(parameterCount):0))))))));
    if (!candidate.hasType("MasterLab") || sourceCount==0 || expected.size()!=static_cast<int>(parameterCount)
        || candidate["eqInstanceId"].toString() != "eq-0001"
        || candidate["limiterInstanceId"].toString() != "limiter-0001"
        || candidate.getNumChildren() != sourceCount) return {};
    if((schema=="4"||schema=="5"||schema=="6"||schema=="7"||schema=="8"||schema=="9")&&(candidate["dynamicEqInstanceId"].toString()!="dynamic-eq-0001"||candidate["dynamicsInstanceId"].toString()!="dynamics-0001"||candidate["stereoInstanceId"].toString()!="stereo-0001"))return {};
    if((schema=="5"||schema=="6"||schema=="7"||schema=="8"||schema=="9")&&candidate["stabilizerInstanceId"].toString()!="stabilizer-0001")return {};
    std::set<juce::String> permitted,seen;
    for(int i=0;i<sourceCount;++i){auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(expected[i]);if(!parameter)return {};permitted.insert(parameter->paramID);}
    auto canonical = juce::ValueTree("MasterLab");
    canonical.setProperty("schemaVersion", 9, nullptr);
    canonical.setProperty("eqInstanceId", "eq-0001", nullptr);
    canonical.setProperty("limiterInstanceId", "limiter-0001", nullptr);
    canonical.setProperty("dynamicEqInstanceId", "dynamic-eq-0001", nullptr);canonical.setProperty("dynamicsInstanceId", "dynamics-0001", nullptr);canonical.setProperty("stereoInstanceId", "stereo-0001", nullptr);
    canonical.setProperty("stabilizerInstanceId","stabilizer-0001",nullptr);
    for (const auto& child : candidate) {
        if (!child.hasType("PARAM") || child.getNumChildren() != 0
            || child.getNumProperties() != 2 || !child.hasProperty("id")
            || !child.hasProperty("value"))
            return {};
        const auto id = child["id"].toString();
        auto* parameter = parameters.getParameter(id);
        if (parameter == nullptr || !permitted.contains(id) || !seen.insert(id).second)
            return {};
        const auto text = child["value"].toString().toStdString();
        float value = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        const auto range = parameter->getNormalisableRange();
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size()
            || !std::isfinite(value) || value < range.start || value > range.end
            || (parameter->isBoolean() && value != 0.f && value != 1.f)
            || ((dynamic_cast<juce::AudioParameterChoice*>(parameter)!=nullptr || dynamic_cast<juce::AudioParameterInt*>(parameter)!=nullptr) && std::floor(value)!=value))
            return {};
        auto node = juce::ValueTree("PARAM");
        node.setProperty("id", id, nullptr);
        node.setProperty("value", value, nullptr);
        canonical.appendChild(node, nullptr);
    }
    // Append only newly introduced fields, using version-independent factory defaults.
    for(int i=sourceCount;i<expected.size();++i){
        auto* parameter=dynamic_cast<juce::RangedAudioParameter*>(expected[i]);if(!parameter)return {};
        auto node=juce::ValueTree("PARAM");node.setProperty("id",parameter->paramID,nullptr);
        node.setProperty("value",parameter->convertFrom0to1(parameter->getDefaultValue()),nullptr);canonical.appendChild(node,nullptr);
    }
    return canonical;
}
}
