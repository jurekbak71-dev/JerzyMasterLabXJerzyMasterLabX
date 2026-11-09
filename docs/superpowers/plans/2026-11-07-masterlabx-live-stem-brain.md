# MasterLabX Live Stem Brain — execution plan (2026-11-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace file-only stem playback as the primary mode with genuine live separation of the audio passing through MasterLabX in FL Studio, then add per-stem DSP, interactive EQ and Master Brain module orchestration without breaking old sessions.

**Architecture:** The existing source (`Source/StemBrainPanel.h`) launches a native offline `stems-split.exe`; `Source/StemRemix.h` replays four WAV buffers aligned to DAW position. This is not live separation. Add an independent, bounded, worker-driven input/output stream separator with a lock-free callback boundary, model adapter and explicit latency/fallback modes. Use measurable feasibility gates before selecting the neural backend.

**Tech stack:** C++20, JUCE 8.0.6, CMake, Windows x64 VST3, GitHub Actions, native model runtime, CTest, pluginval; FL Studio host acceptance.

**Spec:** User instructions in conversation on 2026-11-07: live separation/remix, per-stem DSP, mouse-editable EQ, manual/automatic Master Brain processor selection, +15% useful control authority, researched modern mastering/restoration, installable VST3.

## Verified baseline and prohibitions
- Baseline PR #18 head `c11af88bb791b37a19fb3df4c29631ad9282a32f`, CI Windows #566 succeeded for OFFLINE HTDemucs.
- `Source/StemRemix.h` uses preloaded WAV; `Source/StemBrainPanel.h` launches offline helper. The live function does not yet exist.
- Keep plugin identifiers, existing APVTS parameter IDs and preset migration; do not silently alter automation parameter meanings or default signal path.
- Worker inference/file operations must not execute on the callback; bounded queue, no callback allocation/locks/waiting.
- No filter-band "stem splitting" marketed as neural source separation.
- Keep existing offline WAV capability as an alternate mode; avoid corrupting running FL Studio projects.
- Do not merge or declare host compatibility without user FL Studio test. Preserve previous usable package.

## Ownership model
- **ChatGPT:** source-grounded DSP research, model feasibility matrix, cross-module architecture decisions, test-oracle design, independent code/CI evidence review, Notion record.
- **Codex:** edit and compile JUCE C++ in an actual checkout, red/green tests, Windows CI and VST3 packaging, instrumentation, performance profiling, bug fixes.
- **Shared checkpoints:** objective pass/fail evidence, ABI/preset review and audio blind/matched-level listening.
- No Codex execution capability is exposed in this chat session. The plan is a durable handoff; creating this plan is not executing the code implementation.

## Review focus / five non-happy paths
1. Short/zero-length/inconsistent blocks and variable host sample rates: no crash, no invalid read, silence or deterministic fallback.
2. Stop/seek/loop and FL Studio offline render: no stale output or unbounded worker backlog.
3. Worker under-runs / model slow / model missing: dry fallback with smooth crossfade and explicit diagnostic; no audio-thread stall.
4. Channel layout changes/mono input, silence, high peaks: deterministic routing, no NaN, no unexpected gain doubling.
5. Old state and automation: reload previous v16/v17 session without changed IDs or loudness.

## Work package A — live separator feasibility / first working audio slice
**Files to inspect:** `Source/Processor.h`, `Source/StemRemix.h`, `Source/StemBrainPanel.h`, `Source/ModuleParameters.h`, `Source/StateValidation.h`, `tests/PluginTests.cpp`, `CMakeLists.txt`, `.github/workflows/windows.yml`.
**Create after reconciling existing architecture:** `Source/LiveStemEngine.h`, `Source/LiveStemEngine.cpp`, `Source/LiveStemBackend.h`, `tests/LiveStemEngineTests.cpp`, `tools/live_stem_benchmark.cpp`.

