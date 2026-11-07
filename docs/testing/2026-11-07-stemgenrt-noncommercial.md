# StemgenRT noncommercial native evaluation — A2

The user selected the noncommercial variant on 2026-11-07. This enables actual
model evaluation; it does not waive model/data license conditions or qualify a
live release. No third-party plugin code is embedded in MasterLabX.

## Implementation

- `LiveStemBackend` explicitly specifies stereo 128-sample input and output for
  the **previous** input hop, with one invalid preroll after reset.
- `StemgenRtBackend` loads the actual FP32 ONNX graph, or its three independent
  Drums/Bass/Vocals partitions using persistent helper threads. The adapter
  maintains all recurrent states, preallocates tensor buffers and validates
  tensor relationships/types/shapes, finite output and state. ORT is 1.26.0 CPU,
  sequential execution, one intra/inter-op thread per session, spinning off and
  KleidiAI disabled. Other is the previous mixture minus ((Drums+Bass)+Vocals),
  matching the parent graph. Host order is Drums/Bass/Other/Vocals.
- `LiveStemWorker` is the actual asynchronous adapter from A1 input queue to
  native neural inference and timestamped output queue. Inference/reset,
  helper synchronization and thread lifecycle are outside the audio callback.
  It assembles hops, resets state on epoch or position gaps, rejects stale
  backlog, catches backend exceptions/nonfinite output and publishes aligned
  frames. Stop/join precedes backend/session destruction.
- `LiveStemEngine` publishes its current epoch atomically so the worker can
  discard pre-seek input without spending neural compute on it.
- `fetch_stemgenrt_models.py` downloads pinned blobs outside Git. SHA-256 and
  sizes are checked before replacement and on cache reuse. Evaluation CMake
  also rejects models not matching the pinned SHA-256 identities.
- The feature is opt-in `MASTERLAB_STEMGENRT=ON` for native evaluation tools.
  It is not yet linked into the plugin or wired to `processBlock`, and changes
  no GUI, APVTS parameter, old-state behavior or existing installer payload.

## Reproduce

```
python tools/fetch_stemgenrt_models.py --out build/models --accept-noncommercial-model-terms
cmake -S . -B build-native -DMASTERLAB_PLUGIN=OFF -DMASTERLAB_STEMGENRT=ON \
  -DMASTERLAB_ORT_ROOT=/path/to/onnxruntime-sdk-1.26.0 \
  -DMASTERLAB_STEMGENRT_MODELS=/absolute/path/to/build/models \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-native --parallel 3
ctest --test-dir build-native --output-on-failure --parallel 2
build-native/stemgenrt_benchmark /absolute/path/to/build/models 2000 20
```

Use native 44100 Hz only. The A1 boundary accepts other rates but this model does
not: A3 must enforce fallback or add a tested rate adapter. No 48/96k neural
support is claimed. The SDK's own MIT/third-party notices must accompany any
future runtime distribution; none is packaged by this evaluation workflow.

## Checks and early Linux evidence

Actual neural tests compare parent and three-partition waveforms across 48 hops,
verify reconstruction against the previous input hop, reset/silence and invalid
input. Worker tests use varying sample content, verify off-audio invocation,
reset while inference is deliberately blocked, exception/nonfinite recovery,
latency alignment and shutdown. A corrupt substituted model set was accepted by
the initial CMake configuration (RED); pinned-hash validation now rejects it and
accepts the genuine blobs (GREEN). A reviewer found the Windows `max` macro
portability defect; the native benchmark now defines `NOMINMAX`.

Linux initial short paced trials, native model, 44100 Hz, 4096-sample boundary delay
(92.88 ms), 10 s per buffer:

| Buffer | Worker mean hop | RTF | Missing result frames | Queue drops | Failures | Callback >1ms late |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
|128|1.683ms|0.580|0|0|0|27|
|256|1.705ms|0.588|0|0|0|16|
|512|1.711ms|0.589|0|0|0|3|

The environment exposes AMD EPYC 9V74 with a two-CPU-equivalent quota. Unpaced
three-worker mean was 2.862 ms / RTF 0.986, versus 3.766 ms / RTF 1.297 for the parent.
Unpaced and paced tests have different scheduling/load behavior; neither is a
controlled hardware ranking. Reconstruction peak error against the previous
input was 1.49e-8 FS. This algebraic residual identity does **not** measure source
isolation or musical quality. Synthetic sine input is not a multitrack corpus.
Short paced tests with reported callback start lateness are not sustained DAW
qualification, and no Windows result is inferred from them.

The separate `Noncommercial StemgenRT evaluation` workflow runs native Windows
CTest, real model waveform parity, unpaced inference and paced callback tests,
with CPU/RSS and host timing evidence. Artifacts contain logs/CSV/notices only;
no model weights or plugin installer are uploaded by that workflow.

## Exact model rights and provenance

Pinned upstream software source: `7d2d0c19d956f67259b8358c4127f991077e02b0`.
Parent graph SHA-256:
`0a49388dd2a47986ab9a161c8764c22853b4d601c188ac052eb530f076d513c7`.
Unmodified partition hashes/geometry are in
`licenses/StemgenRT/partition-manifest.json`; matching training/model card is
pinned to public training source `029c3254fcc252e8d9dfde32cff46bb3cbb8fae2`.

Publisher's weights grant: CC BY-NC-SA 4.0, credit Axel Delafosse/StemgenRT and
dataset authors, retain notices, identify changes and comply with share-alike.
Weights are unchanged; only the host adapter/worker is newly implemented.
The complete grant/notice and upstream MIT notice are retained under
`licenses/StemgenRT`. No legacy v0.6 weights or dataset recordings are included.

Dataset credit: Rafii, Liutkus, Stöter, Mimilakis and Bittner — MUSDB18-HQ 2019,
DOI 10.5281/zenodo.3338373; Pereira, Araújo, Korzeniowski and Vogl — MoisesDB 2023,
arXiv 2307.15913. The publisher explicitly does not clear third-party rights.
MUSDB18-HQ's original record says educational purposes only; MoisesDB carries
CC BY-NC-SA 4.0. A noncommercial selection alone does not establish blanket
clearance for arbitrary distribution. Installer redistribution remains open;
this work evaluates the model and retains provenance rather than asserting
categorical legal clearance.

Sources:
- https://github.com/sweetspotsoundsystem/stemgen-rt/tree/7d2d0c19d956f67259b8358c4127f991077e02b0
- https://github.com/sweetspotsoundsystem/StemgenRT-5.8/blob/029c3254fcc252e8d9dfde32cff46bb3cbb8fae2/MODEL_CARD.md
- https://zenodo.org/records/3338373
- https://github.com/moises-ai/moises-db#licensing
