#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <charconv>
#include <cmath>
#include <set>

namespace masterlab {
// State restoration is a host/control-thread operation, never an audio callback operation.
// Validate the whole candidate before touching APVTS so a bad preset is atomic: no changes.
inline juce::ValueTree validateState(const juce::ValueTree& candidate,
                                    juce::AudioProcessorValueTreeState& parameters,
                                    const juce::Array<juce::AudioProcessorParameter*>& expected) {
    const auto schema=candidate["schemaVersion"].toString();
    const bool legacy=schema=="1";
    if (!candidate.hasType("MasterLab") || (schema!="1"&&schema!="2")
        || candidate["eqInstanceId"].toString() != "eq-0001"
        || candidate["limiterInstanceId"].toString() != "limiter-0001"
        || candidate.getNumChildren() != expected.size()-(legacy?2:0))
        return {};

    std::set<juce::String> seen;
    auto canonical = juce::ValueTree("MasterLab");
    canonical.setProperty("schemaVersion", 2, nullptr);
    canonical.setProperty("eqInstanceId", "eq-0001", nullptr);
    canonical.setProperty("limiterInstanceId", "limiter-0001", nullptr);
    for (const auto& child : candidate) {
        if (!child.hasType("PARAM") || child.getNumChildren() != 0
            || child.getNumProperties() != 2 || !child.hasProperty("id")
            || !child.hasProperty("value"))
            return {};
        const auto id = child["id"].toString();
        auto* parameter = parameters.getParameter(id);
        if (parameter == nullptr || (legacy&&(id=="limiterDrive"||id=="limiterRelease")) || !seen.insert(id).second)
            return {};
        const auto text = child["value"].toString().toStdString();
        float value = 0;
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        const auto range = parameter->getNormalisableRange();
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size()
            || !std::isfinite(value) || value < range.start || value > range.end
            || (parameter->isBoolean() && value != 0.f && value != 1.f))
            return {};
        auto node = juce::ValueTree("PARAM");
        node.setProperty("id", id, nullptr);
        node.setProperty("value", value, nullptr);
        canonical.appendChild(node, nullptr);
    }
    if(legacy){
        for(const auto& id:{juce::String("limiterDrive"),juce::String("limiterRelease")}){
            auto node=juce::ValueTree("PARAM");node.setProperty("id",id,nullptr);
            node.setProperty("value",id=="limiterDrive"?0.f:80.f,nullptr);canonical.appendChild(node,nullptr);
        }
    }
    return canonical;
}
}
