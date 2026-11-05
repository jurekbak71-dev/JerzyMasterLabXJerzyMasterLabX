# MasterLabX — proposed new mastering processors (not yet approved for implementation)
2026-11-05 | Planning document, not shipped features.

## Priority after finishing Phase 4B / Phase 4C
1. **Bass Architect** — 3 operating areas: sub (<90 Hz), body (90–250 Hz) and upper-bass attack; bass harmonic reinforcement without unnecessary sub boosting, low-frequency mono/correlation, phase-aware HP side and transient preservation. Add actual DSP spectral/loudness gain meters; don't use unvalidated phase rotation.
2. **Transient Sculptor** — attack/sustain shaping with three perceptual bands, zero- or declared-latency options, transient detection based on smoothed envelopes (no false claims of mastering-grade lookahead without a real lookahead buffer), parallel blend and true Delta listening. Multirate impulse and null tests.
3. **Precision De-Esser / Harshness Control** — dynamic bands in 2.5–12 kHz, FFT or envelope detector, threshold/ratio, selectable Mid/Side if robust, true band audition; preserve cymbals and avoid excessive pumping.
4. **True-Peak Output Limiter** — real oversampled inter-sample peak detection and oversampled true-peak metering, lookahead with explicit PDC, ceiling and verified ISP prevention. Existing limiter is **sample peak**, and color stages after it can exceed ceiling. This is a release-critical quality/safety improvement, not a cosmetic rename.
5. **Psychoacoustic Detail Enhancer** — restrained but adjustable spectral detail, microdynamics, stereo field checks and matched-loudness audition. Avoid doubling up existing Stereo Architect, Spectral EQ or Harmonic Exciter functions.

## Ordering and compatibility
- Finish and host-validate Phase 4A Impact and Phase 4B smooth transitions before adding more active processors.
- Make 2x/4x oversampling production-ready with fixed/well-managed plugin latency, matched Dry/Wet, true bypass and CPU profiling.
- New processors are individually bypassable and default off in old presets; append APVTS IDs without reinterpreting the six-family `chainOrder` 720 encodings. A new versioned placement/routing representation requires separate engineering and compatibility approval.
- Collect objective harmonic / alias / gain / latency metrics, loudness-matched WAV samples and Windows x64 VST3 test artifacts. Require user FL Studio confirmation before merging.
- Master Brain should be able to recommend/reason about all new processors only after they have validated DSP. Do not label simple style presets as AI reference mastering.

These are **proposed module directions**, not a promise they are implemented. The user may reprioritize them.
