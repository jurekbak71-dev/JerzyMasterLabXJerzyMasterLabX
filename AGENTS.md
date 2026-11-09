# MasterLabX — coding agent instructions

Current scope: **MasterLabX mastering plug-in without STEMS LIVE or STEMS OFFLINE**.
The binding implementation directive is `docs/MASTERLABX_EXECUTION_DIRECTIVE.md` v1.2.
The previous LIVE STEMS plan in Git history is archival and must NOT be implemented in this branch.

- Preserve and improve current EQ, Dynamic EQ, Dynamics, Master Brain, Stereo, Analog Color, Harmonic Exciter, Bass Architect, Transient Sculptor, True Peak, GUI, mastering and restoration.
- Do not restore source separation, offline stem WAV import/remix, neural workers, ONNX/HTDemucs/StemgenRT runtimes, four-stem controls, installers, or CI.
- Do not require compatibility with older FL Studio projects or historic preset schemas. Current parameters and state round-trip must remain correct.
- Before changes inspect actual C++/JUCE code and build dependencies; keep real-time callbacks free of allocation, blocking, disk/network activity and unbounded work.
- Verify changes using Windows VST3 compilation, CTest, pluginval, parameter/GUI regression, CPU and actual host tests when available. Report failures and unverified items.
- Do not merge to `main` or publish a production release without explicit approval and validated acceptance criteria.

Current removal branch: `feat/remove-stems-live-offline-20261109`. Historical STEMS work remains in `feat/masterlabx-live-brain-20261107` for separate investigation.
