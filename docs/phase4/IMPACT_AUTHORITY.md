# Phase 4A — Stronger harmonic authority without legacy breakage
Date 2026-11-05. Development stage; no merge without Windows CI and FL Studio host test.

## User finding
Harmonic Exciter and Analog Color work in FL Studio but are too subtle and do not cover a musically satisfying range.

## Root causes and fixes
- Harmonic Exciter generates harmonic-only addition with a fixed factor 0.5; with band levels low, even high Drive can be subtle. Introduce `exciterImpact` range 0–1: generated harmonic addition multiplier `1+5*Impact` after the existing generation, 20-ms smoothed, leaving 0.0 numerically identical to original.
- Analog Color uses a drive-dependent `1/sqrt(drive)` auto-gain compensation even at extreme drive; its impact can be suppressed in the final wet mix. Introduce `analogImpact` range 0–1: auto-gain exponent interpolates from -0.50 to -0.12 and output stage adds up to 2.5× wet gain, while preserving 0.0 behavior exactly. Normal dry/wet and 20ms parameter smoothing remain.
- Both settings default to 0 and are appended to the end of parameter layout, indices 201,202, preserving all previous 201 IDs. Schema 13 migration from schemas 1–12 retains original behavior and defaults to zero.
- Hybrid Studio panels expose both real APVTS-attached Impact controls.
- `tests/ImpactAuthorityTests.cpp` validates stronger signal deviation from dry and block independence across 44.1/48/96/192 kHz. A level-matched listening evaluation is still required: sheer gain increase alone does not demonstrate improved musical quality.

## Safety / release gates
- Strong settings can make output hotter than the limiter ceiling because both modules are post-chain. Avoid claiming true-peak limiting. Protect output from non-finite values, but existing hard limit is not a guarantee of a safe final master.
- Oversampling currently remains experimental and not host-exposed: 1× high drive can alias. Phase 4B must implement 2x/4x with fixed PDC, matched dry/wet, robust high-frequency alias tests and CPU budgeting. This change intentionally does not combine host PDC architecture with sound-strength controls.
- Model changes in Analog Color are still not crossfaded; Phase 4C must smooth switching and optimize CPU.
- Must obtain successful Windows x64 CTest/pluginval/GUI smoke, then FL Studio confirmation before merge.
