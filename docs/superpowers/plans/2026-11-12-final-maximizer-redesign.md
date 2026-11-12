# Final Maximizer Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the selected Modern Metering Console GUI and replace the audible clamp-based final stage with a tested density, oversampled soft-clip and true-peak maximizer path while fixing the false-active state.

**Architecture:** Keep `FinalTruePeak` as the single fixed-latency final stage, but split its internals into bounded pre-compression, antialiased pre-shaping, program gain control and final true-peak safety. Extend existing lock-free telemetry for the new native JUCE panel; generated raster assets provide only reusable chrome while live values and meters remain native drawing.

**Tech Stack:** C++20, JUCE APVTS/VST3, CMake/CTest, existing lock-free FIFOs and scope capture, generated RGBA GUI assets.

**Spec:** `docs/superpowers/specs/2026-11-12-final-maximizer-redesign.md`

## Global Constraints

- No STEMS or REBALANCE work.
- No MASTER/INSERT redesign in this stage.
- No allocation, locks, file/network access or unbounded work in the audio callback.
- Preserve fixed and correctly reported host latency across bypass and automation.
- Do not copy iZotope artwork, branding, IRC algorithms or trade dress.
- Do not merge to `main` or publish a production release.
- Every behavior change follows a failing-test-first cycle.

## Review Focus

- A restored/default green power state must never disagree with the actual final-stage DSP state; covered by Task 1 state and first-block tests.
- Sustained low bass under high density must recover without waveform-period chatter; covered by Task 2 bass/recovery tests.
- Exciter-fed near-Nyquist content must remain finite and bounded without excessive alias growth; covered by Task 3 audit tests.
- Parameter automation at arbitrary callback boundaries must remain continuous and block invariant; covered by Tasks 2 and 4 partition tests.
- Small editor sizes must preserve power, ceiling, meters and primary faders without overlap; covered by Task 5 GUI smoke matrix.

---

### Task 1: Authoritative activation state

**Files:**
- Modify: `Source/GraphConfiguration.h`
- Modify: `Source/FinalTruePeakPanel.h`
- Modify: `Source/Editor.h`
- Modify: `tests/GuiSmoke.h`
- Modify: `tests/ModuleIntegrationTests.cpp`

**Interfaces:**
- Consumes: APVTS parameters `truePeakPresent`, `truePeakOn`, legacy `limPresent`, `lim`.
- Produces: one shared final-stage-active predicate used by DSP configuration and visible power state.

- [ ] **Step 1: Write failing state tests** asserting that absent+enabled renders inactive, present+enabled processes the first block, restored present+enabled requires no toggle cycle, and OFF retains the fixed delay.
- [ ] **Step 2: Run targeted tests** with `ctest --test-dir build-plugin -R 'module_integration|gui_smoke' --output-on-failure`; expected: the absent+enabled truth test fails on the current implementation.
- [ ] **Step 3: Implement one authoritative active predicate** and make panel/card status derive from presence AND enabled state; do not mutate presence as a side effect of repainting.
- [ ] **Step 4: Re-run targeted tests**; expected: PASS.
- [ ] **Step 5: Commit** state test and fix as `Fix final maximizer activation truth`.

### Task 2: Maximizer signal path

**Files:**
- Modify: `Source/FinalTruePeak.h`
- Modify: `tests/FinalTruePeakTests.cpp`
- Modify: `tests/FinalTruePeakDifferentialTests.cpp`
- Modify: `tests/FinalTruePeakAdversarialTests.cpp`
- Modify: `tests/FinalTruePeakBenchmark.cpp`

**Interfaces:**
- Consumes: `FinalTruePeakSettings` plus fixed `FinalTruePeak::latencySamples`.
- Produces: bounded density compressor, oversampled soft clip, mode-dependent program limiter and measured `FinalTruePeakReading` fields for input/output/GR/density.

- [ ] **Step 1: Add failing audio tests** for no routine hard-clamp plateaus, sine/impulse/bass/dense/near-Nyquist stimuli, density gain bounds, mode differentiation, bypass continuity, parameter ramps, callback partition invariance and independent 8x output ceiling measurement.
- [ ] **Step 2: Run `final_true_peak_dsp`, differential and adversarial tests**; expected: new density/plateau/continuity assertions fail.
- [ ] **Step 3: Add settings and state** for `density`, `compressorMix`, `stereoLink` and `delta`; implement a stereo-linked soft-knee density stage with bounded makeup and smoothed wet/dry.
- [ ] **Step 4: Implement oversampled pre-clip and program limiter** so all nonlinearity precedes the true-peak detector and the normal output path does not depend on sample hard clipping.
- [ ] **Step 5: Implement Clean/Punch/Dense envelope policies** using bounded release/hold behavior and continuous smoothed transitions while retaining constant latency.
- [ ] **Step 6: Re-run targeted tests and benchmark**; expected: all pass and benchmark reports no regression above the existing CI budget.
- [ ] **Step 7: Commit** as `Rebuild final maximizer signal path`.

### Task 3: Exciter peak and alias safeguards

**Files:**
- Modify: `Source/HarmonicExciter.h`
- Modify: `Source/GraphConfiguration.h`
- Modify: `tests/AnalogExciterQualityAudit.cpp`
- Modify: `tests/ImpactAuthorityTests.cpp`
- Modify: `tests/AnalogGraphIntegrationTests.cpp`

