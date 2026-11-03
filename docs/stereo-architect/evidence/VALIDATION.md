# Verification on 2026-11-03

Tested implementation: commits d3a79df and 58e3ffb; final GUI smoke locator correction 2932fb5. The correction searches inside the Stereo panel instead of the editor's immediate children. No DSP changed after the full test run.

| Check | Actual result |
|---|---|
| Linux Release full JUCE build and VST3 link | Pass, GCC 14.2, JUCE 8.0.6 |
| Full CTest | 22/22 pass; see ctest-linux.txt |
| Core-only CTest | 13/13 pass |
| Actual GUI message loop | Pass, reopen 3 times, 750×550 / 1000×700 / 2000×1400; slider → APVTS; see gui-linux.txt |
| Actual Linux VST3 pluginval | Strictness 5, process exit 0; see pluginval-linux.txt |
| Standalone StereoTests ASan + UBSan | Exit 0, no diagnostics; leak detection enabled |
| Legacy width-only render comparison | Bit-identical on tested signals at 44.1/48/96 kHz; details in README.md |
| Audio-thread allocations | Existing allocation, module integration and 720-route tests pass with Stereoize enabled |
| Windows MSVC/VST3 CI | Started on PR; status is live in GitHub Actions, not established by local tests |
| FL Studio / musical listening | Not run |

Stereo tests cover mono on the supported stereo bus, narrow/conventional stereo, anti-phase LF, neutral/bypass/Amount/band nulls, algebraic mono sum, per-band telemetry, generated-Side bass attenuation, solo reconstruction, Delta, DC automation continuity and exact fixed-setting sample-block independence (1/257/4097 samples). Processor integration covers presence/on flags, Parallel 1/N dilution and unchanged 5 ms PDC. State tests cover schema 1–6 migration and schema 7 roundtrip. The 13-core result predates the telemetry CSV export, while full 22-test run includes it.

The screenshot is the actual editor rendering synthetic test audio through the current full chain. Stereoize is zero in that screenshot; it demonstrates the connected W/C displays and control placement, not a measured before/after of Stereoize. The WAV pair isolates StereoImager with Stereoize=0.7 and matched integrated LUFS.

The DSP test proves a measurable signal difference; it does not establish perceived musical quality, universal decorrelation, Windows host behavior or complete Issue #1 acceptance. See ../PHASE1.md for the remaining work.
