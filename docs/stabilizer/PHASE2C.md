# Stabilizer Phase 2C

This slice implements Issue #3 Phase 2C after the Phase 2B Windows gate passed. It does not start later roadmap work.

## DSP audit and behavior

The existing Stabilizer is a stereo-linked 12-band adaptive equalizer. It measures octave-band energy, derives a Neutral/Electronic/Acoustic target, spectrally smooths the result, slews 12 bell filters and supports Shape, Cut, region amounts, transient protection and exact Delta audition. Its old correction was bounded to ±6 dB before Amount and region scaling.

Phase 2C appends an explicit authority profile:

- **Legacy** keeps the previous arithmetic and ±6 dB bound exactly.
- **Clean** uses 0.75x correction authority and a ±4 dB hard bound.
- **Strong** uses 1.35x authority and a ±9 dB hard bound.
- **Extreme** uses 1.8x authority and a ±12 dB hard bound.

The three new profiles scale correction with spectral-support confidence. A concentrated single tone therefore cannot drive Extreme to its 12 dB limit. With Tame enabled, a continuous crest-based factor reduces adaptive motion around transients instead of switching abruptly between full and quarter strength. Silence and near-silence decay toward zero correction. Cut mode retains a 0 dB upper bound in every profile.

No profile adds latency, hidden output gain, saturation or a learned/reference model. Strong and Extreme can raise peaks through boosts, so the downstream limiter and level-matched audition remain necessary.

## Hybrid Studio

The Stabilizer workspace adds the four-position authority selector beside Shape/Cut and the tonal profile. The plotted response is calculated from the actual smoothed DSP filter coefficients and bypass blend delivered through the processor snapshot. The captured evidence view uses Strong at 85% Amount.

## Preset compatibility

Schema 10 appends `stabilizerIntensity` after the 179 schema-9 parameters. Its default is Legacy. Schemas 1–9 migrate at their historical counts and receive that default. A same-compiler comparison against Phase 2B commit `d45f912` rendered Neutral, Cut/Electronic and Acoustic scenarios at 44.1/48/96/192 kHz. All twelve output digests match exactly; the digest-list SHA-256 is `a8c32c7b26bb64941e3db7fe947ad2c5e107a85f703fc9c14dbc7bb95388fa3c`.
