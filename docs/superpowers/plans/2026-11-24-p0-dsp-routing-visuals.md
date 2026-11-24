# MasterLabX P0, DSP, Routing and Visuals Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close verified P0 lifecycle and routing-bank defects, then improve measured Maximizer/Exciter/Stereo/Restoration behavior, benchmark CPU/RAM/PDC, and finish telemetry-backed module layouts.

**Architecture:** Preserve the current two-bank `SerialGraph`, fixed final true-peak stage, APVTS schema and no-STEMS scope. Establish lifecycle and bank-copy invariants first; every later DSP or GUI change is driven by a failing regression or measured evidence and keeps realtime callbacks allocation-free.

**Tech Stack:** C++20, JUCE, CMake/CTest, pluginval, GitHub Actions Windows x64.

**Spec:** `docs/MASTERLABX_EXECUTION_DIRECTIVE.md` and `docs/codex-handoff/2026-11-24/FINAL_MAXIMIZER_GUI_DSP_HANDOFF.md`

## Global Constraints

- No STEMS LIVE or OFFLINE.
- Do not implement the separate MASTER/INSERT profile stage here.
- Do not merge to `main` or publish a production release without explicit approval.
- No allocation, blocking, file/network access, or unbounded work in the audio callback.
- Do not claim FL Studio, pluginval, Windows, CPU, RAM, PDC, or true-peak results without direct evidence.

## Review Focus

- Host release/reprepare while Brain is analysing must stop CPU work and release analysis storage safely.
- Route changes after long-running stateful processing must not allocate, reset unrelated modules, or produce discontinuities.
- Nonlinear stages must remain finite and bounded under adversarial input and automation.
- Reported PDC must equal measured impulse delay at all supported sample rates and block sizes.
- Visualizations must use actual response telemetry and remain non-overlapping at 750×550, 1000×700 and 1400×1000.

---

### Task 1: Processor lifecycle and routing-bank P0

**Files:**
- Modify: `Source/MasterBrain.h`
- Modify: `Source/Processor.h`
- Modify: `Source/SerialGraph.h`
- Test: `tests/PluginTests.cpp`
- Test: `tests/SerialGraphTests.cpp`
- Test: `tests/AllocationTests.cpp`

**Interfaces:**
- Produces: an explicit non-realtime Brain release operation and test-visible bank-transition invariants.

- [x] Add failing release/reprepare and active-analysis resource regression; existing bank state/route and callback-allocation tests cover the confirmed copy path.
- [x] Run the focused test and record the expected failure.
- [x] Implement the smallest lifecycle fix without changing the parameter schema.
- [x] Run focused lifecycle, Brain, serial-bank and allocation tests; sanitizer coverage remains a later verification item.

### Task 2: Maximizer and Exciter safety/tuning

**Files:**
- Modify: `Source/FinalTruePeak.h`
- Modify: `Source/HarmonicExciter.h`
- Modify: `Source/AnalogExciter.h`
- Test: `tests/FinalTruePeakTests.cpp`
- Test: `tests/FinalTruePeakAdversarialTests.cpp`
- Test: `tests/MusicalExciterTests.cpp`

**Interfaces:**
- Consumes: lifecycle and bank invariants from Task 1.
- Produces: bounded nonlinear output, verified ceiling behavior and stable automation.

- [ ] Measure current overshoot, crest loss, THD/alias proxies and automation discontinuities.
- [ ] Add failing regressions only for violations demonstrated by measurements.
- [ ] Tune detector/envelope/saturation while retaining real parameter meanings and fixed PDC.
- [ ] Re-run differential, adversarial, multirate and integration tests; commit independently.

### Task 3: Stereo Architect and Restoration

**Files:**
- Modify: `Source/ModularDsp.h`
- Modify: `Source/AudioRestoration.h`
- Test: `tests/StereoTests.cpp`
- Test: `tests/RestorationTests.cpp`
- Test: `tests/StereoIntegrationTests.cpp`

**Interfaces:**
- Produces: mono-compatible stereo processing and artifact-bounded restoration with real telemetry.

- [ ] Measure correlation, mono null, channel symmetry, residual noise and transient damage.
- [ ] Add failing regressions for confirmed defects.
- [ ] Apply minimal DSP corrections and verify four sample rates, odd block sizes and automation.
- [ ] Commit independently.

### Task 4: CPU, RAM and PDC evidence

**Files:**
- Modify: `tests/ModularBenchmark.cpp`
- Modify: `tests/FinalTruePeakBenchmark.cpp`
- Create or modify: `tests/LifecycleBenchmark.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: Tasks 1–3 implementations.
- Produces: machine-readable per-topology CPU/RAM/PDC evidence.

- [ ] Add serial, parallel, bypass, Brain-idle/active and route-transition cases.
- [ ] Measure rather than infer memory and impulse latency.
- [ ] Record machine/compiler/sample-rate/block-size with every result.
- [ ] Run locally and in Windows CI; commit independently.

### Task 5: Remaining telemetry-backed GUI geometry

**Files:**
- Modify: relevant `Source/*Panel.h` files after screenshot audit.
- Modify: `tests/GuiSmoke.h`

**Interfaces:**
- Consumes: actual readings produced by Tasks 2–3.
- Produces: responsive layouts and screenshots at the three required viewport sizes.

- [ ] Capture current panels and list concrete clipping, hierarchy and information defects.
- [ ] Add failing geometry/visibility checks for each confirmed defect.
- [ ] Re-layout panels without bitmap mockups or invented telemetry.
- [ ] Visually inspect all captures, run GUI smoke and commit independently.

### Task 6: Full verification and handoff

**Files:**
- Modify: evidence documentation only after tests produce results.

- [ ] Run complete local build and CTest.
- [ ] Push the working branch only and monitor the exact Windows SHA.
- [ ] Verify Windows x64 build, DSP, GUI, pluginval and artifacts.
- [ ] Report completed, unverified and remaining work without merging `main`.
