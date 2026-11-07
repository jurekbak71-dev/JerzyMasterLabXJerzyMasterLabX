# MasterLabX GUI — Concept C / implementation ledger
Date: 2026-11-06
Status: active WIP, **NOT yet production GUI**. Approved visual target: powder-coated nearly white, metallic pre-rendered knobs, blue/green/red LEDs.
Canonical spec: [CONCEPT_C_POWDERCOAT_BUILD_SPEC.md](CONCEPT_C_POWDERCOAT_BUILD_SPEC.md).

## Actual code on PR #18
- CMake generates and embeds two 101-frame knob atlases, six coloured LED variants and the powdercoat tile through JUCE binary data; Windows CI installs Pillow.
- `StudioTheme::drawRotarySlider` selects normalized real slider value -> frame 0–100; PNG source region drawn via JUCE. Existing APVTS attachments and host automation remain in place.
- Toggle buttons consume state-dependent green/blue LED sprites; the Final True-Peak panel also displays a red LED only if measured output sample peak exceeds 1.0 (this is **not** independent 8x ISP compliance).
- `Editor.h` uses lighter powder-coated main chassis and brand accents. Seven module subpanels have light side rails and dark inset hardware displays.
- CTest includes generator asset-pack integrity check.

## Not yet done
- The GUI visual layout still resembles the pre-Concept C layout in many views; full card geometry, module navigation, complete master console composition and responsive ergonomics require further changes.
- Starter sprites are deterministic procedural renders, not final polished Blender source assets from Concept C's visual reference.
- Real VU hardware bezels, per-module lamp semantics, screen hierarchy and all panels require visual comparison against reference.
- Windows build/pluginval and FL Studio visual acceptance after the latest changes need direct confirmation from latest CI and actual host.

## Non-regression constraints
- Preserve plugin ID, JUCE APVTS IDs, every preset schema, current DSP, six-family routing and 128-sample True-Peak PDC.
- Never mark 4x detector as standards-certified just because sample clipping LED is green.
- Never claim A/B, transport, AI, six-band EQ or user-facing oversampling is implemented just because image design shows it.
- Do not merge PR #18 before Windows CI and user acceptance of real output.
