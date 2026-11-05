# MasterLabX Phase 3B — Analog Color
Date: 2026-11-05. Status: implementation under review. This document does not certify Windows VST3 or FL Studio tests.

## Architecture
- New **distinct** post-chain DSP stage after Harmonic Exciter, applied after Serial output or Parallel summation, not a seventh/eighth legacy permutation member. The original 720 `chainOrder` permutations are untouched.
- Four **original, inspired-by** algorithms:
  - Tube: asymmetrical biased hyperbolic-tangent response, subtracting zero-input static offset.
  - Tape: saturation with finite response/soft limiting, memory and additional bandwidth restriction.
  - Transformer: variable low-frequency memory and a harder odd soft knee.
  - Solid State: rational transfer function with a firmer overload knee.
- Each has Drive, Bias, Tone (wet-only one-pole tilt), Compression, Dry/Wet, Output and optional level compensation. Mode is automatable but switching is not yet crossfaded.
- 20 ms per-parameter slew/bypass ramp, 12 Hz DC removal, per-channel state, finite guards, no allocations in audio callback, real output/delta RMS.
- Parameter count 201; preset schema 12 appends indices 191–200 and preserves 0–190. Prior preset versions 1–11 migrate using disabled `analogPresent=false` default. Existing 6-family permutations and enabled/disabled module gain law remain unchanged when Analog Color is off.
- Hybrid Studio Analog Color panel: + Modules → Open Analog Color (post chain). Tube/Tape/Transformer/Solid State selection with APVTS attachment; real DSP telemetry.
- Master Brain preview uses `configureGraph` with the full state, so baseline Analog Color settings are included in real preview audio, but the heuristic does **not** recommend or tune Analog Color yet.

## Known limitations
- Experimental **1x** nonlinear processor: aliasing at high drive/high frequencies; no anti-aliased oversampling integrated. No specific hardware circuit or calibrated physical modelling claims.
- Post-chain after sample-peak limiter: saturation can exceed its ceiling. Do not promise True Peak.
- Mode switch is currently not crossfaded and may click; do not automate model choices during a transient.
- High CPU evaluation, host FL Studio validation and full Windows VST3 artifact require a green CI run before release.
- Do not overwrite a production VST3 before validating save/reopen and render on a backup project.

## Test matrix
- `analog_color_dsp`: mode differentiation, true bypass, finite output, silence, automation, multirate 44.1/48/96/192 kHz and block 1/257/4097.
- `analog_graph_integration`: audible DSP effect after serial and parallel; realtime sample-block independence.
- `state_validation`: schema 1–11 migrations to schema 12.
- `gui_smoke`: actual APVTS mode/drive/presence control.
- Windows MSVC, full CTest, pluginval strictness 5 and host FL Studio gates are independent validation steps. Do not claim their success based only on source changes.
