# Phase 3 — Harmonic Exciter and Analog Color (implementation specification)
Status: approved development direction, NOT implemented. 2026-11-04.
Branch for first slice: `codex/harmonic-exciter-phase3a`.
Prior phases #2 and #4–#6 are merged into main; baseline is schema 10 / 180 parameters and six-module routing. Use current code as source of truth (previous roadmap has historical baseline numbers).

## Design priorities
- Musical, strongly perceptible color at high settings and subtle Clean settings. Every slider needs actual DSP effect, measurable and audible on suitable stimulus; no decorative knobs.
- True individual DSP for two **separate** processor families; do not simply relabel the existing Analog Dynamics mode.
- Windows x64 JUCE 8 VST3 for FL Studio; robust bypass/restore; realtime-safe. Preserve existing projects/sounds exactly at default/bypass.
- No unverified emulation claims of specific brands, tape machines or transformer hardware.
- Keep existing limiter sample-peak label until true-peak limiter work; nonlinear processing after limiter can breach ceiling, so warn or constrain ordering only with user consent.

## Phase 3A — Harmonic Exciter: implement FIRST
DSP architecture:
- Multiband processing (suggest 3–4 bands with complementary reconstruction verified across sample rates and split edges) operating on high-passed/band-limited input to each nonlinear stage; clean path perfectly reconstructed for Drive=0 / Amount=0.
- Independently adjustable drive and even/odd harmonic balance. Choose mathematically controllable transfer laws with DC-neutral behavior, optional bias (safe DC blocking) and asymmetric response.
- Band-level thresholds or Amounts and crossover controls, global Amount/Mix, output trim, automatable Bypass; dry/wet should be true aligned wet/dry with matched timing.
- M/S optional only if tested with anti-phase and mono compatibility; avoid silently changing low-mid/bass.
- Oversampling (prefer 2x/4x switch) and anti-alias filtering proven by measurements. Document and compensate PDC for all modes, dry and wet. No latency changes while rendering without host notification and safe transitions; if dynamic latency not supportable, keep fixed worst-case reported latency.
- Saturation should be obvious when desired but avoid unbounded peaks, denormals, NaNs and DC. Verify output level compensation and loudness-matched before/after.
- Meter actual produced harmonic content (FFT off callback) and real band/total activity, including silence flags; Delta = actual wet/dry contribution.
- Hybrid Studio panel: differentiated four colored band controls, responsive drive/shape/meters and real spectrum. Controls APVTS-bound and verifiably connected; no fake animations.

## Phase 3B — Analog Color: separate follow-up PR
- Modes Tube / Tape / Transformer / Solid State with genuinely distinct transfer + dynamics behaviors and musically useful Drive, Bias, Tone, Compression, Mix, Output and optional Flux/Hysteresis character; complexity must be proportional to validated sonic benefit.
- Tape mode must not imply exact physical hysteresis without an implementation; optional wow/flutter are bounded and disabled by default; avoid unintended pitch variation.
- Proper DC protection, oversampling strategy, anti-alias tests and fixed/safely managed PDC. Stable automation without clicks.
- Hybrid Studio panel with analog-inspired controls, genuine gain-reduction and harmonic measurements; all controls connect to DSP.
- Compare modes using harmonics/dynamics and matched-level music, not merely 4 different names for one waveshaper.

## Integration and backwards compatibility
- Current module order encoding is 720 permutations of SIX module families in `Source/SerialGraph.h`. Adding a 7th processor must NOT reinterpret existing `chainOrder` or hardcode new permutations into old IDs. Establish new versioned routing representation or compatible secondary placement mechanism, documenting old/new behavior. Legacy 6 modules should render identically. First review routing alternatives and choose least-invasive safe design.
- Preserve all first 180 APVTS parameter IDs and positional indices. Append new IDs. Bump schema version and validate migrations of schema 1–10, including .jmlx, host state/automation; user must explicitly control new module presence/bypass and safe default off.
- Update all paths: realtime DSP, both banks, serial and parallel, graph configuration, brain preview configuration, state import/export, rendering tests, UI chain controls and activity telemetry. A new module must not disappear from preview or parallel processing.
- Preserve older snapshots' PDC and gain law under default options. Test changeover of module topology while playing, CPU/RAM and no audio callback heap allocations, mutex or FFT.
- If adding a seventh module makes graph integration too large, FIRST deliver a tested DSP-only library with clear interface and test evidence, then a separate plugin-integration PR. Do NOT claim an installable VST3 feature until it is integrated and tested.

## Test gates and evidence
- At least 44.1 / 48 / 96 / 192 kHz and blocks 1 / 128 / 257 / 4097; test silence, DC, dual mono, sine (multiple frequencies), sparse transients, noise and music where licenses permit.
- FFT-based fundamental/H2/H3/H4/H5 and spectral alias-energy comparisons at 1x/2x/4x; verify odd/even controls vary independently, nonzero extra harmonics in Strong/Extreme, no regression at neutral/default.
- Dry-wet null tests with latency alignment; click-free parameter automation and bypass; deterministic or explicitly bounded numerical behavior; no memory growth.
- CI Windows MSVC + complete CTest, strict pluginval level 5, ASan/UBSan targets, GUI reopen/resize 750x550/1000x700/2000x1400, FL Studio manual host tests noted pending unless actually done.
- Publish reproducible code, DSP metrics, before/after WAVs with matched loudness, screenshot of actual GUI, download-ready zipped Windows VST3 and SHA-256 of tested package. No completion claims before evidence.

## PR scope and cadence
PR 3A-1 DSP architecture + measured prototype -> PR 3A-2 complete GUI/VST3 integration and Windows artifact (these may be one PR if comfortably bounded) -> 3B Analog Color separately. Avoid combining both experimental processors with routing overhaul in a single unreviewable PR. Documentation must identify which step is actually done.
