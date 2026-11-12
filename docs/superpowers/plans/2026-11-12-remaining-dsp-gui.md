# Remaining MasterLabX DSP/GUI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement task-by-task, inline in the existing isolated worktree.

**Goal:** Implement and validate the remaining approved no-STEMS requirements, without a production or FL Studio acceptance claim.
**Architecture:** Retain C++20/JUCE, the existing parameter IDs and fixed final slot. Add independently testable DSP capabilities before wiring their controls and measured displays. Preserve the verified 02785d8 candidate in history.
**Tech Stack:** C++20, JUCE 8.0.6, CMake, Linux GCC, Windows ClangCL, CTest, pluginval.
**Spec:** Full handoff on docs/codex-masterlabx-audit-gui-dsp-20261110; docs/MASTERLABX_EXECUTION_DIRECTIVE.md; Notion Concept C page 4105fed1901b810e9472e11439017087, fetched 2026-11-12.

## Global constraints
- No STEMS, REBALANCE, main merge, or production publication.
- No allocation, locks, file/network I/O or unbounded work in the audio callback.
- Existing host parameter IDs retain their meaning; new controls append IDs and restore neutral defaults into earlier current-project states.
- Test rates 44100/48000/96000/192000 and blocks 1/17/128/257/4097 where applicable.
- GUI screenshots at 750x550, 1000x700 and 1400x1000; measured data only.

## Review focus
- Replacing state while new controls hold non-default values must restore absent controls to neutral defaults (Task 1).
- Stereo anti-phase input must not disappear from level measurements; waveform labels identify mid-channel projections (Task 1).
- Delta monitoring must preserve the stated dry-minus-processed sign and avoid toggle discontinuities (Task 1).
- Changes of quality/latency during transport must not desynchronize dry/wet or host PDC (Tasks 3/5).
- Silence, bypass and transport resets must remove stale measurements (Tasks 1/2/6).

## Execution order and concrete first deliverable

### Task 1: Unified Analog Exciter Mix/Delta and before/after capture
Files: Source/AnalogExciter.h, SerialGraph.h, GraphConfiguration.h, Processor.h, ModuleParameters.h, StateValidation.h, AnalogizerPanel.h; tests/AnalogExciterMonitorTests.cpp, StateTests.cpp, GuiSmoke.h; CMakeLists.txt.
Interfaces: AnalogExciter::monitor(float mix,bool delta); AnalogExciter::reading() -> AnalogExciterReading; SerialGraph::analogExciterMonitor/analogExciterRead; ResponseFrame::analogExciter.
- [x] RED: unity/default monitor matches the existing harmonic-then-colour chain; mix=0 equals input; delta+normal=input; disabled module delta is zero; partition/reset/nonfinite controls remain finite and consistent.
- [x] Implement sample-smoothed global Mix/Delta with fixed 128-sample scratch storage, independent per-channel levels and continuous ScopeCapture of mid input/output. Preserve existing colour mix semantics.
- [x] Append analogGlobalMix=1 and analogDelta=false; state validation accepts existing 304/316-parameter current-family states and fills only absent fields.
- [x] Add real controls and input/output displays; verify host automation and state restore through actual Processor and GUI.
- [x] Run targeted tests, complete CTest, actual GUI and inspect screenshots, then commit.

### Task 2: Analog Luxury and quantitative colour validation
Files: AnalogizerPanel.h, StudioTheme.h, AnalogExciter.h, Analogizer.h, tests/AnalogizerTests.cpp, tests/AnalogGraphIntegrationTests.cpp, asset pipeline.
- [ ] Integrate approved layout using real monitor capture from Task 1; no example traces baked into assets.
- [ ] Add repeatable H2-H5/THD/IMD/aliasing and control-authority measurements on the routed combined module; measure existing behavior before changing transfers.
- [ ] Implement measured harmonic analysis outside callback with explicit invalid/unpitched state; validate on known tones and broadband signals.
- [ ] Select and validate anti-aliasing based on measured failures, including latency/gain alignment and parameter interaction tests; GUI exposes only implemented quality modes.

2026-11-12 Task 2 RED audit: routed H2-H5 and Tube/FET/Tape signatures are distinct; EXCITE mean absolute authority is 0.24437071 in the deterministic 1 kHz fixture. The 10 kHz audit FAILS the explicit 0.05 alias/fundamental gate: Tube 0.16266517, FET 0.22050812, Tape 0.14815984. Isolating the blocks attributes 0.00023708 to the ADAA harmonic generator and 0.06065963-0.11182596 to the colour stage before combined interaction. Ruling: do not expose a fictitious oversampling control or retain a partial transfer rewrite; the attempted zero-latency first-order rewrite reduced isolated alias but the combined path still failed. Correct work requires an aligned oversampled colour path and verified PDC.

### Task 3: Final limiter and MASTER/INSERT latency
Files: FinalTruePeak.h, Engine.h, SerialGraph.h, Processor.h, FinalTruePeakPanel.h, ProcessingModePanel.h and related tests.
- [ ] Audit combined-stage GR, bypass transitions and independent reconstructed peak tests.
- [ ] Implement actual attack/lookahead/transient controls and oversampling with fixed aligned paths; measure ceiling error and aliasing for each mode.
- [ ] Implement explicit lower-latency INSERT preparation/profile and host PDC reporting without callback latency notifications or implicit parameter overwrites.

