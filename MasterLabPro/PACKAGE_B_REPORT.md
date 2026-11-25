# MasterLabPro Package B report

Status: **IN PROGRESS — do not treat as accepted until CI and artifact evidence below is complete.**

## WYKONANO

- Versioned C ABI, validation, restricted-directory native loader and registry.
- Four independent technical DSP libraries plus two deliberately invalid test libraries.
- Ten-slot serial modular graph with stable instance IDs, nonlinear reorder and bypass representation.
- Atomic reference-module automation, bounded SPSC publication/retirement and stopped-state reset.
- Parameter lane registry, versioned state codec and unavailable-module state retention.
- Global/module analysis taps, latency alignment, audio DELTA and DELTA listen.
- Runtime variable-latency transition and diagnostics.
- A+C GUI console integration and dynamic module screenshot scenario (awaiting Windows CI rendering).

## ZWERYFIKOWANO LOKALNIE

- Direct `g++ -Wall -Wextra -Werror` builds for the non-JUCE tests.
- Independent instance state, nonlinear order, delay impulse/PDC, aligned DELTA and variable latency.
- Six sample rates and eleven block sizes.
- Concurrent automation/topology publication, bounded overload and stopped shutdown.
- Rack state round-trip and missing-module preservation.

## TESTY I POMIARY

Final CTest, sanitizer, Windows, GUI and benchmark results will be recorded after the corresponding GitHub
Actions run. Local measurements are development evidence only and are not substituted for CI artifacts.

## ŹRÓDŁA I LICENCJE

No external DSP code was copied. The implementation is original and uses the project's existing JUCE
dependency.

| Read source | Verified useful point | License/use |
|---|---|---|
| [VST3 Parameters and Automation](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Parameters+Automation/Index.html) | Stable 32-bit parameter assignment; automatable set must not be reconfigured during an instance | Documentation only |
| [JUCE VST3ClientExtensions](https://docs.juce.com/master/structjuce_1_1VST3ClientExtensions.html) | Explicit compatible parameter-ID remapping exists for migrations | Documentation only; JUCE remains the existing dependency |
| [AudioKit](https://github.com/AudioKit/AudioKit) | Layered core/extension packages and graph nodes | MIT; concepts reviewed, no code copied |
| [PiPedal](https://github.com/rerdavies/pipedal) | Real host discovers external LV2 modules; native plug-ins are not a security sandbox | License not relied upon; concepts reviewed, no code copied |

Analogcode, OGFX, OpenRB-338, pingw33n, StereoWidener and Spreader were not uniquely identified and read to
a standard sufficient for technical or licensing claims in this package. They are therefore
**NIEZWERYFIKOWANE** and no code or claimed design was taken from them.

## NIEZWERYFIKOWANE / NIEWYKONANE

- Final Windows x64 CMake/JUCE result, sanitizer jobs, packaged artifacts and rendered screenshot.
- Real FL Studio/VST3 behavior (explicitly Package C).
- Auditory quality of future production mastering modules (outside Package B).

## COMMITY I ARTEFAKTY

To be filled with immutable SHA, workflow run and artifact links after successful CI.
