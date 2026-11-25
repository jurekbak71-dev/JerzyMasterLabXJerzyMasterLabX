# MasterLabPro Package B architecture

## Runtime layers

| Layer | Implementation | Thread role |
|---|---|---|
| Module ABI/SDK | `sdk/masterlabpro_module_abi.h` | Contract only |
| Loader and registry | `ModuleLoader`, `ModuleRackHost` | Control thread only |
| Parameter registry | `ParameterRegistry` | Control mapping; atomic module updates |
| Immutable graph snapshots | `GraphSnapshot`, `ModularGraph` | Produced by control, consumed by audio |
| Latency and DELTA | `StereoDelayAligner`, graph taps | Audio thread, preallocated |
| State | `RackStateCodec`, module state callbacks | Stopped/control thread only |
| Analysis | existing `AnalysisCapture`/`AnalysisHistory` | Capture on audio, FFT and history off audio |
| GUI | existing A+C `RackEditor`, dynamic module panel | JUCE message thread |

The console discovers modules only in its explicit `modules` directory. A successful loader owns the
native library until every instance has been destroyed. Instances are prepared before publication.
The control thread publishes a fixed-size snapshot through an SPSC queue. The callback never owns or
destroys a library or instance.

```text
GUI/controller -> ModuleRackHost -> prepared GraphSnapshot -> bounded SPSC
                                                        -> ModularGraph -> DSP modules
audio capture <- meters/analysis <- aligned taps <------+
control retirement <- retired snapshot SPSC <----------+
```

Ten visible rack positions determine serial DSP order. Instance identifiers are independent of slot
positions. Reordering therefore changes processing order without changing parameter identity or module
state.

## Transition policy

The same stateful module object is never processed by two graphs simultaneously. A topology change ramps
the old graph to silence for 128 samples, publishes the new snapshot at the block boundary, then ramps it
in. This is intentionally a bounded non-overlapping transition rather than an uncontrolled dual-graph
crossfade. A latency change uses the same envelope and resets the corresponding alignment history while
silent. This can create a very short level dip; it avoids a click and avoids duplicating stateful DSP.

## Security boundary

A dynamically loaded native module executes in the host process. Directory and ABI validation reduce
accidental loading and incompatibility; they are not a sandbox and cannot protect the host from malicious
or defective native code.

## Package boundary

Package B supplies the host core, SDK, technical modules, GUI integration and tests. It does not implement
a VST3 wrapper, FL Studio validation, Master Brain, STEMS, or the nine production mastering processors.
