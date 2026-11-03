# Stereo Architect: first implementation slice

Issue #1; baseline `66926e0`. This is a bounded first PR, not completion of the entire next-generation plan.

## Code-level audit

- `ModulePanel` sliders/buttons use APVTS attachments. `Processor` caches raw atomics in the order defined by `ModuleParameters`; `configureGraph`/`configureModules` load them into both `SerialGraph` banks. Stereo is enabled only when both `stereoPresent` (index 88) and `stereoOn` (89) are true. Removing the module retains settings. The GUI now labels removed, bypassed, active Serial and active Parallel separately.
- Legacy StereoImager splits M and S into four complementary bands. Width 1, Amount 1, centered balance, no audition is neutral; width 2 doubles **existing** Side. For dual mono, S=(L-R)/2=0, so even maximum width has no effect. This is expected arithmetic, not a disconnected slider.
- The 20 ms one-pole Slew controls bypass, widths, band enable, solo, Amount, mono, Delta and now Stereoize. Tests must settle automation before evaluating static endpoints. Solo affects Mid and Side together. Amount 0 or disabled bands return width to unity; they also suppress new generated Side.
- Parallel sums each present branch and divides by branch count, including bypassed present modules. Thus one imager plus one dry branch halves the imager's delta. The integration test reproduces this, including PDC. This PR preserves that gain law; branch trims/mixes are deferred.
- The existing vectorscope and spectrum show **global output**, while four band readings describe Stereo Architect's local signal before balance, solo summation and module bypass crossfade. The four displayed W/C values now expose both fields from the DSP snapshot FIFO. They are not measurements of each branch at the final master output.

## Implemented DSP

An appended `stereoizeAmount` parameter (index 162, range 0–1, default 0) creates optional Side from Mid. Two 300 Hz high-pass poles protect low frequencies, followed by a 700 Hz first-order all-pass phase rotation. The resulting signal uses the existing complementary split; its lowest band is omitted. Other bands receive up to 0.5 times the generated signal, scaled by Stereoize, Amount, band enable and width. Mono preview suppresses it. This is deterministic phase rotation, not a claim of statistically independent ambience or instrument recovery.

Dry Mid remains unchanged. At centered balance, no solo/Delta and before any downstream nonlinear processing, `(L+R)/2` is preserved to floating-point tolerance. Mono-bass is the existing optional control for **input Side**, with its legacy default retained. Newly generated Side is bass protected regardless of that toggle. The bass filter is a slope, not a brick wall.

No delay buffers, allocation, locks, FFT or worker tasks were added to DSP. All-pass/filter history and coefficients are fixed-size; coefficients are prepared once. Existing 5 ms host PDC is unchanged. Width-only paths retain their previous arithmetic with Stereoize zero.

Stereoize can increase peaks and stereo loudness, and its additive phase interaction can also narrow some already-stereo signals. There is deliberately no hidden gain normalization or forced widening. The evidence renderer level-matches using the project's integrated LUFS meter. Use Stereoize on mono/narrow input and compare Mono preview; it is optional sound shaping, not a transparent universal mastering setting.

## Presets and automation

Schema 7 appends one ID; all previous 162 IDs, indices, ranges and defaults remain unchanged. Schema 1–6 input is validated at its original parameter count, then extended with factory defaults, including Stereoize=0 even if the receiving plugin previously had it enabled. Schema 7 requires 163 parameters. Old software cannot load schema 7; backward loading of old `.jmlx`/host states is supported, forward loading into an old binary is not promised.

State tests now exercise migration 1–6; preset roundtrip/Undo/Redo includes Stereoize, and module integration renders exercise it with allocation auditing. Windows VST3 CI already builds and runs CTest and actual pluginval on every PR; the new targets are included automatically.

## Hybrid Studio increment and next layout

Implemented: attached Stereoize control beside Amount, explicit routing/bypass status, per-band measured width and correlation, negative global correlation prompt, existing live output spectrum/vectorscope and Mono/Delta/solo controls retained. GUI smoke checks the new slider reaches APVTS and resizes/reopens the actual editor.

Next panel iteration: top chain/presence/bypass strip; left four colored bands with W/C and separate input-Side/output-Side telemetry; right output vectorscope and named mono comparison; bottom Width / Stereoize / gain-match controls. Add signal-valid flags so silence is displayed as unavailable, not correlation zero. Add explicit branch count/dilution and input scarcity hints. Continue dark high-contrast theme and bounded FIFO snapshots; do not move audio analysis into the paint callback. Clean/Strong/Extreme maps, depth/microdelay modes, full HiDPI/accessibility work and GUI design system remain pending.

## Verification evidence

See [local results](evidence/VALIDATION.md), [matched WAV and measurement notes](evidence/README.md) and the [actual GUI screenshot](evidence/hybrid-stereo-linux.png). Windows CI status remains in the PR checks; FL Studio is pending.

## Reproduce

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
# Linux: ./build/stereo_tests evidence
# Windows: ./build/Release/stereo_tests.exe evidence
```

`stereo_tests` generates original deterministic synthetic signals; no third-party music is redistributed. Input kinds: 0=dual mono (mono represented on the supported stereo bus), 1=narrow stereo, 2=conventional stereo, 3=opposite-phase 45 Hz bass. WAVs use 48 kHz stereo PCM16. Each wet WAV is normalized to the corresponding input integrated LUFS; raw-output metrics are printed before normalization. The `legacy_width` column uses all four legacy widths at 2; `new_width` uses widths at 1 plus Stereoize=0.7. These are diagnostic settings, not an equal-setting old/new comparison. The matched delta is RMS(wet_matched-dry)/RMS(dry), measured after one second. Four seconds of stationary synthetic audio is not proof of perceptual musical quality.

## Explicitly pending

FL Studio playback/save/reopen/export/freeze, listening on rights-cleared music, blind audibility evaluation, formal phase/magnitude sweeps, THD+N/aliasing and intersample/true-peak validation, exhaustive silence/denormal and long-running CPU/RAM soak, complete Hybrid Studio design and character modes. No claims about other DSP families or ML completion. Issue #1 stays open.
