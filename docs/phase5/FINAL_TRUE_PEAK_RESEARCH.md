# Phase 5C — final true-peak limiting research (draft)
Date: 2026-11-05

## Why
The existing limiter is part of the legacy six-family routing graph; Harmonic Exciter, Analog Color, Bass Architect and Transient Sculptor process after it and can raise output peaks beyond ceiling. A final protection stage is needed after them all, including in the parallel routing case.

## Implemented in this branch
- Standalone `FinalTruePeak` DSP core with 4x polyphase Blackman-windowed-sinc peak reconstruction (25 taps per fractional phase).
- Linked stereo peak detection, optional 0–18 dB drive, -12 to -0.1 dBFS ceiling, 10–1000 ms release.
- Fixed 128-sample delay and 4x peak-based gain calculation, protective sample-peak clamp and measured peak / gain-reduction readings.
- Separate CTest `final_true_peak_dsp`: 44.1 / 48 / 96 / 192 kHz, 128-sample latency check, output sample-peak ceiling, stereo matching, block independence, zero input and non-finite guard.

## NOT YET PRODUCTION READY / NOT IN VST3
- Do not expose to FL Studio or merge before plugin-wide latency and all dry/wet/host bypass and parallel paths are aligned at fixed total latency. Default old project latency must not change silently.
- 4x detector is not a formal BS.1770/EBU true-peak compliance claim yet. Validate reconstructed *output* inter-sample peaks across challenging high-frequency stimuli, transients and rapidly changing gain. Detecting 4x input peaks plus a sample clamp alone does not prove all inter-sample peaks are below the ceiling.
- Full CPU profiling needed: naive 25×4×2 FIR and O(129) peak window per sample is deliberately unoptimized reference.
- In disabled mode this standalone core still adds 128 samples and should not be inserted in existing processing chain until host PDC is redesigned.
- No APVTS parameters, GUI or plugin registration in this branch yet.
- No true-peak guarantee for the current shipping VST3. The existing sample ceiling is upstream of post-chain modules.

## Next technical gates
1. Windows CI on this standalone core, CTest 38/38 and pluginval regression of existing VST3.
2. Independent 4x–8x output true-peak offline tests and worst-case impulse/square-wave measurements; correct errors before deployment.
3. Efficient interpolator plus monotonic window peak tracker, deterministic real-time performance.
4. New explicit host latency model and aligned dry/wet, bypass, parallel, 1x/2x/4x switching.
5. Optional final section GUI and true-peak monitoring, append-only schema 16, no old parameter renumbering; end-to-end Windows x64 pluginval and FL Studio test.

Prior verified merge: PR #14 commit 8209ad00e64d140b626ad5bc1d826250234b2d66.
