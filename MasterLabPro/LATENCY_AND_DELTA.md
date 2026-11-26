# Latency, PDC and DELTA

Every active module reports latency in samples. A serial graph latency is their bounded sum.
`StereoDelayAligner` uses preallocated stereo storage and supports up to 65,536 samples. Graph publication,
bypass and latency changes refresh the sum. A runtime latency change increments a diagnostic counter and
uses fade-out, alignment reset, then fade-in.

Global analysis is:

```text
GLOBAL_DELTA = GLOBAL_AFTER - delay(GLOBAL_BEFORE, graph_latency)
```

Module analysis taps the selected module immediately before and after processing and delays its BEFORE by
that module's reported latency:

```text
MODULE_DELTA = MODULE_AFTER - delay(MODULE_BEFORE, module_latency)
```

The spectral heat-map's dB difference remains a visualization; it is not substituted for audio DELTA.
DELTA LISTEN uses the aligned audio difference and a 20 ms monitor ramp.

Automated impulse tests cover the technical delay. After alignment, a delay-only graph produces DELTA below
`1e-6` in the tested floating-point path. Changing latency from 3 to 7 samples is also tested. A future VST3
wrapper must forward the graph latency and issue the host's latency-change notification; that wrapper is
Package C scope.

## Package C alignment corrections

Delay-line reset invalidates history in constant time without clearing the storage. The current block's alignment and tap calculations run before graph retirement and latency refresh, so a new graph cannot reset the alignment of an already processed block. Global and selected-module aligned input buffers are separate. The global aligner advances even when no analysis output was requested. Dynamic-latency transition behavior and DAW PDC still require validation; bypass now preserves the module's latency using a separate preallocated dry delay; native processing continues to keep history warm.

Regression: bypass of a 7-sample technical delay must retain 7-sample PDC and produce a delayed dry impulse with zero steady-state DELTA. Ten stereo delay lines are allocated outside audio; bypass is not a CPU-saving switch. Transition artifacts and DAW notification still require testing.