### Task A1: callback-safe buffering, bypass and timebase
- [x] Capture actual `processBlock` entry/exit graph and `prepareToPlay` / transport handling; document exact points in code.
- [x] Write failing tests for variable-sized audio blocks, mono/stereo, seek/loop, start/stop, worker starvation and zero input; prove RED.
- [x] Implement fixed-capacity ring input/output with sample-based timestamps, discontinuity generation counter and no heap allocation/lock in callback. Expose interfaces that do not allocate on the audio thread.
- [x] Verify GREEN under sanitizers where supported; record peak callback time and allocations.
- [x] Commit separately.

### Task A2: neural backend feasibility gate
- [ ] Benchmark current bundled HTDemucs GGUF/F16 native runtime in sliding-window mode: 44.1/48 kHz, CPU-only, CPU multi-core, GPU if explicitly supported; measure algorithmic lookahead, end-to-end latency, throughput (real-time factor), transient quality and reconstruction residual.
- [ ] Compare streaming HTDemucs/SCNet approaches and a genuinely causal lightweight architecture; verify model license and distributability, not just code license.
- [ ] Select backend only after benchmark. A 23 ms STFT hop in a paper is NOT proof of 23 ms usable host latency.
- [ ] Implement asynchronous backend and bounded crossfaded original fallback. If no CPU backend meets stable real-time execution, do NOT label the CPU path LIVE; provide measurable supported configuration and honest high-quality/offline mode.
- [ ] Test actual separation of a known multitrack reference and identity/reconstruction metrics, not a synthetic frequency splitter.
- [ ] Commit separately.

### Task A3: host integration and persistent modes
- [ ] Add new distinct APVTS IDs for Live/Offline/Original, do not repurpose `stemMode` legacy semantics; register fixed IDs once.
- [ ] Wire live engine input to the actual incoming master bus before existing processing, not to an imported song.
- [ ] Publish stable reported latency via JUCE `setLatencySamples` at suitable lifecycle points; test latency compensation during playback and FL Studio export.
- [ ] Validate old preset migration, offline WAV mode and user-controlled fallback.
- [ ] Commit with updated manual and CI evidence.

## Work package B — per-stem DSP and remix
**Inspect:** `Source/Processor.h`, `Source/StemRemix.h`, `Source/ModuleParameters.h`, existing module graph and `tests/ModuleIntegrationTests.cpp`.
**Potential creates:** `Source/StemProcessorGraph.h`, `tests/StemProcessorGraphTests.cpp`.
- [ ] Failing tests: changing only bass EQ leaves other isolated stems unchanged; mute/solo, phase and parallel gain are correct.
- [ ] Implement four independent, bounded DSP chains with gain/pan, parametric and dynamic EQ, dynamics/transients, saturation, stereo/M-S, meters. Reuse tested DSP primitives where feasible.
- [ ] Smooth parameter changes; verify A/B level-matched output, phase and sum-vs-original difference for each backend.
- [ ] Expose deterministic host automation with fixed parameter IDs and session recovery.
- [ ] Commit with sound examples and measurement CSVs.

## Work package C — mouse-editable dynamic EQ
**Inspect:** existing EQ widget inside `Source/Editor.h`, `Source/Processor.h` and parameter table.
- [ ] Define fixed maximum host-automatable EQ slots, each with stable enabled/frequency/gain/Q/type; all new slots disabled on migrated legacy presets. Do not alter existing first 16.
- [ ] Write failing GUI/input tests: left-click blank chart creates a slot; right-click on node deletes; drag changes freq/gain; wheel changes Q; no unintended right-click popup deletion in blank area.
- [ ] Add node hit-testing, drag/wheel gesture and selection status; GUI frequency curve must follow actual DSP coefficients.
- [ ] Test peak CPU, numerical stability, extreme Q, crossovers, automation save/load and exact compatibility with existing host snapshots.
- [ ] Commit separately.

