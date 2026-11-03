# MasterLabX — Hybrid Studio + Master Brain AI 2.0
Status: approved direction / implementation brief; 2026-11-03. This document specifies planned work, NOT implemented features.

## Goal and principles
Turn MasterLabX into an expressive mastering toolkit without sacrificing transparency, realtime safety, FL Studio compatibility or measurable quality. User reports that Stereo Architect is almost inaudible and other modules too subtle. Preserve Clean mastering, but expose Strong and Extreme sound shaping. GUI direction: Hybrid Studio, modern digital displays + tactile analog-inspired controls. Target Windows x64 VST3, JUCE/C++20, FL Studio. No invention of completed work, no unsupported AI claims.

## Current baseline (verify against main before editing)
Six DSP families: Stabilizer, Dynamic EQ, Smart Dynamics, Stereo Architect, Spectral EQ, Limiter; one instance per family. Serial/Parallel, 720 orders, snapshot/migration schema 6 and 162 parameters. Limiter sample-peak with 5 ms PDC, NOT True Peak. Master Brain is heuristic; its proposals use real DSP preview and require Apply. No tested FL Studio integration claimed. See README.md, MODULAR_WORKFLOW.md, CHAIN_ROUTING_FIX.md, BRAIN_IMPROVEMENTS.md, MASTER_BRAIN.md. Historic docs may contradict latest code.

## Priority 0 — reproducible fault diagnosis
- Reproduce in standalone DSP tests and Windows FL Studio with mono, identical dual-mono, narrow stereo, wide stereo, anti-correlated bass, transients, broadband noise and music (rights-cleared).
- Verify GUI controls -> APVTS -> DSP settings -> chain present/enabled -> output including Serial vs Parallel and bypass.
- Measure at matched output loudness: signal delta, stereo width and correlation per band, magnitude/phase response, LUFS, peak/true peak, GR, THD+N, aliasing, CPU/RAM, block-size independence.
- In parallel routing, equal-weight 1/N averaging is intentional baseline but perceptually dilutes a single processed branch; design explicit per-branch gain and dry/wet with gain compensation and backwards-compatible defaults. Do NOT silently amplify everything.
- Add regression tests proving extreme settings cause significant nonzero signal differences on suitable input, while neutral settings null against latency-aligned dry.

## Phase 1 — fix six processors
1. Stereo Architect FIRST: current width operates only on existing Side (0–2x). Add selectable, controlled Stereoize/decorrelation for mono/narrow material, M/S per-band width, depth/space controls with safe band-aware microdelays where useful; stronger shaping without mandatory widening of subbass. Provide 4-band solo/delta, mono monitoring, continuous per-band correlation and explicit mono-compatibility warnings. Avoid phase cancellation, unintended spectral comb filtering and level jumps. Character presets Clean/Strong/Extreme are explicit controls, not hidden gain multipliers.
2. Smart Dynamics: wider expressive compression, punch/transients, parallel compression, meaningful analog saturation with optional oversampling; verify thresholds/knee/ratio and bypass. Distinguish dynamic transfer behavior from added harmonics.
3. Stabilizer: make current +/-6 dB per filter and default Amount 25% visible, increase safe intensity and tune adaptation/response while avoiding pumping, overcorrection and flat-spectrum forcing; compare objective before/after.
4. Dynamic EQ: improve detection, band controls, adjustable effective ranges and solo/delta; avoid excessive overshoot and unstable gain.
5. Spectral EQ: improve live editing, M/S and L/R options, filters and audition; linear phase only with latency/CPU design and validation.
6. Limiter/Maximizer: measured true-peak design, oversampled true-peak detection/limiting, adaptive release, selectable clipping/transient controls; do not label True Peak until intersample validation passes.

## Phase 2 — four new real DSP families
7. Harmonic Exciter/Enhancer: multiband even/odd harmonic profiles, drive, tone, input/wet-only selection as appropriate, M/S control, oversampling, level-compensated audition, true input/output harmonic delta and spectrum.
8. Analog Color: tube/tape/transformer/solid-state saturation models with drive, bias, dynamics, tone and blend; meaningful distinct transfer curves; defend against DC and aliasing.
9. Psychoacoustic Enhancer: bass definition, clarity, presence, air and spectral contrast using measurable phase-safe techniques; do not imply magical restoration of missing content.
10. Bass Architect: controlled subharmonic synthesis, punch/sustain, low-end mono and peak protection; low-frequency phase/translation measurement.

