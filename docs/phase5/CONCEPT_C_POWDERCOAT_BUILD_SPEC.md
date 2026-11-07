# MASTERLABX — CONCEPT C / PRODUCTION GUI BUILD SPEC
Status: design approved by user 2026-11-06; implementation NOT approved as completed.
Visual reference: conversation image "Concept C — Powder-Coated Mastering Console", 1448x1086 (4:3 concept only).
Scope: Windows x64 VST3, JUCE, FL Studio. Visual/UI refactor; preserve DSP and preset compatibility.

## 1. Design contract
- Not a recolor of legacy StudioTheme: reconstruct chassis, controls, navigation, panels, spacing, type, lighting and responsive geometry.
- **Material:** nearly white powder-coated steel, faint fine-grain microtexture, beveled silver edges and recessed charcoal windows, screws/vents.
- **LEDs:** green = powered/ready, red = true-peak/clip/critical indication, vivid saturated police-beacon blue = selection/active focus or major control; no arbitrary strobing or flashing.
- Silver knurled metallic knobs are **pre-rendered frame atlases** (NOT simple JUCE circle/gradient in drawRotarySlider). Render normal, hover, disabled and focus overlays.
- The image is a VISUAL goal, NOT a literal feature specification: no dummy oversampling selector, transport/playback buttons, fake analyzer values or invented DSP controls.
- Render data-driven live output only; screen and markings must use real APVTS and telemetry.

## 2. Production geometry
- Source design coordinate system 1440x1080 (reference 4:3); keep adaptive viewport, do NOT set a fixed 1440x1080 editor requirement.
- A layout guide, not hardcoded absolute positions: header 0–8%, first module rack 8–45%, center spectrum/meter workbench 45–70%, lower rack 70–92%, bottom status rail 92–100%.
- At narrower windows use a component stack or tab navigation; never shrink knobs below useful pointer targets. Preserve current host resize limits and resizer behavior.
- Retina/HiDPI: provide 1x/2x resources; define asset scaling and prevent pixel bleeding. Graphic windows are separate live JUCE drawing surfaces.

## 3. UI sections / truth-in-functionality
- Header: brand, preset browser, A/B if real, bypass, input/output meter from host.
- Main rack: Spectral EQ, Dynamics, Stereo Architect, Limiter/Final True-Peak.
- Analyzer: measured spectrum, EQ curve, loudness, stereo meters, correlation using actual live DSP/telemetry.
- Lower rack: Analog Color, Bass Architect, Transient Sculptor, Stabilizer, Master Brain. Include Harmonic Exciter and all other real modules via navigation, not hiding/removing them.
- Six-family serial/parallel processing graph and all existing modules stay functional, even if graphical previews show simplified controls.
- The present plugin has a 16-band EQ; concept's "6-band" label is illustrative and must NOT replace that 16-band function.
- Current True-Peak fixed host PDC=128 samples and existing parameter IDs/state schema must remain unchanged.
- Oversampling "4x" in image is visual ONLY; expose only if a real parameter and safe fixed PDC exist, otherwise omit.
- Master Brain is existing rules-driven analysis, not a trained AI model; never present mockup promises as implemented.
- No new transport/play/stop unless host-supported and implemented; do not introduce decorative fake switches.
- All light states must reflect the underlying parameter (automation included), and red must correspond to a documented warning threshold.

## 4. Asset production and naming
Repository target: `assets/gui-concept-c/`; generator: `tools/gui_assets/` (new files only after implementation).
- powdercoat/frame: reusable 9-slice chassis corners/edges, header and panel tile, 1x/2x.
- knob-medium and knob-large: 101 RGBA frames each over 270° rotation; index = round(normalized APVTS value*100). Keep exact identical knob center/footprint for every frame. Never bake label or parameter reading into frame.
- LED rings separate RGBA overlays in green/red/blue/off for param status and indicators; blue has a static glow rather than moving beacon (audio plugin accessibility).
- toggle-on/off, push-on/off, selector tabs, screw, ventilation strips, analog meter frame and needle independent.
- Assets must be authored under clear names: `knob_medium_000.png` ... `knob_medium_100.png`, likewise large; LED masks sized to corresponding knob assets.
- Avoid image-model-generated individual knob frames: inconsistent pivot. Use deterministic renderer with fixed camera/light and rotating indicator only, or Blender rig with locked scene/camera.
- Record source renderer settings and package generated RGBA PNG plus source (.blend or generator code), license/origin, and manifest with each asset's pixel dimensions, pivot, frame count, variants.

## 5. JUCE integration architecture
- Keep existing `Source/Editor.h` / `StudioTheme.h` and panels as reference until new component layout has passed gate.
- Implement resource-backed `RenderedKnobLookAndFeel` (or custom `RenderedKnob`) from embedded BinaryData image atlas; sprite drawing on message thread, APVTS attachments remain on each original Slider.
- Map normalized position with each actual slider's skew/range and current value (JUCE provides normalized position); verify drag, wheel, keyboard, double-click, text entry, automation and focus.
- Build reusable `PowderCoatPanel`, `LedStatus`, `AnalogGRMeter`, `ModuleCard`, `MeterWindow`, `MasterLabXChrome`.
- Meters and analyzer render measured telemetry; cache static backgrounds, do not allocate or decode PNG per paint callback.
- Do not touch audio thread, DSP graph, parameters, plugin identity, PDC or preset migrations.

## 6. Staged implementation
1. Produce reproducible 3D asset kit and visual golden references (no JUCE edits).
2. Integrate pre-rendered knob sprites on a temporary component/isolated GUI sample; test all existing controls and HiDPI.
3. Replace global chassis/header and navigation with Powder Coat white, ensuring complete panel switching and resizing.
4. Rebuild every module panel with shared card shell and real APVTS controls, including 16-band EQ and every secondary page.
5. Rebuild analyzer, VU/GR and LED windows with actual telemetry and thresholds.
6. Run Windows x64 CI, CTest, GUI open/resize smoke, pluginval strictness 5, screenshot comparisons, then FL Studio interactive acceptance.
7. Merge only after visual approval and host validation. No silent merge.

## 7. Acceptance criteria
- Existing GUI is no longer visible behind or within the redesigned interface at normal view and all subpages.
- Golden screenshot comparisons at minimum 3 supported sizes and both 100%/150% Windows scaling.
- Each visible control changes its real parameter in plugin and reflected host automation; loading old projects gives identical DSP output for identical states.
- All icons, screws, knobs and LEDs come from produced render assets, not merely repainting JUCE vectors; charts and live text remain JUCE-driven.
- No clipping/overlap, nondeterministic image frame jitter, meaningless LED animation, new audio-thread allocation or pluginval failure.
- Separate evidence: automated tests != manual FL Studio visual approval.

## 8. Historical PR #18
Old Phase 5F commits are vector cosmetics, NOT the approved C build. This document supersedes its previous visual direction without claiming the code was replaced.
