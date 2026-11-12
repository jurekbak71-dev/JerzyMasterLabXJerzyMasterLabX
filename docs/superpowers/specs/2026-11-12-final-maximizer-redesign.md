# Final Maximizer redesign

## Intent

Replace the current crowded rotary-control limiter panel and repair the final
stage so that its visible state always matches the active DSP. The result must
be a mastering maximizer that can increase density and loudness without relying
on an audible final hard clamp.

The selected visual target is the first generated concept, **Modern Metering
Console**, displayed in the 2026-11-12 design review.

## Goals

- Reproduce the selected layout in the existing JUCE editor, preserving the
  surrounding MasterLabX shell and module navigation.
- Use linear faders/sliders with exact numeric readouts; remove the rotary
  controls and decorative analogue GR meter from this panel.
- Make `OFF`, `ACTIVE`, missing-stage, true-peak-risk and clipping states
  visually unambiguous and driven by real processor state/telemetry.
- Provide a large measured history view for limiter attenuation, compressor
  contribution and transient events, plus IN/GR/OUT meters and peak holds.
- Add a controlled pre-limiter density/compression stage, oversampled soft
  clipping and a final lookahead true-peak safety stage.
- Prevent the Exciter-to-Maximizer handoff from producing uncontrolled peaks or
  alias-heavy clipping at supported settings.
- Preserve fixed, correctly reported host latency and real-time safety.

## Non-goals

- No copy of iZotope branding, IRC algorithms, artwork or trade dress.
- No STEMS or REBALANCE work.
- No MASTER/INSERT redesign in this stage.
- No merge to `main` and no production release without explicit approval.

## Selected layout

### Status row

- Fixed Final Slot indicator.
- One authoritative power control derived from both presence and enabled state.
- `Clean`, `Punch`, `Dense` processing modes.
- `2x`, `4x`, `8x` quality selector.
- Auto Release and Delta controls.

### Primary controls and display

- Left: vertical Input Gain and Density/Compression faders.
- Centre: large scrolling display with downward limiter GR, upward/density
  contribution, transient markers and IN TP / OUT TP / GR / LUFS-S readouts.
- Right: IN L/R, GR and OUT L/R meters, peak holds, Output Ceiling fader and
  `SAFE / TP RISK / CLIP` state.

### Lower control row

- Character, Release, Soft Clip, Transient Emphasis, Stereo Link and Compressor
  Mix as horizontal sliders with editable values and units.

## Rendered assets

Render reusable transparent assets separately from live text and telemetry:

- vertical and horizontal fader caps in normal, hover, pressed and disabled
  states;
- fader tracks and meter segment overlays;
- power, Auto, Delta and status indicator surfaces;
- mode/quality segmented-selector surfaces;
- display and numeric-readout bezels.

Text, values, graphs, meter fills and state colours remain native JUCE drawing
so that scaling, automation and accessibility do not depend on baked bitmaps.
Assets require 1x and 2x outputs; the 2x source is authoritative for downscale.

## DSP architecture to validate test-first

1. Repair state authority: a green/active UI state is permitted only when the
   fixed stage is present and enabled. State restore must configure DSP before
   the first processed block without requiring a toggle cycle.
2. Pre-limiter density stage: low-ratio, soft-knee stereo-linked compression
   with bounded gain contribution and gain-matched wet/dry control.
3. Oversampled soft clip: placed before the final detector, with antialias
   filtering and no post-detector nonlinear processing.
4. Program limiter: mode-dependent envelope behaviour for clean, punch and
   dense operation, with continuous gain transitions.
5. Final safety: independently measured output true peak at or below the chosen
   ceiling within the documented test tolerance, without using a routine hard
   clamp as the normal gain-control mechanism.
6. Exciter handoff: bound and measure peak growth; keep the nonlinear Exciter
   oversampled in production quality and verify its output into the Maximizer.

Exact coefficients and time constants are implementation choices selected from
measured tests, not visual-design commitments.

## Approaches considered

1. **Modern Metering Console — selected.** Strongest signal hierarchy and
   fastest diagnosis of GR, true peak and clipping. It changes the current panel
   most, but retains the MasterLabX shell.
2. **Exciter Family Hybrid.** More decorative metal/amber treatment and closer
   family resemblance, at the cost of weaker measurement priority.
3. **Precision Mastering Desk.** Maximum meter area and minimal decoration, but
   too visually detached from the surrounding application.

## Acceptance evidence

- A regression test first reproduces restored/default `ACTIVE` UI state while
  DSP is absent; the fixed implementation passes without a toggle cycle.
- Automated audio tests cover silence, impulse, sine, near-Nyquist, dense mix,
  bass-heavy material and Exciter-fed signals at all sample rates and callback
  partitions currently supported by the project.
- Independent oversampled output measurement verifies the true-peak ceiling;
  results include worst case and tolerance rather than a blanket certification
  claim.
- Tests quantify aliasing, overshoot, pumping/recovery, discontinuities,
  latency, block invariance, bypass transitions and non-finite safety.
- GUI smoke verifies power-state truth, fader automation/gestures, value entry,
  resizing and actual live telemetry.
- Final Windows x64 VST3 build and pluginval are required before any completion
  claim.

