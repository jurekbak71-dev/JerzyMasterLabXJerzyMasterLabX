# Phase 2B validation

Validated on Linux x86-64 on 2026-11-04. The Windows x64 workflow repeats the build, CTest suite, pluginval validation and artifact packaging before this phase is accepted.

| Check | Result |
| --- | --- |
| Release build | Passed; VST3 and all test targets built |
| CTest | 26/26 passed in 55.82 s |
| Phase 2B rates | 44.1, 48, 96 and 192 kHz passed with 257-sample blocks |
| Strong authority | 25.40 dB maximum telemetry versus 18.15 dB in Legacy on the deterministic test tone |
| Dry/Wet and Delta | Sample error below 2e-6 against the explicit parallel equations |
| Automation | Intensity, Punch and Delta changes stayed finite and below the click threshold at all four rates |
| Audio-thread allocations | Zero in the measured Phase 2B processing loops |
| Legacy digest | Twelve renders match Phase 2A commit `682b9c8` exactly |
| GUI smoke | Passed three editor opens and 750x550, 1000x700 and 2000x1400 sizes |
| pluginval | Passed at strictness level 5 |
| ASan/UBSan | Phase 2B DSP test passed with leak detection and halt-on-UB enabled |

`dynamics-smoke.png` is the captured 1000x700 Hybrid Studio view. Its transfer curve uses the current APVTS settings; the three history traces use processor telemetry rather than fabricated UI data.
