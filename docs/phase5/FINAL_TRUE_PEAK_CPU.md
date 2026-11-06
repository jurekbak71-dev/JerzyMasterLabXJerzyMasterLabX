# MasterLabX Phase 5D — Final True-Peak CPU optimization

Date: 2026-11-06. Branch `feat/phase5d-truepeak-cpu-20261106`.

## Scope

Optimize the inner loop of the final 4x True-Peak limiter **without altering its user-facing controls, preset schema, lookahead latency, sample-level DSP response, or Hybrid Studio GUI**.

The original `Source/FinalTruePeak.h` scans a ring of 129 peak estimates on **every processed sample** to obtain the stereo-linked lookahead maximum. This is O(129) peak comparisons per sample, plus the 4x/25-tap FIR.

Phase 5D replaces only that scanning maximum with a fixed-size monotonic deque of (peak, sample index) entries. Old elements expire after 129 frames; weaker or equal candidates are discarded. The front tracks the exact maximum of the same sliding window. Time complexity is amortized O(1) per frame, memory is constant, and no allocation, lock or heap is introduced in the audio callback.

## Verification gates

- `tests/FinalTruePeakReference.h` is a frozen copy of the pre-optimization core from merged PR #15.
- `final_true_peak_differential` compares both channels sample-by-sample under four sample rates, bypass, full drive, limiting, sparse impulses, stereo differences, variable block sizes, and mid-stream automation. It also compares detector and reduction telemetry.
- Run all existing 39 regressions, Windows x64 CI, pluginval strictness 5, and GUI smoke. Do not merge on a failed test.
- The optimization does not change 4x FIR cost. A CPU benchmark on Windows is still necessary to quantify total plugin CPU savings.

## Known limitations retained

This optimization is **not** certification of true-peak intersample ceiling compliance for arbitrary signals. The 4x reconstructed *input* detector plus a sample-peak clamp and 2.5% detector headroom require stronger adversarial tests against independently 8x or higher reconstructed *output*. Host PDC remains 128 extra samples even when the final stage is bypassed.
