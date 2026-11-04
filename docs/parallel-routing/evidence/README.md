# Phase 2A evidence

The WAV files are deterministic five-second 48 kHz stereo PCM16 renders produced by `parallel_evidence` through the actual `SerialGraph`; no third-party audio is included.

- `parallel-dry.wav`: latency-aligned reference.
- `parallel-legacy-1-over-n-matched.wav`: Spectral EQ plus two present/bypassed dry branches using the preserved 1/N denominator, loudness matched to the reference.
- `parallel-focused-compensated-matched.wav`: the same three branches with +6 dB EQ weight, -6 dB dry weights and normalized compensation, loudness matched to the reference.
- `parallel-metrics.csv`: raw integrated LUFS, sample peak, stereo correlation, loudness-match gain and matched delta ratio.

The focused render produces a larger level-matched change: delta ratio 0.466625 versus 0.278939 for the legacy weighting. This demonstrates control authority on this synthetic signal; it is not a listening-quality score. Raw peak is reported before matching and is not a True Peak measurement.

## Legacy regression

`ParallelLegacyDigest.cpp` was compiled twice with GCC 14.2 `-O3`: once against `origin/main` at `7d841c9`, and once against this implementation. Twelve renders cover 44.1/48/96/192 kHz and three presence/bypass combinations. Their per-render FNV-1a digests were identical; the SHA-256 of either twelve-line digest list is `979a20e70b391f2c882bf0df9a25e41b7289a1de0c789c8755b2bce54223d391`. This establishes bit-identical output in the local same-compiler comparison when the appended controls retain their defaults; it is not a cross-compiler bit-exact promise.

## CPU probe

`ParallelBenchmark.cpp` renders 2000 blocks of 257 samples and keeps the best of five wall-clock runs. Both modes use the same Dynamics + Spectral EQ branches; controlled mode enables non-default trim, wet and normalization. Input generation is included. The shared cloud host makes these numbers provisional, especially the negative 48 kHz difference.

| Rate | Legacy seconds | Controlled seconds | Difference | Controlled audio/wall |
|---:|---:|---:|---:|---:|
| 44.1 kHz | 0.508052 | 0.537104 | +5.72% | 21.70x |
| 48 kHz | 0.628969 | 0.615371 | -2.16% | 17.40x |
| 96 kHz | 1.141440 | 1.319400 | +15.59% | 4.06x |
| 192 kHz | 0.351913 | 0.376477 | +6.98% | 7.11x |

Even the slowest recorded controlled run remained above 4x realtime. These measurements are an engineering smoke probe, not a realtime deadline guarantee.
