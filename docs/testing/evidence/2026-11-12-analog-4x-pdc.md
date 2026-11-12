# Analog colour 4x / PDC evidence — 2026-11-12

Scope: fixed 4x oversampling for the unified Analog Exciter colour path,
latency-aligned module Dry/Wet and Delta, routing-stable host PDC, and aligned
INSERT profile dry reference. STEMS are not in scope.

## Measured quality gate at 48 kHz

10 kHz input, measured 30 kHz foldback at 18 kHz, combined harmonic and colour
path:

| Model | Before ratio | After ratio |
|---|---:|---:|
| Tube | 0.08676427 | 0.00456200 |
| FET | 0.08623987 | 0.00377059 |
| Tape | 0.09220487 | 0.00481866 |

The executable gate is `< 0.05` for every model. Isolated post-change colour
ratios were 0.00027336, 0.00024021, and 0.00031279 respectively.

## Latency contract

- Analog colour / unified Analog Exciter: fixed 48 samples.
- Complete plugin at 48 kHz: 416 samples (240 spectral engine + 48 colour +
  128 final true-peak stage).
- Complete plugin at 96 kHz: 656 samples.
- Bypass, module Dry/Wet, Delta, routing transitions, and INSERT 0% use aligned
  references; automating module presence does not change reported PDC.

## Verification

- Core CTest: 36/36 passed, 45.50 s.
- Plugin CTest after the INSERT alignment repair: 50/50 passed, 126.18 s.
- GUI smoke: custom EQ interaction, Brain approval/Undo, reopen, and three resize
  sizes passed; plugin state/PDC/routing/oversized-buffer smoke passed.
- Callback allocation tests passed.
- Modular benchmark remained above real time on this Linux runner; worst result
  was 2.30969x real time at 192 kHz / block 32.

The Linux VST3 build is a compile/test vehicle. A Windows x64 artifact still
requires the Windows CI/package run and is not asserted by this evidence file.
