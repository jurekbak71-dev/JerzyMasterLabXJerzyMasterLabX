# MasterLabX regression audit — ongoing, not a production release

## Scope and source identity

User START authorizes repair and testing, not merge to main. The user explicitly confirmed **no STEMS LIVE/OFFLINE** in this session; REBALANCE remains separate. Later source-separation references in the handoff are superseded by that confirmation. Current APVTS IDs and state behavior are retained.

Implementation baseline: `2e1c68a84816849753a5a7d42a98dc57b6c4e1d0`, branch `feat/remove-stems-live-offline-20261109`. Work is isolated on `fix/masterlabx-audit-dsp-gui-20261110`. The original local checkout was older (`b0e6658`); it was not overwritten. Main is `5d964d72ea4f74f1db415cad69435aa11ace871d` at initial fetch.

Read the full handoff at `docs/codex-handoff/2026-11-10/CODEX_MASTERLABX_FULL_IMPLEMENTATION_PROMPT.md` on `docs/codex-masterlabx-audit-gui-dsp-20261110` (`c492bbf6`). The uploaded ZIP contains a related, nonidentical document and 11 PNG references; every size/hash matched its manifest, ZIP CRC passed. The approved Analog Luxury layout was visually inspected. No drawn example waves or needle graphics were represented as DSP measurements.

GitHub readback: PR 18 is draft/open at `c11af88`; PR 19 and PR 20 are draft/open at `ca979dad`; neither contains the latest no-STEMS iteration. PR 21 is a documentation-only draft. Baseline Windows workflow [42886356702](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/42886356702) was independently queried and is SUCCESS for `2e1c68a`. This does not validate this repair branch or the user's installed binary. **The exact binary represented by the user's screenshots remains unverified.**

## Reproduced defects and bounded repairs

1. **Final OFF still limits.** `configureGraph()` OR-ed the final enable with the legacy limiter enable. With final OFF, toggling the legacy flag changed output by up to `0.548811376`; expected zero. The single final enable now controls both constituents. The regression renders real graph audio at block sizes 1/257/4097 and separately proves the ON state still limits. OFF output peak 0.8, ON peak 0.226058 for a -12 dB ceiling. PDC is retained.
2. **Bass/Transient/Final scopes depend on callback size.** Each call cleared 64 points and filled `i*64/count`, creating zero holes for small blocks and changing the represented time interval. Identical sine streams produced differences up to 0.399565. `ScopeCapture` now captures 512 consecutive actual samples, publishes complete frames on a rising zero crossing (100 ms auto-trigger timeout for DC/silence), and retains the sample clock across callbacks. No interpolation, FFT, heap allocation or lock is introduced. At 48 kHz the displayed span is 10.646 ms; at 192 kHz it is 2.661 ms. This is a short waveform view, not a spectrum or envelope detector.
3. **Limiter waveform alignment and GR baseline.** Input trace now uses the delayed sample corresponding to the output sample; the previous trace compared times 128 samples apart. Initial/disabled gain envelope is unity (0 dB reduction), not zero gain. Captures reset across ON/OFF discontinuities in all three processors. Review added a nonzero bypass fixture: Bass and Transient previously retained pre-bypass frames; both reproduced failures before the settings-transition reset fix. GUI traces are clipped to the instrument rectangle; GR uses a separate 0–18 dB mapping and a time-span legend.

4. **Invisible Analog Exciter rotary controls.** At 750×550, the old 39 px slider minus its value field left less than the theme's minimum 24 px rotary area. The new GUI regression reproduced `analogizerDrive` without drawable area. Two columns on each side now reserve 60 px slider height at minimum size. A second regression reproduced central EXCITE overlapping bottom-row labels; compact knob bounds now leave a gap. Bass and final-limiter knob bounds also leave room for the scope legends. This fixes control visibility, not completion of the approved Analog Luxury design.
5. **GUI evidence generation.** Module screenshot streams now seek to zero before truncating; previously repeated captures appended PNG payloads and viewers showed the first render. The EQ response check uses the current plot rectangle and width-relative curve coverage instead of an obsolete rectangle and a fixed 400-pixel threshold.