**Interfaces:**
- Consumes: current Harmonic Exciter band drive/mix/output settings and maximizer input.
- Produces: production-quality oversampling selection and bounded Exciter output/crest behavior without hidden final clipping.

- [ ] **Step 1: Add failing audits** for high-drive multitone, near-Nyquist and four-band maximum settings, asserting finite output, bounded peak growth, measured alias-energy limit and click-free bypass/mix transitions.
- [ ] **Step 2: Run Exciter quality/integration tests**; expected: at least peak-growth or alias audit fails with the current forced non-oversampled configuration.
- [ ] **Step 3: Enable the existing oversampled production path** through graph configuration and replace broad output clamping with bounded nonlinear contribution and smoothed gain compensation proven by the tests.
- [ ] **Step 4: Add an Exciter-to-Maximizer integration render** and assert independent true-peak ceiling and finite output.
- [ ] **Step 5: Re-run targeted tests and benchmarks**; expected: PASS within the documented CPU limit.
- [ ] **Step 6: Commit** as `Harden exciter into final maximizer`.

### Task 4: Parameters, state and telemetry

**Files:**
- Modify: `Source/ModuleParameters.h`
- Modify: `Source/Processor.h`
- Modify: `Source/GraphConfiguration.h`
- Modify: `Source/FinalTruePeak.h`
- Modify: `Source/StateValidation.h`
- Modify: `tests/StateTests.cpp`
- Modify: `tests/ScopePartitionTests.cpp`

**Interfaces:**
- Produces APVTS IDs: `truePeakDensity`, `truePeakCompressorMix`, `truePeakStereoLink`, `truePeakDelta`; preserves existing IDs for ceiling, release, drive, output, soft clip, attack, lookahead, transient emphasis and quality.
- Produces telemetry: per-frame input L/R peak, output L/R peak, limiter GR, density contribution, short loudness and transient markers.

- [ ] **Step 1: Add failing parameter/state tests** for defaults, automation, round-trip and accepted current-state migration, plus scope partition invariance for every new telemetry lane.
- [ ] **Step 2: Run state and scope tests**; expected: missing parameter/telemetry assertions fail.
- [ ] **Step 3: Append parameters and wire configuration** without reordering existing host parameter IDs; increment current schema only after round-trip tests define the new state.
- [ ] **Step 4: Extend fixed-size telemetry** with no audio-thread allocation and reset it on activation generation changes.
- [ ] **Step 5: Re-run state, scope, allocation and module integration tests**; expected: PASS.
- [ ] **Step 6: Commit** as `Expose maximizer controls and telemetry`.

### Task 5: Modern Metering Console GUI and assets

**Files:**
- Create: `Assets/Maximizer/` generated 1x/2x PNG assets
- Modify: `tools/gui_assets/render_concept_c.py`
- Modify: `tools/gui_assets/validate_concept_c.py`
- Modify: `CMakeLists.txt`
- Modify: `Source/StudioTheme.h`
- Replace: `Source/FinalTruePeakPanel.h`
- Modify: `tests/GuiSmoke.h`
- Modify: `scripts/gui-smoke.sh`

**Interfaces:**
- Consumes: Task 4 APVTS IDs and telemetry.
- Produces: selected option 1 layout at supported editor sizes with real host gestures and value entry.

- [ ] **Step 1: Extend failing GUI smoke** to require linear slider styles, authoritative power state, all primary component IDs, host gestures/value edits and non-overlapping bounds at 750, 1000 and 1440 px widths.
- [ ] **Step 2: Run GUI smoke**; expected: new component/style/geometry assertions fail.
- [ ] **Step 3: Render and inspect individual assets** for fader caps/tracks, switch states, selector/readout/display bezels and meter overlays; keep text, values and live fills out of raster assets.
- [ ] **Step 4: Register assets in BinaryData/theme** and implement the option 1 panel hierarchy with vertical primary faders, native history graph, IN/GR/OUT meter bridge and horizontal lower controls.
- [ ] **Step 5: Capture actual JUCE screenshots** at the three target widths and compare them with the selected generated reference; fix P0-P2 overlap, hierarchy and state-legibility issues.
- [ ] **Step 6: Run GUI smoke and plugin parameter integration**; expected: PASS.
- [ ] **Step 7: Commit** as `Build modern final maximizer console`.

### Task 6: Full regression and Windows evidence

**Files:**
- Modify only if a test exposes a root cause in files owned by Tasks 1-5.
- Update: project evidence/ledger file if present and already used by this branch.

**Interfaces:**
- Consumes: complete branch implementation.
- Produces: exact-SHA local and Windows evidence; no merge or release.

- [ ] **Step 1: Run core CTest and plugin CTest suites** with `--output-on-failure`; record exact totals, duration and failures.
- [ ] **Step 2: Run allocation, CPU, adversarial true-peak, GUI smoke and deterministic render checks**; record measured worst case and limits.
- [ ] **Step 3: Run `git diff --check` and inspect final screenshots** for silence, active limiting, bypass and small/large geometry.
- [ ] **Step 4: Commit any evidence-only update**, then push only the feature branch.
- [ ] **Step 5: Run Windows x64 VST3 CI for the exact SHA** and require compilation, CTest, GUI smoke, pluginval and package steps to pass.
- [ ] **Step 6: Report verified, unverified and unfinished items** without merging to `main`.

