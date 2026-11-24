# MasterLabX — Final Maximizer: audited Codex continuation (2026-11-24)

**STATUS: WIP / DRAFT; NOT PRODUCTION-APPROVED.** This document records code and evidence, not a claim that a rendered mockup works.

## 0. Source of truth; exact repository and commits

- Repository: https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX
- **CONTINUE FROM** branch: `fix/masterlabx-maximizer-gui-dsp-20261124`
- **CURRENT DRAFT PR**: https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/pull/26
- **DO NOT REBASE ON an earlier STEMS branch, PR #22 or `main`.**
- Parent branch = `feat/masterlabx-final-maximizer-lookahead-oversampling-20261124` / PR #25.
- Parent's VERIFIED Windows x64 commit: `a3c5fc9b8831a6933b22cfb21107eb242121f830`, [CI #1181](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45333541864), success (45/45 CTest, JUCE GUI screenshot, pluginval, SHA-verified installer).
- Last CI-tested source commit before this documentation update: `44f1466b836ecede913848b04ed08dfca75530fc`. Any new handoff/document commit advances HEAD. Codex must check `git rev-parse HEAD` and matching CI before claiming that new SHA tested.
- **LATEST CODE CI #1202**: https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45366278956 SUCCESS for code SHA 44f1466...; 46/46 tests, GUI JUCE screenshots, pluginval, package SHA-256. Visual reference fidelity FAILS despite green technical checks. Prior CI #1190 FAILED at CMake graphics validation (nontransparent scope centre) and was subsequently corrected.
- Workstation for original user acceptance: Windows x64 / FL Studio. Local container cannot fetch the private repo using git because DNS `github.com` fails; work here uses the actual GitHub connector and remote CI.

## 1. Mandatory prior decisions, no unauthorized scope change