### Task 4: Stereo and routing
Files: Stereo.h, SerialGraph.h, ModulePanel.h, ParallelPanel.h, related tests.
- [ ] Read detailed Stereo requirements; implement distinct channel Haas/ping-pong and correctly described phase operation; measure final-output correlation and mono null behavior.
- [ ] Extend supported parallel routes to the full actual node set with latency alignment, explicit final-slot rules and smoothing; test order/state/automation transitions.

### Task 5: Restoration and Brain
Files: AudioRestoration.h, RestorationPanel.h, Brain.h/BrainRules.h/ProcessorBrain.h as actually present, related tests.
- [ ] Replace unvalidated four-region hiss approximation with a validated spectral learn/freeze/reduction path, measuring SNR, transients and musical-noise behavior before acceptance.
- [ ] Validate hum/click/clip controls independently, delta sign and quality/PDC modes; no neural dependency without source/license/quality verification.
- [ ] Integrate complete node selection and manual/auto/hybrid control into Brain with Apply/Undo/A-B and bounded offline preview tests.

### Task 6: Remaining instruments and GUI acceptance
Files: Editor.h, ConceptCOverviewPanel.h, ModulePanel.h, TransientSculptor.h/Panel.h, BassArchitectPanel.h, GuiSmoke.h.
- [ ] Add actual transient envelopes/events and band modulation histories, Dynamic EQ energy/action heatmap, Dynamics before/after energy and full-band GR.
- [ ] Fix Brain/Inspector/EQ compact geometry and label collisions; capture all screens in signal/silence/bypass and HiDPI conditions.
- [ ] Verify meters against independent reference signals and document tolerances.

### Task 7: Final integration and delivery
- [ ] Exact-commit Linux/Windows CTest, pluginval, callback allocation/deadline profiling and host-independent lifecycle tests; fresh whole-branch review.
- [ ] Verify installer, PE imports, archive CRC/hashes/licenses; publish test artifact only, update existing Notion progress with evidence.
- [ ] FL Studio installation, listening and visual acceptance remain external acceptance gates unless an actual host becomes available.

## Ledger
2026-11-12: base 1fff72c, clean isolated worktree. User explicitly requested START on remaining scope. Ruling: execute inline without another approval exchange, preserving explicit authorization; introduce neutral global monitor controls rather than repurposing colour Mix. Later subsystem designs will be refined after their code/measurement audit, not guessed from module names. Task 1 active; no new DSP implementation claimed yet.

Task 1: implemented and locally verified, Linux 47/47 CTest (42.55 s), GUI PASS, ASan/UBSan PASS, process allocation count zero in fixture. Neutral monitor matches pre-monitor combined chain sample-for-sample. States with 304 and 316 fields restore into 318-field state. Windows validation of these changes is pending.
Ruling: colour output retains its old meaning and is labelled Colour output; do not imply a new global output gain. UI currently shows four generated-band RMS values, not H1-H8 analysis.
Ruling: pull forward shared Brain graph configuration from Task 5 after a regression test, because offline preview currently omits host-only Analogizer/Restoration/EXCITE settings. This affects trust in all following measurements.
2026-11-12 Brain preview parity: RED reproduced errors up to 10.7293453 LUFS and 0.394044936 sample peak. Shared graph configuration plus the same latency-aligned INSERT blend produced exact parity in the deterministic fixture for serial/parallel, MASTER/INSERT, and 44.1/48/96/192 kHz. Linux 48/48 CTest PASS (44.44 s), GUI PASS. Ruling: this is an early Task 5 dependency repair; it does not establish Brain proposal quality or FL Studio behavior.
2026-11-12 Task 2 oversampling: fixed 4x colour path with 48-sample aligned Dry/Wet/Delta and constant PDC. Combined 10 kHz alias ratios fell from 0.0862-0.0922 in the retained RED audit to 0.00377-0.00482. Linux plugin CTest 50/50 and GUI smoke passed; Windows run 43203981489 succeeded including build, DSP, GUI, pluginval and packaging. Artifact download into this executor remains unverified because the signed Actions blob URL returned HTTP 403.
2026-11-12 Task 3 limiter audit: RED single-stage reference found maximum error 0.113428533 because the full serial graph ran the legacy 5 ms Engine limiter immediately before FinalTruePeak. Ruling: retain the 5 ms FIFO solely for timing/preset compatibility but remove its second gain computer from the serial final slot; FinalTruePeak is the one output limiter and now supplies the GR trace. Targeted GREEN maximum error 0.000000000; six limiter/graph tests passed. Ruling: do not claim lower-latency INSERT yet. `workMode` is host-automatable and read in the audio callback; changing allocated delay length or host PDC there is unsafe. A control-thread preparation contract or non-automatable instance mode is required before the profile can truthfully change latency.
2026-11-12 Task 4 Stereo source reconciliation: Notion page 4105fed1901b810e9472e11439017087 explicitly requires interpolated channel Haas plus Stereo Motion/ping-pong with mono/correlation control. RED centered-transient test showed the current Side-only delay generated no channel Haas energy. Ruling: preserve neutral/default behavior and IDs, but nonzero Haas now delays a real channel; Motion trades that delay between L/R rather than modulating level pan. Exact sample-wise mono sum is no longer a valid invariant for nonzero channel delay; enforce a bounded mono-energy compatibility gate and expose the existing mono preview/correlation telemetry.
