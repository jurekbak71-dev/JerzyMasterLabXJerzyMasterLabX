# Phase 5E — True-Peak adversarial audit and CPU evidence
Date: 2026-11-06. Source: JerzyMasterLabX public GitHub, parent merged PR #16.

## Scope and release gate
The approved VST3 with optimized 4x detector is the baseline, not a certified BS.1770/EBU R128 true-peak limiter. A 2.5% headroom before a sample clamp does not mathematically guarantee an arbitrary 8x-reconstructed output peak will remain below ceiling.

This phase adds:
- `final_true_peak_adversarial`: an independent 49-tap / 8-phase Blackman-sinc reconstruction of output from 96 test cases across 44.1/48/96/192 kHz. Stimuli include near-Nyquist phase offsets, alternating samples, square edges, impulses, DC transitions, multitone and deterministic pseudorandom bursts. Measures worst excess in dB over the user ceiling. Test also asserts strict sample-peak bound, no NaNs and finite reconstruction.
- `final_true_peak_benchmark`: Windows x64 side-by-side baseline versus optimized core microbenchmark at 44.1/48/96 kHz, printed as ns/sample and old/new ratio. Benchmarks are noisy and are not automatic performance guarantees.

The adversarial test is *diagnostic*, not a pass/fail of true-peak compliance; any measured positive output excess is a blocker to a certified guarantee and prompts correction before claiming it. None of these tests changes the user-facing VST3 audio signal, parameter IDs, preset schema or latency. Full CI and pluginval required before merge. Next gate: implement correct reconstructed-output ceiling control and attack/release without changing host PDC; compare independently to a high-quality oversampler and verify measurements before exposure to users.

## First measured Windows x64 audit (2026-11-06)

Run `42184414494` produced a direct, independent 8x reconstruction of output:
- 96 cases, worst **+0.4842 dB** above requested ceiling (48 kHz, multitone/near-Nyquist signal 7, drive mode 2). The original limiting stage therefore **did not meet its own intended reconstructed-output ceiling** in this stress corpus despite clipping individual samples.
- CPU benchmark, before/after Phase 5D optimization: **44.1 kHz 257.93→230.70 ns/sample (1.118x); 48 kHz 258.07→213.89 ns/sample (1.207x); 96 kHz 257.00→213.77 ns/sample (1.202x)**. Single Windows runner, not proof of all hosts.

Corrective candidate on this same PR: inner reconstructed-peak limiting target changed from `ceiling * .975` to `ceiling * .90`, while the user's literal sample-peak ceiling and fixed host PDC remain unchanged. This intentionally trades a modest amount of peak loudness for additional ISP headroom. Differential regression continues to demand **bit-exact disabled/bypass output**, and separately checks the stricter enabled limiter against the frozen reference. The 96-signal independent 8x output reconstruction is now an actual test gate at **≤+0.05 dB relative to the ceiling**, pending fresh CI measurement. No certification, no full BS.1770 compliance claim.

If the gated audit fails, keep the PR open and correct the signal path; do not weaken the threshold without independent engineering justification.
