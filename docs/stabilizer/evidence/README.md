# Stabilizer Phase 2C evidence

`stabilizer_evidence` renders a deterministic five-second 48 kHz stereo stimulus through the actual Stabilizer. The matched PCM16 files use integrated-LUFS matching against the dry render.

- `stabilizer-legacy-matched.wav`: exact Phase 2B algorithm.
- `stabilizer-strong-matched.wav`: Strong with continuous transient protection.
- `stabilizer-extreme-tame-matched.wav`: Extreme with continuous transient protection.
- `stabilizer-metrics.csv`: raw LUFS, sample peak, crest, maximum real correction telemetry, matching gain and matched delta.

The stimulus combines four tonal regions, deterministic noise and repeated transients. On it, Extreme reaches 6.02 dB with Tame and 8.36 dB without Tame. The protected raw render has 16.02 dB crest versus 17.24 dB without protection. These synthetic measurements show bounded authority and protection behavior; they do not select a musically preferred mode.
