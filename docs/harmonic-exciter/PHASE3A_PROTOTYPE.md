# Harmonic Exciter — first standalone DSP slice (2026-11-04)

## Implemented
- `Source/HarmonicExciter.h`: independent no-heap four-band stereo processor prototype.
- Complementary low/mid/high/air band split, per-band intensity and global drive/even/odd/mix/output controls.
- Second-harmonic generator (squared shaped band, slow DC removal) and cubic third-harmonic generator (also contributes to the fundamental; no claim of pure H3).
- Optional output-minus-input Delta and block RMS readings. Neutral bypass returns the input unchanged; new feature is not connected to VST3 or UI.
- `tests/HarmonicExciterTests.cpp` registered in CTest.

## Standalone development validation
Local equivalent prototype compiled using GCC C++20 with `-O2 -Wall -Wextra -Werror`; also built and run using ASan/UBSan, exit 0. Frequency test 1 kHz on 4 rates:
| Rate | H2 amplitude (even-only) | H3 amplitude (odd-only) |
|---|---:|---:|
| 44,100 | 0.0725509 | 0.0209323 |
| 48,000 | 0.0723680 | 0.0209247 |
| 96,000 | 0.0713222 | 0.0208996 |
| 192,000 | 0.0707768 | 0.0209007 |
Tests verified dual-mono equality, zero-addition null, bypass, silence, finite output, and exact sample block independence (1 vs 4097). **These are standalone developer checks on the matching local implementation, not a complete repository Windows/Linux CI result.** GitHub CI/complete VST3 will require additional validation.

## Explicit limitations / next work
- Does NOT implement oversampling, anti-alias filtering, phase-safe automation smoothing or robust stereo metering. Existing C++ core is a prototype, not production-quality DSP; high-frequency aliasing is expected.
- At 0% even/odd, output gain still applies when non-zero. For a unity bypass, set outputDb=0 and use off; ensure plugin design makes this clear.
- Current crossover coefficients are prepared on settings update, not per sample; smooth actual host automation before exposing these parameters.
- There is no new APVTS parameter, schema migration, serial/parallel routing or Master Brain preview integration yet; 180 existing parameter IDs and legacy output are untouched.
- Host VST3 and Hybrid Studio GUI have **not** been changed, compiled or functionally verified for this prototype.
- Next: oversampling and alias tests; proper chain order extension preserving legacy 720 permutations; GUI/APVTS control integration; Windows CI, VST3, FL Studio manual gate.
