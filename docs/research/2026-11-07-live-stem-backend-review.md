# Live music source separation research — 2026-11-07

**Decision for MasterLabX:** keep the existing offline HTDemucs engine as HIGH QUALITY/OFFLINE fallback. Prototype a genuinely causal or streaming four-stem backend with explicit callback deadline and host PDC. Do **not** simply embed third-party model weights before redistribution rights, Windows behavior, and audio quality are validated.

## Options investigated

| Architecture | Evidence | Main limitation | Engineering decision |
| --- | --- | --- | --- |
| Existing `stems.cpp` + HTDemucs v4 F16 | Existing MasterLabX CI #566 verified 9-s WAV producing four stems | File process only, high context/compute; not a drop-in low latency stream | Preserve as offline fallback; benchmark sliding windows only in separate branch |
| Streaming adaptation of HTDemucs / SCNet | Sechet, Evrard, Kowalski, arXiv:2609.35397: output hop can have bounded latency; only tested HTDemucs streamed faster than real time on a GPU | Low algorithmic latency does **not** imply CPU execution deadline | Implement benchmark harness before choosing |
| HS-TasNet / StemgenRT | StemgenRT uses JUCE, ONNX, worker queues, four stereo stem buses, sample-aligned dry fallback and reported host PDC. Current source 0.8.0 has 3 workers and published timing evidence | **Windows 128-sample gate currently fails**: README reports 224 missed results / 40,400 callbacks on Ryzen 5 5500. Model v0.7/v0.8 rights are **CC BY-NC-SA 4.0** with unconfirmed dataset clearance | Study callback/worker scheduling and fallback patterns. **Do not bundle or distribute its weights** without license review. MIT source code is not a license for the model. |
| Purpose-built causal lightweight model | Li et al. arXiv:2609.12201 validated stringent embedded latency; illustrates quality/compute tradeoff | Training/inference integration effort and potentially lower SDR | Keep as alternative only if evidence indicates CPU feasibility |
| Band-SCNet, Online SCNet | Published CPU comparisons and recent open-source implementations | Weights and current distribution licenses require independent verification; latency vs quality tradeoff | Benchmark as candidate after licensing check |

## Practical implementation notes derived from sources
1. **Worker and callback**: ring buffers with monotonic sample positions, explicit generation for seek/loop and SPSC boundaries; no blocking or inference on `processBlock`.
2. **Late inference**: a complete, latency-aligned mix must be preserved; when model output misses its timestamp, crossfade to the same delayed original, **not silence or stale stems**. When output resumes, ramp over a bounded sample count.
3. **Energy reconstruction**: if three stems are inferred, compute `other = delayed_input - drums - bass - vocals` with careful numeric accounting, but compare perceptual quality, aliasing and stem bleed. Reconstruction guarantees sum identity, not isolation quality.
4. **Host latency**: measure algorithmic window/lookahead separately from graph/worker buffer and FL Studio hardware round trip. Use JUCE `setLatencySamples` and verify actual PDC with impulse.
5. **Host sample rate**: 44.1 kHz-only models need robust resampling or explicit unsupported-rate fallback. No invisible wrong-pitch 48/96 kHz processing.
6. **Silence safety**: hard-mute hallucinated model content on digital silence; detect and gate spurious leakage only with attack/release smoothing.
7. **Benchmarks**: run reference material with source tracks, report SI-SDR or SDR, CPU worker p99 duration, callback deadline misses, runtime memory, exact hardware and session details. Quality and CPU results must be tested separately.

## License checkpoint
- StemgenRT original code: MIT, copyright (c) 2026 Axel Delafosse (must keep notice if reused).
- StemgenRT v0.7/v0.8 independent weights: CC BY-NC-SA 4.0 per `licenses/WEIGHTS-LICENSE.md`; pretrained data MUSDB18-HQ and MoisesDB have commercial restrictions. Their README explicitly states Windows experimental at present.
- The existing MasterLabX HTDemucs model also needs a separate model/data distribution review; code license does not establish all model/training rights.

## Mastering and EQ secondary research
- FabFilter Pro-Q 4 illustrates node-based EQ gestures, spectral dynamics, EQ Sketch and multi-instance control; do not copy visual assets or code. In FL Studio, model/host latency can affect multi-instance ordering.
- EBU R128 recommends -23 LUFS as a *broadcast programme* reference, not mandatory music mastering target. Implement configurable loudness goals and measure via ITU-R BS.1770-5.
- Steinberg VST3 prohibits changing the active automatable parameter set dynamically. Additional bands need predeclared stable IDs and enable flags; don't change meanings of legacy IDs.

## Source links (checked 2026-11-07)
- https://github.com/sweetspotsoundsystem/stemgen-rt
- https://github.com/sweetspotsoundsystem/stemgen-rt/blob/main/licenses/WEIGHTS-LICENSE.md
- https://github.com/sweetspotsoundsystem/stemgen-rt/blob/main/docs/parallel-workers.md
- https://arxiv.org/abs/2609.35397
- https://arxiv.org/abs/2609.12201
- https://www.fabfilter.com/help/pro-q
- https://prod.fabfilter.com/help/pro-q/using/instance-list
- https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Parameters%2BAutomation/Index.html
- https://docs.juce.com/master/classjuce_1_1AudioProcessor.html
- https://tech.ebu.ch/publications/r128