## Work package D — Master Brain process selection
**Inspect:** `Source/MasterBrain.h`, `Source/BrainRules.h`, `Source/BrainPreview.h`, `Source/ModuleParameters.h`, `tests/BrainTests.cpp`.
- [ ] Build an explicit registry of existing internal modules with side effects, routes, measured preconditions and confidence.
- [ ] MANUAL: only user-selected processors are available for suggestion; locked parameters remain unchanged.
- [ ] AUTO: select minimal necessary modules from measured audio defects; preview, matched loudness and true Apply/Undo must be audible and verified.
- [ ] HYBRID: allowlist plus automatic settings; negative tests for stale proposals, blocked controls, unavailable modules and undo.
- [ ] Treat hosting arbitrary external VST3 as a separate feasibility/host-stability work package; do not claim universal third-party VST control without an actual nested host.
- [ ] Commit per independently verified behavior.

## Work package E — +15% authority and mastering/restoration
- [ ] Inventory every module parameter: old min/max, mapping and DSP output; record baseline measurements (gain-matched!).
- [ ] Implement >=15% larger *safe useful adjustment authority* where meaningful, not blanket multiplying gain/dB/ratio; preserve prior automation response semantics by adding new IDs or opt-in extended mode if necessary.
- [ ] Compare modern restoration methods (de-click, de-clip, denoise, de-reverb), dynamic tonal balance, multiband compression, true peak and spectral repair; distinguish provable implementation from research-only candidates.
- [ ] Implement selected modules with tests for silence, crest factor, true peak, LUFS, stereo compatibility and aliasing.
- [ ] Do not hardcode a single music LUFS target; measurement uses ITU-R BS.1770-5 and EBU R128 where appropriate.
- [ ] Commit per processor, with objective A/B audio evidence.

## Work package F — Windows installation and acceptance
- [ ] Run CTest full suite, pluginval strictness 5, Steinberg validator if available, GUI smoke, no-callback-allocation checks; attach logs.
- [ ] Exercise 44.1/48/96 kHz, several FL Studio buffers, host start/seek/loop, automation, old project load, offline render and missing-model fallback.
- [ ] Package x64 VST3, native runtime/models and double-click installer; include licenses, SHA-256, rollback path and performance matrix.
- [ ] Verify ZIP actually contains the VST3 and installer; download artifact and inspect.
- [ ] User FL Studio audition is a required acceptance gate, not inferable from green CI.

## Tooling actually available in ChatGPT
- Connected GitHub: inspect source, work with branches, PRs and Actions.
- Connected Notion: durable project decisions, current state and source links.
- Context7: live technical docs for libraries/APIs.
- Superpowers skills: planning, TDD, systematic debugging, review and verification.
- Local Python/container: bench prototypes and inspect artifacts; not a replacement for Windows/FL Studio.
- A direct Codex agent-dispatch tool is **not** exposed in this chat. Run Codex in its coding environment against this branch and this file; then review results against CI evidence.

## Primary authoritative references
- JUCE AudioProcessor latency / block contract: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html
- VST3 parameter IDs/automation: https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Parameters%2BAutomation/Index.html
- 2026 streaming offline separator research: https://arxiv.org/abs/2609.35397
- Low power real-time feasibility: https://arxiv.org/abs/2609.12201
- ITU-R BS.1770-5: https://www.itu.int/rec/R-REC-BS.1770-5-202311-I/en
- EBU R128: https://tech.ebu.ch/publications/r128

## Status
**2026-11-07:** research and architecture audit started; this plan documents intended work only. No new live streaming DSP implementation or passing tests are claimed.

## Execution checkpoint (2026-11-07)
A1 implemented in 6e42247, with Linux ASan/UBSan and zero-allocation tests;
A2 measurement harness in c5e4e21. Windows code commit c5e4e21 passes 45/45 CTest,
GUI, pluginval 5, neural smoke and packaging (run 42387405624). See
`docs/testing/2026-11-07-live-stem-a1.md` and
`docs/testing/2026-11-07-live-stem-a2.md` for exact behavior, limitations and
raw evidence. A2 backend selection remains open: native full-context HTDemucs
requires 27.71–48.34 s for 2 s audio on measured Windows configuration. A3/B–F are
not complete. No FL Studio test or live installer is claimed.