Read, in order:
1. `AGENTS.md` — project-specific agent rules. **No STEMS or source separation**, no mock DSP, no hidden degradation of routing/mastering. Do not merge or publish without explicit approval.
2. `docs/MASTERLABX_EXECUTION_DIRECTIVE.md` v1.2 — binding scope.
3. `docs/phase5/CONCEPT_C_POWDERCOAT_BUILD_SPEC.md` — GUI contract; approved 2026-11-06. Some historical statements (old 128-sample PDC) have been superseded by later PR #25: **inspect actual code and acceptance dates**; never silently restore old values.
4. Notion: [MasterLabX — approved GUI Concept C, ongoing audit and Maximizer](https://app.notion.com/p/4105fed1901b810e9472e11439017087).
5. Notion operating guide approved 2026-10-29: https://app.notion.com/p/4085fed1901b817db779c41c0375f6bc .
6. **LATEST USER-SELECTED screenshot**: `image(20261124-091158).png`, the Ozone Maximizer visual and functional reference explicitly provided in chat. It supersedes the older `image(20261110-200811).png`. It is a benchmark, not licensed code or graphics to copy verbatim; the user will supply the Codex handoff ZIP containing the exact screenshot.
7. Two 1672x941 **ImageGen concept renders** were produced in chat for CORE and ADVANCED. Their exact names in the separate user-delivered image ZIP are `maximizer_core_concept_1672x941.png` and `maximizer_advanced_concept_1672x941.png`. They are not source code or actual JUCE screenshots; fake demo metrics, unimplemented upward-compression/stereo-independence controls and preset state MUST NOT be presented as functioning. Reproducible static asset kit described below IS actual code.

## 2. Confirmed baseline functional scope

- Core maximizer DSP: `Source/FinalTruePeak.h`. Actual parameters: ceilingDb, releaseMs, driveDb, character (Transparent/Punch/Aggressive), autoRelease, softClip, outputDb, lookaheadMs, attackMs, transientEmphasis, 1×/2×/4×/8× **true-peak detection factor**. 10 ms maximum lookahead buffer and constant host latency **for each sample rate**; inspect source before changing. This is **NOT** iZotope IRC5; don't copy proprietary algorithms.
- Host parameter IDs and mapping: `Source/Processor.h`, `Source/ModuleParameters.h`, `Source/GraphConfiguration.h`. The four additional advanced parameters are appended after 316 existing IDs, indices 316–319. Preserve all IDs and values on existing sessions.
- Main GUI: `Source/Editor.h` includes `Source/FinalTruePeakPanel.h` panel; `Source/StudioTheme.h` loads previous 101-frame rendered Concept C knobs/LEDs. Do not replace APVTS controls with drawn mock controls.
- Test and package pipeline: `.github/workflows/windows.yml`, Windows 2022 ClangCL Release, CTest, adversarial independent true peak, GUI smoke, pluginval strictness 5, installer with SHA-256 and rollback.
- Before this PR, 8×/49-tap adversarial suite ran 96 cases, highest measured excess +0.0491 dB above target under a +0.05 dB acceptance tolerance. **No absolute true-peak certification.**

## 3. What THIS BRANCH changed (actually written code)

| Source path | Recorded change | Verifiable state |
|---|---|---|
| `Source/FinalTruePeakPanel.h` | Replaced collision-prone CORE/ADVANCED coordinates with adaptive grid, dedicated meter column, clipped DSP waveform, own telemetry footer, and selected settings clearly distinguished from last processed audio. Embedded generated bezel and scope frame; APVTS attachments and current DSP unchanged. | Code committed and screenshot-captured CI #1202; visual fidelity NOT approved |
| `tools/gui_assets/render_maximizer.py` | **Deterministic Pillow renderer** for `maximizer_meter_bezel.png` (512×396 RGBA) and `maximizer_scope_frame.png` (1600×470 RGBA). Metallic geometry is static only; NO fake needle, waveform, meter, or LED status. | Source committed; CI #1202 validated |
| `tools/gui_assets/validate_maximizer.py` | Check RGBA/dimensions, manifest SHA-256 and transparent scope center. | Registered CTest passed CI #1202 |
| `CMakeLists.txt` | Render, validate, embed both PNG assets with `juce_add_binary_data`; new `maximizer_asset_pack` CTest. | Source committed; CI pending |
| `tests/GuiSmoke.h` | Require actual render assets; capture CORE and ADVANCED at 750×550, 1000×700, 1400×1000; require useful advanced slider height; keep old `limiter-smoke.png`. | CI #1202 green; visual target NOT met |
| `.github/workflows/windows.yml` | Upload screenshot names `maximizer-core-*.png`, `maximizer-advanced-*.png` in GUI and build-log artifacts. | CI #1202 green; visual target NOT met |

**Important**: the old baked assets already in Concept C remain. The new PNGs are built from deterministic geometry, not imported ImageGen pixels. The freshly generated images depict **static hardware elements only**, while the GUI draws GR/TP/waves from `FinalTruePeakReading`.

## 4. GUI acceptance checks — run against real JUCE screenshots, not a design image

1. Export and VISUALLY inspect both `CORE` and `ADVANCED` at **750×550**, **1000×700**, **1400×1000**. Screenshots saved in CI artifact `Concept-C-JUCE-GUI-screenshots`. Reject any overlap, cropped label, hidden knob, unreadable control, vanishing LED, or misplaced VU.
2. At CORE 1000×700 ensure `truePeakSoftClip` knob and value never enter the meter rectangle. Meter should occupy a dedicated right column; all five primary knobs must be visible.
3. On ADVANCED, `truePeakOversampling` is a real ComboBox and is labeled **TP DETECTION**, not FULL OVERSAMPLING. No non-existent oversampling control.
4. The analog GR needle must move from `reading_.gainReductionDb`, zero meaning no GR; the cyan/orange/red waveform is `reading_.inputWave/outputWave/gainEnvelope`. Static bezel/frame contain NO readings.
5. Changing lookahead/8× must update the *SELECTED* values immediately; `LAST PROCESSED` may change only after a real audio callback. Never pass stale DSP telemetry as current selection.
6. Test APVTS mouse drag, wheel, text entry, keyboard, preset state save/load, automation replay, parameter 0/1 limits, GUI reopen, Windows 100%/150% scaling, host PDC; automated screenshot snapshot alone does NOT prove these.
7. At minimum distinguish true peak estimate from sample peak, show correct units, ensure no accidental clipping or scroll bar obstruction.
8. Verify that both skin and small-control sprites are sourced from *real embedded* images, correctly decoded and not missing at 2× HiDPI.
9. Stay consistent with Concept C dark inset display, metallic controls, LED semantics and user-provided Maximizer screenshot; don't randomly redesign all other panels.

## 5. DSP issues STILL UNSOLVED — do not claim quality acceptance

- Current `oversamplingFactor` is true-peak FIR detector/reconstruction, **not full nonlinear soft-clip path oversampling** with polyphase anti-alias upsample/downsample. This is a remaining implementation task, likely requiring carefully sized preallocated buffers and latency management.
- Character modes need more robust subjective/quantitative dynamics tuning on real mixes: attack/release, adaptive release pumping, transient preservation, distortion and aliasing, sound quality compared on loudness-matched material; do NOT call it IRC5 or identical to Ozone.
- Measure 1×/2×/4×/8× detector safety at 44.1/48/96/192 kHz, stress ceiling across transient shapes and PDC. Do not lower ceiling silently or falsify meter.
- Conduct nonlinear THD/IMD and aliasing measurements, stress CPU and no audio-thread allocations/locks, gain-reduction correlation, output dBTP, LUFS on real mixes.
- User's earlier reported Master Brain/Analog Exciter sonic problems are not automatically fixed by this Maximizer GUI work; retain PR #23/#24 context, don't mix unrelated changes silently.

## 6. First Codex actions — exact Windows commands and evidence gates

Use this branch only:
```powershell
git clone https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX.git
cd JerzyMasterLabXJerzyMasterLabX
git fetch origin
git checkout fix/masterlabx-maximizer-gui-dsp-20261124
git status --short
git rev-parse HEAD
python -m pip install pillow==12.0.0 psutil==7.1.3
$py = python -c "import sys; print(sys.executable)"
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -T ClangCL "-DPython3_EXECUTABLE=$py"
cmake --build build --config Release --parallel 1
ctest --test-dir build -C Release --output-on-failure
./build/Release/plugin_tests.exe --gui
```
`pluginval` and package commands are authoritatively in `.github/workflows/windows.yml`. Do not invent a passed local run from a green CI on a **different commit**. On Linux without the Windows JUCE SDK/host, local output cannot replace Windows VST3/FL Studio acceptance.

Read fresh [PR #26](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/pull/26) checks and [CI #1190](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45364424047) first. Inspect the **real** PNGs. Fix root causes, repeat test and CI. If a run is canceled due to a newer push, verify latest HEAD and run together.

## 7. Acceptance / release policy

A handoff is NOT a release. Declare finished only after: (A) no compile/test/pluginval regression, (B) all six GUI screenshots visually audited and approved, (C) static asset hashes pass and actual PNGs are embedded, (D) host automation/PDC/preset test in FL Studio, (E) audio-quality evaluation with loudness matching, (F) rollback-capable installer verified on the exact commit. Record each with path, SHA, test and date.

The result of this branch is a **UI-focused in-progress change**. Do not claim full 8× nonlinear oversampling, Ozone algorithm equivalence, original 11-file Analog Luxury asset integration, automatic mastering quality or Windows end-user acceptance.

## 8. Reproducible graphics, not a flat mock

```powershell
python tools/gui_assets/render_maximizer.py build/concept_c_gui
python tools/gui_assets/validate_maximizer.py build/concept_c_gui
```
Outputs and dimensions:
- `build/concept_c_gui/maximizer_meter_bezel.png` — 512×396 RGBA, static analog metallic enclosure and face; needle/scale drawn by JUCE over it.
- `build/concept_c_gui/maximizer_scope_frame.png` — 1600×470 RGBA, transparent central hole; waveform visible through aperture.
- `build/concept_c_gui/maximizer_assets_manifest.json` — SHA-256 and generator provenance.
Both PNGs are embedded via `MasterLabGuiAssets` during CMake generation. Preserve source script to keep asset re-renders deterministic. A separate full-panel *concept* render from ImageGen is not a usable knob atlas or functional GUI.

## 9. Report format expected of Codex

- **WYKONANO** — files/commit SHAs and completed operations;
- **ZWERYFIKOWANO** — code evidence, screenshots and sources;
- **TESTY** — exact commands, environments, counts, errors and CI links;
- **NIEZWERYFIKOWANE** — unknown host/visual/audio results;
- **NIEWYKONANE** — prioritized remaining steps;
- **ŹRÓDŁA** — Notion docs, GitHub commit, issue/PR, CI artifacts, JUCE official Graphics API https://docs.juce.com/develop/classjuce_1_1Graphics.html .

**Do not create another parallel stale branch, merge, or publish a user-facing production version without permission.**

## 10. Latest user reference, real GUI audit and exact Codex continuation — 2026-11-24

**STATE: IMPLEMENTATION STILL INCOMPLETE.** The full-panel ImageGen renders are visually ambitious proposals, not real working JUCE. The user specifically selected a screenshot of Ozone Maximizer as the desired structure and comparable behaviour. The primary visual source is the attached user-supplied file image(20261124-091158).png. No Ozone branding or proprietary code/assets should be copied.

**Current code:** PR #26, branch fix/masterlabx-maximizer-gui-dsp-20261124, source commit 44f1466b836ecede913848b04ed08dfca75530fc before this documentation update, parent PR #25 source a3c5fc9b8831a6933b22cfb21107eb242121f830. Preserve 320 APVTS parameter IDs, no STEMS, processor routing, saved presets, PDC, FL Studio Windows VST3 compatibility, source code and history. A document-only commit advances branch HEAD again; check the exact SHA of each subsequent CI run.

**PROVEN TEST RESULTS:** Windows CI #1202 https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45366278956 completed SUCCESS with Release ClangCL x64, 46/46 CTest, real JUCE GUI smoke, pluginval strictness 5, installer ZIP SHA-256 and asset checks. Independent 8x/49-tap 96-case stress test reported worst +0.0491 dB above ceiling within its own +0.05 dB tolerance only. No certification. Prior CI #1190 https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45364424047 FAILED at CMake configure because the earlier scope-frame centre was not transparent; this was fixed. Do NOT describe #1190 as success.

**ACTUAL GUI VISUAL REVIEW of CI #1202:** Download https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45366278956/artifacts/14907840714. I inspected maximizer-core-1000x700.png, maximizer-advanced-1000x700.png and maximizer-core-750x550.png. The current CORE presents five equal small knobs in one row, a small GR analog meter to their right and a low ~110 px waveform strip. The current ADVANCED presents three knobs and a TP ComboBox with the same small scope. This does NOT match the selected reference: a large left Gain/Drive knob and ceiling/character, a big central real gain-reduction/waveform history, a right bank of real input/output/GR/true-peak/LUFS meters, and clearly grouped processing controls below. Actual GUI is still visibly cramped and information-poor, regardless of green CTest. The 750px view shrinks the scope to a thin stripe and omits the analog GR meter. The current CI artifact has CORE images for 750x550 / 1000x700 / 2000x1400 and ADVANCED for 750x550 / 1000x700 / 1400x1000; CORE 1400x1000 capture is missing and MUST be added. Do not say six requested viewport variants were visually verified.

**STATIC 3D/PNG ASSETS VERIFIED, but not the full GUI:** CI #1202 artifact https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45366278956/artifacts/14906817192 includes maximizer_meter_bezel.png (512x396 RGBA) and maximizer_scope_frame.png (1600x470 RGBA), and maximizer_assets_manifest.json with matching hashes. ZIP CRC checked; 1600x470 scope centre alpha=0. Deterministic generator tools/gui_assets/render_maximizer.py and validator tools/gui_assets/validate_maximizer.py are committed. These are static chassis elements only; no fake telemetry, waveforms, needles, LEDs or meter values are baked into them.

**WHAT CODE REALLY SUPPORTS NOW:** Source/FinalTruePeak.h + FinalTruePeakPanel.h expose real truePeakDrive (Gain), truePeakCeiling, truePeakRelease, truePeakOutput, truePeakSoftClip, truePeakCharacter, truePeakAutoRelease, truePeakLookahead, truePeakAttack, truePeakTransient, truePeakOversampling (TP detection 1x/2x/4x/8x). FinalTruePeakReading stores detectedTruePeak, outputSamplePeak, gainReductionDb, 64-point inputWave/outputWave/gainEnvelope, outputTruePeak, lookaheadMs, ceilingLinear, active, oversamplingFactor. These are not evidence of true integrated/short-term LUFS, distinct L/R input/output peak history, 10-second GR scrolling timeline, Stereo Independence, Upward Compress, real stereo mid-side modes or user-settable 10-second Zoom. Validate existence in other processors before implementing, and NEVER show fabricated values or inert knobs.

**CODEx NEXT STEPS (not yet done):**
1. Open user screenshot and both concept PNGs from the separate downloaded handoff ZIP; do not infer their pixels from these words. Inspect actual screenshots from CI #1202, compare hierarchies, present implementation plan grounded in code. Use user's screenshot as functional/structural reference, not direct asset clone.
2. Build the real responsive JUCE maximizer layout with large main Gain, separate ceiling and character controls, big DSP history graph, right TRUE input/output/GR/TP/LUFS meters backed by actual DSP telemetry, and lower grouped controls for available real parameters. Keep MasterLabX Concept C chassis, metal sprites and LED semantics. Use additional parameters ONLY when a real functioning DSP algorithm is implemented and migration is testable; otherwise omit proposals.
3. A short 64-point waveform in FinalTruePeakReading is not a 10-second history. If adding scrolling GR, write bounded preallocated audio-to-GUI ring with real timebase/zoom and correct scales, no allocations or locks on audio thread. If input/output stereo meters and LUFS lack authoritative measurements, implement true measurement or leave them absent and mark feature incomplete.
4. Support accessible controls and readable responsive pages at CORE and ADVANCED 750x550,1000x700,1400x1000. Add CORE 1400x1000 screenshot to CI, inspect actual output at 100/150/200% Windows DPI, no cutoffs/overlap, all APVTS controls keyboard/mouse/automation/preset-save/load tested, stable latency/PDC and pluginval.
5. DSP unresolved separately: 8x is true-peak reconstruction DETECTION, not full nonlinear anti-aliased soft-clip oversampling; no equivalence to iZotope IRC algorithm or formal quality acceptance. Do not alter DSP silently while implementing GUI. Finish music-matched listening, THD/IMD/aliasing/CPU and FL Studio in-host checks before claiming production-ready.
6. Handoff and release gate: make new real GUI screenshots, report exact new commit and CI, red/green test results and unresolved problems. No main merge or final release without approval.

**EXACT CONTINUATION POINTERS:** docs/codex-handoff/2026-11-24/FINAL_MAXIMIZER_GUI_DSP_HANDOFF.md (this document); AGENTS.md; docs/MASTERLABX_EXECUTION_DIRECTIVE.md; docs/phase5/CONCEPT_C_POWDERCOAT_BUILD_SPEC.md; Source/Editor.h; Source/FinalTruePeakPanel.h; Source/FinalTruePeak.h; Source/Processor.h; Source/ModuleParameters.h; Source/GraphConfiguration.h; Source/StudioTheme.h; tests/GuiSmoke.h; tests/FinalTruePeakTests.cpp; tests/FinalTruePeakGraphTests.cpp; tools/gui_assets/render_maximizer.py; tools/gui_assets/validate_maximizer.py; CMakeLists.txt; .github/workflows/windows.yml; install/install_masterlabx.ps1; install/INSTALACJA.md. Notion source: https://app.notion.com/p/4105fed1901b810e9472e11439017087 .

**SOURCE IMAGES DELIVERY:** The full reference screenshot and both new full-panel ImageGen designs are in the conversation's separate handoff ZIP to download and attach to Codex. They are NOT embedded in GitHub or secretly linked to final JUCE. If ZIP is not attached to Codex, request it rather than claim to have visually seen it. The actual generated metal bezel/monitor frame, source script and asset manifest ARE in GitHub/CI.

**REPORT REQUIREMENTS:** WYKONANO / ZWERYFIKOWANO / TESTY / NIEZWERYFIKOWANE / NIEWYKONANE / ŹRÓDŁA, with actual command, SHA, time, artifact and URL. No invented tests, no conceptual-image-as-implementation, no fake meters, no automatic merge or publishing.
