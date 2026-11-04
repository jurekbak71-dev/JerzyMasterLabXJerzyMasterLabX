# Phase 2C validation

Validated on Linux x86-64 on 2026-11-04. The Windows x64 workflow repeats the build, full CTest suite, GUI smoke, pluginval and artifact packaging before the phase is accepted.

| Check | Result |
| --- | --- |
| Release build | Passed; VST3 and all test/evidence targets built |
| CTest | 28/28 passed in 53.09 s |
| GUI smoke | Passed three editor opens at 750x550, 1000x700 and 2000x1400 |
| pluginval | Passed at strictness level 5 |
| ASan/UBSan | Phase 2C DSP test passed with leak detection and halt-on-UB enabled |
| Audio-thread allocations | Zero in the measured Phase 2C loops |
| PDC | Fixed 5 ms remains unchanged at all four tested rates |

Coverage includes:

- exact Legacy digests for three scenarios at 44.1, 48, 96 and 192 kHz;
- Clean/Strong/Extreme authority and 4/9/12 dB hard bounds;
- single-tone support protection, broadband material and transient-rich material;
- silence, near-silence, automation, Cut mode, Delta and variable block sizes;
- APVTS delivery, schema-10 restore, schema 1–9 migration and fixed PDC;
- zero allocations in the measured audio-thread loops;
- Hybrid Studio control attachment and coefficient-based response rendering.

The measured maximum corrections for Clean/Strong/Extreme were 3.04/6.71/8.94 dB at 44.1 kHz and remained inside their 4/9/12 dB bounds at all four sample rates. A single-tone Extreme probe stayed below 6.59 dB. On transient-rich material, Tame reduced maximum adaptive movement by 28–33% relative to the same unprotected settings.
