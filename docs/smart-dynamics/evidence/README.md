# Smart Dynamics Phase 2B evidence

`dynamics_evidence` renders deterministic five-second 48 kHz stereo PCM16 files through the actual `SmartCompressor`. The matched files use integrated-LUFS gain matching against `dynamics-dry.wav`.

- `dynamics-legacy-matched.wav`: original dynamics equation.
- `dynamics-strong-matched.wav`: Strong reduction without Punch.
- `dynamics-strong-punch-matched.wav`: Strong with Punch 80%.
- `dynamics-metrics.csv`: raw LUFS, sample peak, crest, maximum real telemetry, match gain, matched delta and a 1 kHz harmonic probe.

On this stimulus Strong raises maximum gain reduction from 11.27 to 15.78 dB without makeup. Punch reports 9.6 dB transient restoration and raises loudness-matched crest from 14.12 to 14.69 dB. Matched delta rises from 0.0200 in Legacy to 0.0852 with Strong+Punch. These synthetic measurements demonstrate control authority, not musical preference.

The Digital 1 kHz unity-ratio probe measures harmonic energy at -156.5 dB relative to the fundamental; the existing Analog color measures -49.7 dB across harmonics 2–8. This is a deterministic DFT probe, not a full AES17 THD+N or aliasing certification.