## Phase 3 — graph and state
- Stable module instance IDs, support duplicate instances and reordering; flexible serial/parallel routing with explicit branch trim/mix and PDC.
- No audio-thread allocation, locking, FFT, file I/O or ML inference; pre-allocate, crossfade safe graph swaps.
- Preserve existing 162 IDs/indexes, state migration from schema 1–6, presets .jmlx, existing FLP, latency/PDC, automation, Undo/Redo.
- Ensure limiter at end of full chain if user needs final ceiling; warn when other processors follow it or parallel branches bypass it.

## Phase 4 — Hybrid Studio UI (design concurrently, integrate incrementally)
- Dark high-contrast, elegant hybrid digital/analog visual language with consistent color coding for ten DSP families.
- Persistent top chain strip, drag and bypass state visibly separate, main workspace per module, global I/O meters and Master Brain.
- Stereo panel: live multiband width/correlation, vectorscope, M/S and mono A/B; harmonic module: spectrum and genuine added harmonic content; dynamics: GR histories; limiter: output/time history.
- Musical macros Punch/Warmth/Depth/Air/Presence/Density explicitly mapped to concrete automatable DSP, with direct editing of underlying parameters.
- Clean/Strong/Extreme modes modify declared parameter maps/profiles; preserve manual overrides and gain-match.
- 75–200% scale, HiDPI, keyboard and precision input, accessibility/contrast, 30/60 FPS bounded visualization, separate GUI analysis workers; no GUI dependency for audio output.

## Phase 5 — Master Brain AI 2.0
- Keep existing deterministic heuristic engine and actual DSP preview as an explainable fallback; distinguish it in UI from model-based AI.
- Add music feature extraction: section structure, energy, spectral shape, rhythm and transient features, dynamics, M/S statistics; no unvalidated claims to identify instruments/mood.
- A reference library of versioned statistical feature profiles by style (e.g. post-punk/darkwave, punk/rock, techno/industrial, ambient), not a single EQ curve. Support user-owned WAV/FLAC references with loudness matched analysis.
- Data provenance, rights and licensing for every audio sample / public corpus; not all Creative Commons material is commercial-use compatible; do not redistribute protected masters or scrape streaming services.
- Candidate generation of multiple creatively distinct strategies (Dark & Deep, Punch & Energy, Wide & Atmospheric, Raw & Vintage), render with REAL graph away from audio callback, evaluate loudness-matched metrics, rank while preserving transparent reports and audible A/B/C/D.
- Optional genuinely trained/embedded local ML models only after datasets, training/evaluation pipeline, model provenance, size, latency and quality are validated; use ONNX Runtime or equivalent only if justified. Never rebrand heuristics as ML.
- Learning preference/ranking from explicit accept/reject choices: opt-in, local, inspectable, resettable. No automatic destructive changes, Apply and Undo remain mandatory.
- Evaluate on independent rights-cleared diverse test songs, blind loudness-matched listening, quality/mono/phase/transient checks; one source track is not an objective mastering target.

## Implementation order / dependency and delivery
P0 tests and reproducible measurements -> Stereo fix + Hybrid UI design system -> remaining six-module strengthening -> new harmonic/analog processors -> psychoacoustic/bass -> graph/instances/routing refinement -> reference engine and candidate preview -> optional ML -> complete Win x64 VST3 + FL Studio test matrix.
Ship vertical slices with source, unit tests, audio/null/delta tests, screenshots and CI artifacts. No success claim without build and tests; CI is not proof of musical quality.

## Acceptance checklist
- Stereo mode creates noticeable (measured, loudness-matched) widening on supported mono/narrow material, while existing width behaves predictably on stereo; no dangerous mono collapse at safe defaults.
- Every active module exhibits parameter-to-DSP-to-output correspondence on appropriate stimulus; extreme settings do not silently do nothing.
- Neutral bypass is clean and latency-aligned, no clicks, NaNs, xruns or permanent audio loss.
- Double-instance/routing survives save/restore, program re-open, automation and Undo when delivered.
- True Peak only marked available after real intersample tests.
- AI suggestions remain editable, explainable, audio-preview validated and never applied without consent.
- Windows x64 VST3 build plus FL Studio playback, Piano Roll unrelated host workflow, save/reload, freeze/export WAV, GUI open/close and sustained stability tests.
- CPU and RAM measured against baseline and reviewed; no unbounded allocation on audio thread.

## Immediate Codex task
Start with P0 measurement harness and Stereo Architect reconstruction, but FIRST produce code-level audit and tests proving why low audibility occurs (including Parallel dilution, input Side scarcity, presence flags and smoothing). Introduce fixes in coherent changes; do not simultaneously redesign every module. Design GUI component system in parallel as specification/mock layout, not as decorative replacements for real DSP. Open a PR with test evidence, before/after matched-level WAV and documented behavioral changes.
