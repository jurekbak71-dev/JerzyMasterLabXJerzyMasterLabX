# Phase 5A — Bass Architect experimental implementation
Date: 2026-11-05. Stacked upon PR #12, which is stacked upon PR #11. **Do not merge directly into main or claim host-tested before Windows CI and FL Studio validation.**

## DSP
A new optional post-graph Bass Architect, after Harmonic Exciter and Analog Color (not inserted into legacy six-family 720 permutations):
- split at 90 Hz and 250 Hz by matched one-pole filter states; sub, body, high regions recombined when neutral;
- independent Sub and Body +/-12 dB approximate controls;
- harmonic enhancement of low bands with nonlinear tanh plus DC blocker;
- low-band mono blend between original channel bass and L/R average, leaving high band stereo untouched;
- 20 ms parameter smoothing, bypass, dry/wet and output +/-12 dB;
- RMS telemetry of input/output/delta/low; no allocation inside audio callback.

Eight APVTS parameters append to schema 13 => **schema 14 / 211 total**, IDs:
bassPresent, bassOn, bassSub, bassBody, bassHarmonics, bassMono, bassMix, bassOutput.
Factory bassPresent=false. Existing schema 1..13 state restored using defaults for new fields.
Standalone UI panel added to + Modules, with real JUCE/APVTS controls and a meter.
Tests: bass_architect_dsp, GUI Smoke, state migration, entire Windows CI.

## Caveats
- Filtering is simple first-order (not transparent linear-phase mastering crossover); boundaries may not produce a perfectly phase-linear reconstruction at nonzero gains.
- Strong monoBass can cancel anticorrelated sub. Expected result, warn users.
- Processor follows the existing limiter and thus may exceed peak ceiling, as do two prior post-chain nonlinear modules. True peak final limiting is a later mandatory task.
- Harmonic enhancement remains 1x, no oversampling in Bass Architect yet. It may produce aliasing at high settings; a spectrum and CPU review remains.
- No measured CPU performance nor host confirmation yet. No high-end physical hardware emulation is claimed.
- Refactoring and 3D GUI rework is a separate project stage, intended to preserve parameter IDs and preset compatibility.
