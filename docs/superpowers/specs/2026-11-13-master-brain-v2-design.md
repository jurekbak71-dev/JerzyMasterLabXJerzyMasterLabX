# Master Brain V2 — verified automatic chain design

## Intent

Master Brain must build and tune one complete mastering chain from a representative input fragment, not offer disconnected preset suggestions. The user manually chooses genre and mastering character. Brain automatically chooses target loudness, true-peak ceiling, required modules, ordering, routing and parameters, then verifies and iteratively improves the result through the real DSP graph.

## Fixed decisions

- Capture at most 20 seconds of current DAW playback; do not imply whole-song analysis.
- Produce one recommended result, not A/B variants.
- Genre and character remain manual artistic choices.
- LUFS and dBTP targets are automatic and material-aware.
- Processing uses bounded analysis-by-synthesis iterations over the captured audio.
- Apply remains explicit and atomic with Undo; Brain never silently changes the live state.
- When Brain is idle, applied or cancelled it performs no FFT, trial render or background optimisation. Capture and render buffers, FFT objects and trial graphs are released. Only the small report/proposal state remains.
- No STEMS.

## Decision architecture

1. Capture and measure input loudness, sample/estimated true peak, crest distribution, transient density, short-window dynamics, 60-band spectrum/persistence, stereo width/correlation and clipping.
2. Derive bounded automatic targets from genre, character and the measured ability of the fragment to tolerate gain reduction. Targets are reported as decisions, not exposed as user knobs.
3. Build a conservative initial chain from actual modules. The fixed Final Maximizer is always the only final gain computer; legacy limiter parameters are never proposed.
4. Render the captured fragment through the exact proposed graph and measure output.
5. Score target error, true-peak excess, excessive gain reduction, crest loss, spectral over-correction, stereo/mono risk and non-finite/clipped output.
6. Adjust a bounded parameter subset and repeat for at most six iterations, stopping early after two non-improving iterations or when all hard constraints pass.
7. Keep the best valid proposal, release heavy work state, and show one evidence-backed recommendation.

## Safety and quality constraints

- No hard failure may leave changed host parameters.
- Output must remain finite and respect the selected automatic true-peak target in independent oversampled measurement.
- Optimisation may not improve loudness by destroying transient crest or applying excessive limiter GR; style changes bounds, not safety.
- Module decisions must use measured evidence. Unneeded modules remain absent.
- Static EQ, Dynamic EQ and Stabilizer share a tonal correction budget.
- Low-band widening is forbidden; negative low-band correlation can only narrow/mono the affected region.
- Every proposal ID must name a current, audible host parameter.

## Idle resource contract

`MasterBrain` may keep its fixed realtime packet FIFO because it is part of the processor object, but idle capture is a single atomic check and writes nothing. The 20-second stereo capture, preview/render buffers, FFT, analysis detail and trial `SerialGraph` exist only during Pending/Analysing/Optimising and are destroyed on Ready, Applied, Cancelled, Failed, Clear and prepare/reset. The worker thread waits without polling DSP work while idle. Tests must observe zero render/FFT counters after idle callbacks and released heavy-byte accounting.

## GUI

Replace the report-heavy diagnostic page with a premium dark mastering workspace:

- top: Genre, Character, 20 s capture status, Analyse/Cancel and Apply;
- centre: large input/output spectrum and loudness history with detected issues and proposed corrections;
- right: input versus proposed LUFS, dBTP, crest, stereo and GR, plus confidence/constraint status;
- bottom: ordered module chain cards showing why each module is present and its principal change;
- explicit phases: Listening, Building chain, Trial render, Optimising N/6, Verified, Failed;
- no alternate A/B controls, no manual LUFS/ceiling controls, no claims of whole-track analysis;
- live text and meters remain vector/telemetry-driven; rendered assets contain surfaces only.

## Acceptance

- A regression proves Brain writes `truePeakDrive`, `truePeakCeiling` and current Final Maximizer controls, never legacy `limiterDrive`, `limPresent`, `lim` or `ceiling`.
- Synthetic tonal, transient, clipped, anti-phase and already-loud fixtures produce bounded explainable module choices and a verified finite output.
- Iteration retains the best score and never degrades hard constraints.
- 20-second maximum and idle resource contract are automated tests.
- Actual JUCE screenshots at 750, 1000 and 2000 px show no overlap and a legible hierarchy.
- Full core/plugin regressions and exact-SHA Windows VST3 CI pass before packaging is reported.
