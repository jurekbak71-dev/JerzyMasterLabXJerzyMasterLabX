# Smart Dynamics Phase 2B

This slice implements Issue #3 Phase 2B and does not change Stabilizer behavior.

## Audit and behavior

The original compressor already had a stereo-linked peak detector, continuous knee, downward and bounded upward compression, makeup, a real output-domain dry/wet blend and Digital/Analog character. Its default ranges could produce substantial gain reduction, but there was no explicit intensity profile, transient control or Delta audition. Analog uses the existing mild DC-neutral arctangent color without oversampling; Phase 2B does not increase that color or claim hardware emulation.

Phase 2B appends three controls:

- **Intensity**: Legacy, Clean, Strong and Extreme. These explicitly scale downward/upward dynamic gain only; they do not add hidden makeup. Legacy is exactly the old equation. Clean uses 0.8x downward and 0.85x upward action, Strong 1.4x/1.2x, Extreme 1.9x/1.45x.
- **Punch**: compares a fast transient envelope with the main user-controlled envelope and can restore up to a bounded 12 dB transient difference. The amount is 0–100% and is smoothed through the gain stage.
- **Delta**: outputs the exact post-Dry/Wet processed contribution minus input, with the existing 20 ms transition.

The Hybrid Studio panel plots the selected intensity transfer curve and records real downward reduction, upward gain and Punch telemetry from processor snapshots.

## Compatibility

Schema 9 appends `dynamicsIntensity`, `dynPunch` and `dynDelta` after all 176 schema-8 parameters. Their defaults are Legacy, 0% and off. Schemas 1–8 migrate at their historical counts. A same-compiler comparison against Phase 2A commit `682b9c8` rendered Digital, Analog+63% mix and Upward scenarios at 44.1/48/96/192 kHz. All twelve digests match; the digest-list SHA-256 is `a9bffa11b6c3a1f0e97de1f379d85c3349864f8c6197ef5a7d5ea626db07ff04`.

## Limits

Strong and Extreme can create large level changes and higher peaks when Punch is used. They require output-level comparison and downstream peak control. The limiter remains sample-peak. Analog is not oversampled, so the measured harmonic color is retained at its previous mild level instead of being expanded. Manual FL Studio validation remains pending until the Windows artifact is tested in that host.
