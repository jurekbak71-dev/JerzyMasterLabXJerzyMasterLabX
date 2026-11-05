# Phase 5B — Transient Sculptor (draft)
Date 2026-11-05. New post-chain dynamics module. Implementation pending Windows CI and FL Studio host audition.

## Delivered
- Zero-lookahead stereo transient shaper in 3 bands: Low <170 Hz, Mid 170–3400 Hz, High >3400 Hz using two shared matched one-pole low-pass states per channel.
- Each band has independent Attack and Sustain values -1..+1. Fast (1.5 ms rise / 12 ms fall) and slow (55 ms) envelope comparison detects transients; saturated gain is limited to 0.12..4.5, scaled by controls. The algorithm uses no future samples and reports 0 samples of additional latency.
- Post-chain placement after Bass Architect, not modifying the existing six-family 720 permutations or earlier processor indices.
- Real-time safe, smoothed parameters (20 ms), wet mix, output +/-12 dB, safe handling of non-finite inputs, per-band true DSP modulation RMS telemetry.
- Hybrid Studio GUI with eight APVTS knobs and In chain/Active toggles.
- Schema 15: append 10 parameters after prior 211, final 221. Older schema 1–14 default `transientPresent=false`; existing projects retain old sound.
- CTest `transient_sculptor_dsp` measures neutral exact pass-through, strong/negative transient shaping, block independence, automation safety, 44.1/48/96/192 kHz. GUI smoke and state migration tests.

## Limits / release gates
- The new transient envelope uses deliberately nonlinear shapes, not high-quality linear-phase 3-band dynamics. Phase differences are modest but must be measured and subject to musical audition.
- Last post-chain slot can increase peaks above the existing limiter ceiling. No true-peak guarantee yet. A final true-peak limiter remains a high-priority improvement.
- No lookahead or oversampling yet. At extreme attack settings clicks or aliasing are possible with very short impulses. Do not market as true-peak safe.
- Windows x64 CTest/pluginval strictness 5/GUI smoke needed, followed by FL Studio verification before merging PR.
- Ultimate 3D GUI redesign remains a separate approval/design stage; this is a functional panel consistent with the existing Hybrid Studio.