The scope-only changes preserve the rendered audio exactly for the stored deterministic dual-channel fixture: 384408 bytes before/after SHA-256 `8a0af34e82ef121665953decb5d33655c2023bb1595dd7125ff7257d3dce65f9`. This is a bounded regression comparison, not a universal proof.

## Requirement / implementation / evidence / remaining matrix

“Tests” here means the named existing tests actually passed in the Linux core suite; it does not imply coverage of every acceptance criterion.

| Area | Required / current implementation | Evidence | Open / unverified |
|---|---|---|---|
| Console | Concept C, paired before/after FFT and separate EQ response; Editor/ConceptCOverviewPanel | source inspection; GUI tests pending current build | visual acceptance, HiDPI, all signal/silence/bypass captures |
| Spectral EQ | 16 bands, attached parameters, upper band navigation; Engine/Editor | filters, response, routing | full per-control GUI coverage and listening |
| Dynamic EQ | 8 real bands and 32-sample control period; module DELTA | modules, graph tests | spectral heatmap quality, all-band host automation |
| Dynamics | full-band compression/upward/punch; not a multiband compressor | dynamics_phase2, modules | before/after energy heatmap, listening and CPU under heavy sessions |
| Stereo Architect | four M/S bands, Side creation, interpolated Side delay, all-pass blend, motion pan | stereo_architect and source inspection | Side delay is not independent channel Haas/ping-pong; phase knob is a wet blend, not constant angle rotation; final-output correlation differs from pre-delay per-band telemetry |
| Stabilizer | adaptive tonal processor, intensity and delta | stabilizer, stabilizer_phase2 | listening and full before/after display acceptance |
| Analog Exciter | unified harmonic generator + one Tube/FET/Tape colour transfer, EXCITE macro; warm nonlinear term exists | analogizer_dsp, harmonic_graph_integration, impact_authority | approved Analog Luxury PNG layout not integrated; full-chain aliasing/IMD/H2–H5 measurements; colour stage has no oversampling; no unified Wet/Delta; existing mode tests do not alone prove all qualitative claims |
| Analog Color | old host controls adapted into unified AnalogExciter; separate algorithm no longer routed | analog_graph_integration; AnalogExciter adapter | all old control semantics and combined macro interactions need parameter sweep; standalone AnalogColor tests do not prove routed behavior |
| Bass | Sub/Body/Harmonics; no duplicate active Mono Bass; DELTA; repaired waveform capture | bass_architect_dsp, scope_partition | graphical layout at all sizes and listening |
| Transient | three-band attack/sustain, DELTA, modulation RMS; repaired capture | transient_sculptor_dsp, scope_partition | dedicated transient event/envelope display still missing |
| Restoration | four-region learned noise estimate, harmonic hum notches, causal click repair, approximate plateau modification | audio_restoration | not a validated spectral denoiser; separate MUSIC/VOICE quality and musical damage tests missing |
| Final Limiter | legacy sample limiter plus final 4x detector, fixed 128 additional samples; repaired OFF control | final_limiter_off, final_true_peak_dsp/graph/adversarial/differential | user-adjustable lookahead/attack and actual 2x/4x/8x audio oversampling absent; no formal BS.1770 conformance; ON/OFF click stress and combined-stage GR measurement remain to audit |
| Inspector | actual meters and analysis data | source inspection; integration tests pending | measurement-standard compliance and all host conditions |
| Master Brain | capture, measured proposal, A/B/Apply/Undo; extended parameter proposals | brain_rules | current JUCE Brain tests pending; full MANUAL/AUTO/HYBRID orchestration, Restoration selection and all new controls not established |
| Routing | nine movable kinds and one fixed final slot; legacy six-family parallel path | serial_graph, parallel_routing | full arbitrary parallel/hybrid graph not implemented; all automation transitions and meter alignment |
| MASTER/INSERT | existing profile changes wet/dry; fixed host latency | ProcessingModePanel explicitly reports lower-latency graph missing | real low-latency INSERT profile remains unimplemented |
| STEMS | intentionally excluded by current user instruction | AGENTS/directive/Notion and confirmation | do not restore |

## Validation record and gates

