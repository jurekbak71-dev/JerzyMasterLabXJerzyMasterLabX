# Phase 4B — Oversampling research and smooth colour-model switching
Date: 2026-11-05. This branch is stacked **on top of Phase 4A PR #11** and must not be merged directly into main ahead of it.

## Implemented
- Experimental cascading of two 65-tap windowed-sinc 2x FIR processors yields a 4x nonlinear signal path. Calculated overall group delay = 48 base-rate samples; tests check 1 kHz amplitude and 10 kHz cubic-generated 18 kHz alias at 48 kHz, plus silence and 44.1/48/96/192 kHz.
- Harmonic Exciter standalone core now accepts `oversample4x` (default false) with matched 48-sample internal dry FIFO, in addition to prior 1x (0 latency) and 2x (32 samples). `latencySamples()` describes its internal mode.
- **Not exposed by VST3/APVTS**: graph configuration still forces existing host 1x. Safe activation requires a fixed host PDC design and synchronized dry/parallel/bypass paths. The new mode cannot be assumed production-ready merely from passing unit tests.
- Analog Color mode selection now crossfades 20 ms between independently advanced old/new DSP states, with a smoothstep equal-time ramp. No additional delay. Retains previous output algorithm outside mode transitions, and automatically doubles nonlinear calculations for up to 20 ms on a model switch.
- Test for Tube→Solid State switching while processing constant input, and full-core 4x null/alias tests.

## Constraints and future steps
1. New impact parameters from PR #11 remain dependent; default zero preserves phase 3 behavior.
2. Aliasing currently remains possible in the actual VST3 1x path at high Drive/Impact.
3. Cascaded 4x implementation is relatively CPU-heavy; benchmark before exposing it to host. Prefer optimized polyphase FIR SIMD or JUCE oversampling with measured fixed PDC.
4. Real host 2x/4x selection requires delayed global dry/bypass alignment, fixed worst-case latency (for example, 48 samples) and correct history on enable/disable. Presets must retain old audible behavior at default. Do not silently add host latency without verifying prior sample-perfect renders.
5. Rapid repeated mode changes inside an active 20 ms crossfade deserve additional stress tests.
6. Run full Windows MSVC CTest, pluginval strictness 5, GUI smoke, then FL Studio manual host test; do not merge until verified.

## Status
Development implemented on branch; Windows CI and installable VST3 checks are independent.
