# Master Brain — Apply repair, sectional mastering and real A/B (2026-11-06)

Status: implementation on PR #18; **not accepted in FL Studio**.

## User flow
1. Open **Master Brain**, choose target LUFS, peak ceiling, genre/style and allowed processors. Set capture length. 8/16/30/60/120/180/300/600 seconds are supported; recording is from the live DAW input.
2. Start **Analyse audio**, then play the desired continuous song section or whole song from the beginning. Audio stops => capture pauses; to analyse the entire song choose its length up to 600 s. There is no automatic DAW transport-end detection yet.
3. The background worker computes LUFS/peak, stereo correlation, detailed frequency profile and a chronological sequence of ~3-s RMS section measurements. Reports segment contrast, sudden changes, negative correlation, compressed dynamics and comparison against manually curated genre frequency profiles.
4. The worker generates **variant A** (chosen mastering style) and **variant B** (contrasting style), independently renders/validates each using actual DSP on four 4-second samples from start/middle/end of the capture. This is representative SECTIONAL DSP verification, **not a full-song render**.
5. **Audition A/B** places the chosen proposal in the actual VST3 processing chain so it can be heard through FL Studio. Switching undoes the previous audition and applies the other recipe as its own Undo action. No mock audio playback or fake meters.
6. **Apply** commits the currently auditioned proposal or directly applies the selected valid proposal. The UI reports an explicit success or refusal message; Undo should return to the pre-Brain parameter state.

## Safety / compatibility
- No audio-thread allocations, locks or AI inference were introduced.
- Existing preset state, APVTS parameter IDs, DSP engine and PDC remain unchanged; changes are performed on the JUCE UI thread using parameter change gestures.
- Manual parameter edits after analysis disable Apply until a new analysis; review the text report for the reason.
- In this build a "reference" means **hand-authored spectral genre weighting**, not a licensed audio reference matching service or a trained machine-learning model.
- 600 seconds is the upper bound of continuous live analysis. The song must be played into the plugin; no whole-file import or end-of-transport detection has been implemented.
- Host FL Studio acceptance is required in addition to Windows CI, DSP tests and pluginval.

## Regression cases
- Existing 8-second capture, silence, cancel, queue overflow, DSP preview and no automatic change.
- Whole-track section chronological measurements.
- A/B proposals actually differ, and both have independent DSP verification.
- A -> B audition changes actual APVTS values and not just UI state.
- Apply B preserves selected audio settings; one Undo restores pre-Brain values.
- No false success if parameters are stale, proposal is empty or already matches state.
- GUI smoke verifies both buttons, real status report and Undo.
