# MasterLabX Concept C — rendered glass and LED controls
Date: 2026-11-06. Status: implementation in PR #18; **no host acceptance yet**.

## Confirmed design rules
- Physical 48-LED ring with a 270-degree travel around each rotary control.
- Arc frame 0..100 is selected from the real JUCE Slider normalized 0..1 APVTS value, not a time animation. Lit LEDs before pointer; remaining LEDs dark and visible.
- When a control is disabled: render frame 0 (all unlit), keep real knob image in disabled opacity.
- A small RED LED is illuminated if and only if the corresponding module is present and enabled; inactive modules have an UNLIT red diode. Master Brain is analysis/navigation, not an invented audio enable parameter, so no fake illuminated LED.
- Active module background edge glow uses separate PNG; bypassed cards use a dim/non-lit asset.
- Displays have a separate dark glass base and reflective transparent overlay; real text and FFT/EQ curves are composed between them.
- Switches have separate ON and OFF assets, preserving original button listeners/APVTS.
- No changes to DSP, state IDs, PDC, preset schema or audio callback.

## Reproducible assets generated on CMake configuration
Run:
`python tools/gui_assets/render_concept_c.py output_dir`
`python tools/gui_assets/validate_concept_c.py output_dir`

New independently addressable assets:
- `led_wreath_96_atlas.png`, `led_wreath_160_atlas.png`: 101 frames, 48 segments, 11 columns by 10 rows. 
- `glass_analyzer.png`: dark display body with recessed rim.
- `glass_overlay.png`: transparent reflectance layer on top of live instrument readouts.
- `glass_readout.png`, `glass_readout_overlay.png`: glass-backed numeric value widgets.
- `module_glow_on.png`, `module_glow_off.png`: edge emissive overlays.
- `switch_on.png`, `switch_off.png`: rendered pushbutton images.
- Existing assets: knob 96/160 atlases, red/green/blue indicator sprites, powdercoat and analog GR bezel.

A separate user-facing asset ZIP contains a more extensive, independently inspectable 37-file procedural RGBA kit, manifest hashes, source generator, a demo HTML page, and two visual contact sheets. The ZIP exists in the current conversation sandbox, **not in the Git repository**; the repo generator outputs a compatible subset for CMake. Do not claim the two sets of images are byte-for-byte identical.

## Technical validation gate
- CMake resource pack generation and validation must succeed under Windows Python/Pillow.
- JUCE rotary ring must visibly light the correct prefix and dim inactive module LED, at all supported sizes.
- GUI smoke must cover actual APVTS/automation state, status diodes and glass, with screenshots across 750x550, 1000x700, 1400x1000.
- Windows x64 Release VST3, 42+ CTest, strict pluginval and FL Studio visual inspection required.
- PR remains Draft until final render style is accepted.
