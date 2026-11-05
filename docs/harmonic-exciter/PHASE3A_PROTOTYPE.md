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

## Follow-up: standalone 2x FIR anti-alias primitive (2026-11-04)

Added `Source/HarmonicOversampler.h` and `tests/HarmonicOversamplingTests.cpp`, registered as CTest `harmonic_oversampling_dsp`. Experimental implementation: Blackman-windowed 65-tap lowpass with 2x zero-stuff interpolation, 2x nonlinear callback, and decimating anti-alias FIR, no dynamic allocation on the audio path. The primitive reports **32 base-rate samples of latency**; any further integration MUST delay dry path by 32 samples and handle host PDC, including fixed-latency mode changes.

Actual isolated source built with GCC C++20 `-O2 -Wall -Wextra -Werror` and ran successfully. Targeted `-fsanitize=address,undefined` also passed. At 44.1/48/96/192 kHz, 1 kHz identity gain measurements were 0.200005 / 0.200004 / 0.200001 / 0.200000 for input amplitude 0.2. At 48 kHz, cubic waveshaping of 10 kHz input yielded an 18 kHz alias component 0.03125000 without oversampling, versus 0.00000202 with the isolated 2x filter (ratio ~0.0000646). These tests cover one specific alias stimulus only, not comprehensive stopband behavior/THD or in-plugin performance.

**Not integrated yet:** no wiring of this primitive into `HarmonicExciter::process`; no 4x mode, latency compensated wet/dry, parameter smoothing, APVTS, routing, GUI, Brain preview or Windows VST3. Existing 180 parameters and legacy sound remain unchanged. GitHub CI test results must be read separately and must not be inferred from local tests.

Next required step: integrate 2x per nonlinear band with 32-sample aligned dry/wet, measure H2/H3 and alias suppression on the full HarmonicExciter, then ensure a fixed PDC strategy compatible with existing plugin before any VST3 release.

## Follow-up: full-core optional 2x path integrated and locally tested (2026-11-04)

The `HarmonicExciter` now has a non-host-exposed `oversample2x` configuration switch (default **false**, preserving the original 1x prototype path). Each channel and frequency band runs a separate 2x reconstruction / nonlinear / anti-alias decimation chain. An input-side 32-sample dry delay matches the 32-base-sample latency of the FIR; `latencySamples()` reports 32 while enabled in 2x mode and 0 otherwise. Existing off/default processing is unaffected.

Full-core C++20 build from an equivalent local source replica using GCC `-O2 -Wall -Wextra -Werror`: pass. Targeted ASan/UBSan: pass. A 48 kHz, 10 kHz, 0.5 amplitude input with odd harmonics and 12 dB drive gave 18 kHz aliased amplitude **0.02531838** in 1x and **0.00000177** in 2x. The exact delayed-dry null test gave maximum error 0; processing the same 48 kHz test using 1-sample and 257-sample blocks also differed by 0 in the tested scenario. These are **local DSP probes**, not CI, whole-plugin or musical-listening certification. A mixed two-render local smoke executable reported 0.17 s wall time and ~2.3 MB max RSS, not a reliable per-plugin CPU benchmark.

Remaining risks: mode switching can cause clicks, runtime host PDC not integrated, oversampling is not in the VST3 graph, 4x missing, higher sample rate and broader alias spectra unverified, UI and APVTS unimplemented. CPU budget needs realistic host assessment; 65-tap per-stage FIR multiplied across four bands and stereo channels is potentially expensive. No claim of complete Windows VST3 availability. Keep PR as draft.
