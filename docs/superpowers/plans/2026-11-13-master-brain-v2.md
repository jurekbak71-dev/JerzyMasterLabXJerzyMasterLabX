# Master Brain V2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build one verified automatic mastering chain from at most 20 seconds while imposing no analysis CPU or heavy-memory cost when Brain is idle.

**Architecture:** Separate feature extraction, target policy, proposal construction and bounded analysis-by-synthesis scoring. The worker owns heavy buffers only during a job and publishes a small immutable result for the editor.

**Tech Stack:** C++20, JUCE VST3/APVTS, existing `SerialGraph`, CMake/CTest, Python/Pillow GUI assets.

**Spec:** `docs/superpowers/specs/2026-11-13-master-brain-v2-design.md`

## Global Constraints

- Maximum capture is 20 seconds.
- One result; manual Genre/Character; automatic LUFS/dBTP.
- Never address the retired legacy limiter.
- No audio-thread allocation, lock, FFT or trial render.
- Heavy job memory is released whenever Brain is not working.
- No merge to `main`; no completion claim without exact tests.

## Review Focus

- Silence or too-short input disables Apply without changing parameters — Task 2.
- 192 kHz capture stays bounded and releases memory — Task 1.
- Anti-phase low end cannot trigger widening — Task 2.
- Loud clipped input cannot be made “better” by more drive — Task 3.
- Cancel/restart generations cannot publish stale proposals — Task 1.

---

### Task 1: Bounded job lifecycle and idle resource contract

**Files:** Modify `Source/MasterBrain.h`; test `tests/BrainTests.cpp`, `tests/AllocationTests.cpp`.

**Interfaces:** Produce `BrainResourceReading MasterBrain::resourceReadingUI()`, job-owned capture storage and phase `Optimising`.

- [x] Write failing tests for 20-second clamp, cancellation generation, heavy bytes released in Idle/Ready/Cancelled/Failed and no idle analysis counters.
- [x] Run `brain_tests` and `allocation_tests`; confirm expected failures.
- [x] Move heavy capture/FFT/render ownership into the active job, publish only compact features/proposal, and block the worker instead of doing idle DSP work.
- [x] Re-run targeted tests; commit `Bound Master Brain jobs and idle resources`.

### Task 2: Current-parameter chain builder and automatic targets

**Files:** Create `Source/BrainTargets.h`, `Source/BrainChainBuilder.h`; modify `Source/BrainRules.h`, `Source/Processor.h`; test `tests/BrainRulesTests.cpp`, `tests/BrainTests.cpp`.

**Interfaces:** Produce `BrainTargets deriveBrainTargets(const BrainFeatures&, const BrainConfig&)` and `BrainProposal buildBrainChain(...)` using current APVTS IDs only.

- [ ] Write failing tests proving automatic targets are bounded/material-aware and proposals use `truePeak*` but never retired limiter IDs.
- [ ] Run tests and confirm the legacy-ID regression fails.
- [ ] Implement target policy and measured module selection with shared tonal/stereo/dynamics budgets.
- [ ] Re-run targeted tests; commit `Build current Master Brain chain decisions`.

### Task 3: Analysis-by-synthesis optimiser

**Files:** Create `Source/BrainOptimizer.h`; modify `Source/BrainPreview.h`, `Source/MasterBrain.h`; test `tests/BrainPreviewParityTests.cpp`, `tests/BrainAnalysisTests.cpp`.

**Interfaces:** Produce `BrainScore scoreBrainRender(...)` and `BrainOptimizationResult optimizeBrainChain(...)`, maximum six trials, best-valid retention.

- [ ] Add failing fixtures for already-loud, bass-heavy, harsh, transient, clipped and anti-phase fragments; assert finite output, dBTP bound, crest/GR limits and non-worsening score.
- [ ] Run targeted tests and capture RED evidence.
- [ ] Render the exact captured fragment through the real graph, measure it, adjust bounded parameters and stop after convergence/non-improvement.
- [ ] Verify deterministic results and callback partition invariance; commit `Optimize Master Brain through actual DSP`.

### Task 4: Premium Brain workspace

**Files:** Create `Source/MasterBrainPanel.h`; modify `Source/Editor.h`, `Source/StudioTheme.h`, `CMakeLists.txt`, GUI asset scripts; test `tests/GuiSmoke.h`.

**Interfaces:** Consume compact snapshot targets, score, iteration, issues and ordered module decisions; no audio buffers.

- [ ] Extend GUI smoke first for absence of manual target/ceiling and A/B, phase/status IDs, module cards, current controls and geometry at 750/1000/2000.
- [ ] Run GUI smoke and confirm RED.
- [ ] Render separate surfaces and implement the dark mastering workspace with real telemetry and accessible controls.
- [ ] Inspect actual JUCE screenshots, fix hierarchy/overlap, rerun smoke; commit `Rebuild Master Brain mastering workspace`.

### Task 5: Regression and exact-SHA Windows package

**Files:** Update evidence docs/workflow only if required by observed failures.

- [ ] Run complete core and plugin CTest suites, GUI smoke, pluginval, CPU/allocation benchmarks and `git diff --check`.
- [ ] Record exact measurements and inspect screenshots.
- [ ] Push only the feature branch and require Windows x64 build, CTest, GUI, pluginval and package success for the exact SHA.
- [ ] Report verified/unverified/unfinished work; do not merge `main`.
