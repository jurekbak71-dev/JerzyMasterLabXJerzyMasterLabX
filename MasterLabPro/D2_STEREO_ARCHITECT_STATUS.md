# D2 Stereo Architect — status and measured verification

Date: 2026-11-27
Branch: `masterlabpro-development`
Scope: this note documents only the D2 Stereo Architect work present in this change. It is not an acceptance report for D2–D4 or GUI A+C.

## Implemented in this change

- Kept original parameter IDs 1–14 and added IDs 15–27 for Natural/Wide/Ultra mode, seven band enable switches, and Stereoize controls. State loading accepts the original 14-parameter and intermediate 22-parameter formats; current state is 27 parameters.
- Width 0% maps Side to 0; 100% is neutral; the existing width control's 200% maps to Side gains 2×/3×/4× in Natural/Wide/Ultra. At 100%, all modes stay neutral. These are explicit linear Side gains, not a claim of perceptual loudness compensation.
- Stereoize is off by default. It adds a first-order all-pass-derived component from Mid to Side; amount, spread, character, and mono-compatibility control its level. This intentionally changes phase and mono fold-down when enabled.
- Added mode and Stereoize buttons, host parameter attachments, per-band enable switches, and Stereoize controls to the Stereo Architect editor. The existing measured vectorscope remains in place.
- Added DSP checks for all three modes, all seven band switches, mono Stereoize, mono compatibility, and old-state migration.

## Measurements

Run: local Linux Release build, 48 kHz, 128-sample blocks, 1200 Hz anti-phase sinusoid, ULTRA unless noted. Ratios are RMS Side output / RMS Side input after settling; they are not loudness or listening judgments.

| Check | Result |
| --- | ---: |
| 30 Hz, SUB width 0%, Side ratio | 0.443753 |
| 8 kHz, SUB width 0%, Side ratio | 0.999977 |
| 1200 Hz, 200%, NATURAL | 2.000 |
| 1200 Hz, 200%, WIDE | 3.000 |
| 1200 Hz, 200%, ULTRA | 4.000 |
| All seven bands disabled | 1.000 |
| Stereoize mono Side/Mid, off | 0.000000 |
| Stereoize mono Side/Mid, enabled, compatibility 0% | 0.384517 |
| Stereoize mono Side/Mid, enabled, compatibility 100% | 0.192259 |
| Stereoize mono Mid deviation | < 1e-6 RMS relative |

Individual band enable probe, 48 kHz, anti-phase sine at approximate band center, selected band 200%, other widths 100%, ULTRA; values are enabled/disabled Side ratios:

| Band | Probe Hz | Enabled | Disabled |
| --- | ---: | ---: | ---: |
| SUB | 40 | 3.3746 | 1.0000 |
| LOW | 150 | 2.8071 | 1.0000 |
| LOW MID | 375 | 1.9981 | 1.0000 |
| MID | 1000 | 2.8052 | 1.0000 |
| HIGH MID | 3000 | 2.0210 | 1.0000 |
| HIGH | 6000 | 2.0962 | 1.0000 |
| AIR | 12000 | 3.5000 | 1.0000 |

The variation reflects the existing first-order crossover overlap; these measurements do not establish sharp band isolation or low inter-band leakage.

## Verification performed

- Linux Release JUCE build: `masterlabpro_stereo_test`, `masterlabpro_plugin_editor_regression`, and `MasterLabPro_VST3` built.
- Full local CTest: 25/25 passed.
- ASan + UBSan with `ASAN_OPTIONS=detect_leaks=1`: 23/23 passed.
- TSan: 23/23 passed.
- Local Linux VST3 pluginval strictness 5 passed with GUI tests skipped. This is not Windows or FL Studio verification.
- `git diff --check` passed before commit.

## Known limits / remaining work

- This change does not implement Mono Bass, phase/correlation protection, correlation limiting, Side-level limiting, or automatic width limiting.
- Existing first-order crossover filters remain. These center-frequency probes do not measure crossover leakage, full frequency response, phase response, transient clicks, peak overshoot, or mono compatibility over varied source material.
- Stereoize is a simple all-pass-derived decorrelator, not a reproduction of a commercial processor. Its sound requires level-matched listening; no listening claim is made here.
- GUI interaction regression compiled but could not be run locally because the environment has no display server. Windows CI must verify the current commit. No latest D2 Windows installer, pluginval result, rendered 100/150/200% image set, or FL Studio test exists yet.
- Input Compressor independence validation, Spectral EQ, Dynamic EQ, full visual redesign, benchmarks dedicated to Stereo Architect, and full D2–D4 acceptance remain outstanding.