- Baseline Linux GCC 14.2 Release core: 31/31 PASS, 11.48 s.
- Initial OFF fix: 32/32 PASS, 12.42 s.
- Scope integration: 33/33 PASS; later idle/bypass regression checks also run individually. Latest core run: 33/33 PASS, 17.04 s.
- Scope partition/reset/bypass tests rerun under ASan/UBSan after the final DSP edits: exit 0, no reported diagnostics.
- Local JUCE configure initially failed on X11 include paths, then missing FreeType pkg-config dependencies; local dependency metadata resolved configuration. VST3 linking initially missed the module linker library path; the local CMake cache now supplies it. These are environment fixes, not source-level DSP fixes.
- Linux full CTest before the last nonzero-bypass regression: 46/46 PASS, 49.05 s. Latest nonzero-bypass scope test and ASan/UBSan: PASS; full suite rerun after that repair is pending. Linux VST3 pluginval strictness 5: exit 0. GUI screenshots/reopen test: PASS after the evidence, rotary and label fixes, including host gestures and Brain Apply/Undo. Three module sizes (750×550, 1000×700, 1400×1000) captured; EQ also tested at 2000×1400. This does not establish all-module visual acceptance; remaining compact layouts still need work.
- Existing limiter microbenchmark: 441.38 ns/sample at 44.1 kHz, 439.89 at 48 kHz, 479.09 at 96 kHz on this Linux container. Its `old` means the test reference implementation, **not this branch's baseline**. This is not full-chain CPU or a real-time deadline/host-soak test.
- New Windows CI / PE architecture / installer / ZIP checks: pending.
- FL Studio on user's PC, musical listening, all-module final GUI, host soak/CPU: **NOT VERIFIED**.

The complete specification is **not fulfilled**. A repair artifact must be labelled a test candidate, never final/production. Do not merge without the user's explicit acceptance.

## Sources read

- [Handoff](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/blob/docs/codex-masterlabx-audit-gui-dsp-20261110/docs/codex-handoff/2026-11-10/CODEX_MASTERLABX_FULL_IMPLEMENTATION_PROMPT.md)
- [Concept C and November 10 requirements](https://app.notion.com/p/4105fed1901b810e9472e11439017087)
- [Decision history, including removal of STEMS](https://app.notion.com/p/4115fed1901b81c4a099d97f56279132)
- [Approved operating guide](https://app.notion.com/p/4085fed1901b817db779c41c0375f6bc)
- Repo AGENTS.md, CMakeLists.txt, docs/MASTERLABX_EXECUTION_DIRECTIVE.md, docs/stereo-architect, docs/phase5/FINAL_TRUE_PEAK_RESEARCH.md, active Source headers and tests named above.
- Local JUCE 8.0.6 source, especially extras/Build/juceaide/CMakeLists.txt and extras/Build/CMake/JUCEUtils.cmake for the build diagnosis.

## Verified on 2026-11-12: repair candidate 02785d8

Windows [run 43005212781](https://github.com/jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX/actions/runs/43005212781) completed successfully. Downloaded LastTest.log contains 46 Test Passed / 0 Test Failed. CI also passed real GUI reopen/resize, pluginval strictness 5 and packaging. Exact artifact head is 02785d881fb354a753b4af450098bf6806924fb0.

Downloaded artifact hash matches GitHub metadata; both outer and inner ZIP CRC checks passed. The VST3 has PE signature and AMD64 machine 0x8664, plus moduleinfo.json. Package contains CMD/PowerShell installer, installation instructions and font license. Installer source was read; it requests elevation and replaces the existing VST3 bundle in CommonProgramFiles. **Installer execution and FL Studio remain unverified.** Checksums are in evidence/2026-11-12/windows-package-verification.json.

Final Linux run for 02785d8: 46/46 PASS, 42.02 s; GUI PASS. Independent 8x/49-tap stress fixture: 96 cases, worst excess -0.2110 dB relative to ceiling. This is fixture evidence, not universal true-peak conformance. Full-chain benchmark fixture and baseline/current CSVs are attached; they exclude restoration, host/GUI overhead and deadline-jitter testing.

This is an installable test candidate with bounded regression repairs. The open requirement matrix above still applies; the full task is not complete and no main merge occurred.
