# MasterLabX — Final Maximizer: audited Codex continuation (2026-11-24)

**STATUS: WIP / DRAFT; NOT PRODUCTION-APPROVED.** This document records code and evidence, not a claim that a rendered mockup works.

## 0. Source of truth; exact repository and commits

- Repository: https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX
- **CONTINUE FROM** branch: `fix/masterlabx-maximizer-gui-dsp-20261124`
- **CURRENT DRAFT PR**: https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/pull/26
- **DO NOT REBASE ON an earlier STEMS branch, PR #22 or `main`.**
- Parent branch = `feat/masterlabx-final-maximizer-lookahead-oversampling-20261124` / PR #25.
- Parent's VERIFIED Windows x64 commit: `a3c5fc9b8831a6933b22cfb21107eb242121f830`, [CI #1181](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45333541864), success (45/45 CTest, JUCE GUI screenshot, pluginval, SHA-verified installer).
- Current PR #26 last code commit *before this handoff*: `e2ff6ab1a07121e2ce0c83901517b86c9b448f05`; check `git rev-parse HEAD` before any continuation. This handoff commit itself will advance HEAD.
- **RUNNING INTEGRATION VALIDATION**: [Windows CI #1190](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/45364424047). The new changes MUST NOT be marked tested until this run succeeds and screenshots are actually inspected. Recheck HEAD/run correspondence after every fix.
- Workstation for original user acceptance: Windows x64 / FL Studio. Local container cannot fetch the private repo using git because DNS `github.com` fails; work here uses the actual GitHub connector and remote CI.

## 1. Mandatory prior decisions, no unauthorized scope change

Read, in order:
1. `AGENTS.md` — project-specific agent rules. **No STEMS or source separation**, no mock DSP, no hidden degradation of routing/mastering. Do not merge or publish without explicit approval.
2. `docs/MASTERLABX_EXECUTION_DIRECTIVE.md` v1.2 — binding scope.
3. `docs/phase5/CONCEPT_C_POWDERCOAT_BUILD_SPEC.md` — GUI contract; approved 2026-11-06. Some historical statements (old 128-sample PDC) have been superseded by later PR #25: **inspect actual code and acceptance dates**; never silently restore old values.
4. Notion: [MasterLabX — approved GUI Concept C, ongoing audit and Maximizer](https://app.notion.com/p/4105fed1901b810e9472e11439017087).
5. Notion operating guide approved 2026-10-29: https://app.notion.com/p/4085fed1901b817db779c41c0375f6bc .
6. User-provided Final Maximizer reference screenshot in the 2026-11-24 chat, filename `image(20261110-200811).png` (old look: five left rotary controls, actual GR VU on the right, full-width dark waveform screen, small telemetry footer). **It is a visual target, NOT a screenshot of accepted new code.** If not available in Codex workspace, request/copy this exact image; never claim it was inspected.
7. An optional new full-panel ImageGen render was produced in chat on 2026-11-24. **Conceptual only**: its text/values, controls and claims may be fictional, and it MUST NOT be wired as a flat UI or construed as implemented modules. The reproducible PNGs below ARE real assets.

## 2. Confirmed baseline functional scope

- Core maximizer DSP: `Source/FinalTruePeak.h`. Actual parameters: ceilingDb, releaseMs, driveDb, character (Transparent/Punch/Aggressive), autoRelease, softClip, outputDb, lookaheadMs, attackMs, transientEmphasis, 1×/2×/4×/8× **true-peak detection factor**. 10 ms maximum lookahead buffer and constant host latency **for each sample rate**; inspect source before changing. This is **NOT** iZotope IRC5; don't copy proprietary algorithms.
- Host parameter IDs and mapping: `Source/Processor.h`, `Source/ModuleParameters.h`, `Source/GraphConfiguration.h`. The four additional advanced parameters are appended after 316 existing IDs, indices 316–319. Preserve all IDs and values on existing sessions.
- Main GUI: `Source/Editor.h` includes `Source/FinalTruePeakPanel.h` panel; `Source/StudioTheme.h` loads previous 101-frame rendered Concept C knobs/LEDs. Do not replace APVTS controls with drawn mock controls.
- Test and package pipeline: `.github/workflows/windows.yml`, Windows 2022 ClangCL Release, CTest, adversarial independent true peak, GUI smoke, pluginval strictness 5, installer with SHA-256 and rollback.
- Before this PR, 8×/49-tap adversarial suite ran 96 cases, highest measured excess +0.0491 dB above target under a +0.05 dB acceptance tolerance. **No absolute true-peak certification.**

## 3. What THIS BRANCH changed (actually written code)

| Source path | Recorded change | Verifiable state |
|---|---|---|
| `Source/FinalTruePeakPanel.h` | Replaced collision-prone CORE/ADVANCED coordinates with adaptive grid, dedicated meter column, clipped DSP waveform, own telemetry footer, and selected settings clearly distinguished from last processed audio. Embedded generated bezel and scope frame; APVTS attachments and current DSP unchanged. | Code committed, fresh screenshot required |
| `tools/gui_assets/render_maximizer.py` | **Deterministic Pillow renderer** for `maximizer_meter_bezel.png` (512×396 RGBA) and `maximizer_scope_frame.png` (1600×470 RGBA). Metallic geometry is static only; NO fake needle, waveform, meter, or LED status. | Source committed; CI validation pending |
| `tools/gui_assets/validate_maximizer.py` | Check RGBA/dimensions, manifest SHA-256 and transparent scope center. | CTest registered; CI pending |
| `CMakeLists.txt` | Render, validate, embed both PNG assets with `juce_add_binary_data`; new `maximizer_asset_pack` CTest. | Source committed; CI pending |
| `tests/GuiSmoke.h` | Require actual render assets; capture CORE and ADVANCED at 750×550, 1000×700, 1400×1000; require useful advanced slider height; keep old `limiter-smoke.png`. | CI pending |
| `.github/workflows/windows.yml` | Upload screenshot names `maximizer-core-*.png`, `maximizer-advanced-*.png` in GUI and build-log artifacts. | CI pending |

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
