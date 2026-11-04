# Phase 2A validation

Validated on Linux x86_64 with GCC 14.2 and JUCE 8.0.6. Windows MSVC results are supplied by the pull request workflow; manual FL Studio testing remains pending.

| Gate | Result |
|---|---|
| Release build | Full JUCE shared code, VST3 and test targets built successfully |
| CTest | 24/24 passed serially in 51.94 s; see `ctest-linux.txt` |
| Targeted DSP | Legacy and controlled equations have 0 reported RMS error at 44.1/48/96/192 kHz |
| Processor integration | APVTS trim/wet/normalization, schema restore and fixed 5 ms PDC passed at all four rates |
| Audio thread | Allocation counter remains zero during processing; no locks were introduced |
| Sanitizers | `parallel_routing_tests` passed ASan + UBSan, including leak detection |
| Legacy sound | Twelve same-compiler baseline/current render digests match exactly |
| GUI | Actual message loop passed three reopen cycles and 750×550 / 1000×700 / 2000×1400; APVTS bindings and real RMS/limiter telemetry asserted |
| VST3 | pluginval 1.0.3 strictness 5 exited 0; see `pluginval-linux.txt` |
| Evidence | Three matched-listening WAVs, metrics CSV and actual GUI screenshot are included |

The DSP test additionally covers one processed plus two bypassed/dry branches, all bypassed, removed branches, enabled state telemetry, routing order swaps, limiter gain trace and ceiling warning, normalized and legacy equations, stereo correlation, automation continuity and stable PDC. Trim, wet and normalization automation is smoothed on constant input. A newly added branch clears its private alignment history before the graph crossfade, preventing stale samples from a previous presence cycle.

The committed screenshot is an actual editor snapshot after test audio passed through the graph. Branch meters come from bounded processor response snapshots; the test rejects a missing/serial frame, zero Spectral EQ RMS or a missing limiter warning before capturing it.

The first parallel CTest probe used three concurrent jobs and the pre-existing `brain_analysis` queue-capacity test failed while other analysis tests ran concurrently. The complete serial run then passed 24/24, matching the repository's Windows workflow, which does not request parallel CTest execution. No Brain code was changed in this slice.

No claim is made for FL Studio save/reopen, automation recording or export because FL Studio was unavailable in this environment. The limiter is still sample-peak, and a limiter branch does not impose a ceiling on the full parallel sum.
