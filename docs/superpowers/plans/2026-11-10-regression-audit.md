# MasterLabX regression audit and repairs

Goal: verified Windows x64 VST3, with truthful GUI/DSP evidence and no merge to main.
Architecture: retain current C++20/JUCE 8.0.6 graph and host IDs; isolate reproduced defects with deterministic regression tests. The 2026-11-10 user confirmation excludes STEMS LIVE/OFFLINE. REBALANCE is separate.
Spec: docs/codex-handoff/2026-11-10/CODEX_MASTERLABX_FULL_IMPLEMENTATION_PROMPT.md on docs/codex-masterlabx-audit-gui-dsp-20261110.
Execution: inline in the current session; no background automation.

- [x] Read handoff, Notion decisions and branch AGENTS; inspect PR 18/19/20/21 and fetch actual implementation head 2e1c68a.
- [x] Clean Release DSP baseline, 31/31 CTest; verify uploaded assets by hash/CRC.
- [x] Reproduce final OFF leaving old limiter active (FinalLimiterOffTests); fix GraphConfiguration; 32/32 CTest.
- [x] Reproduce callback-dependent scope data (ScopePartitionTests); continuous sample-clock capture, rising trigger, time scale, clipping in Bass/Transient/Final panels.
- [ ] Validate new scope reset, silence, aligned input/output and release/sanitizers; full test suite.
- [x] Compile actual JUCE plugin, run existing state/automation/GUI tests and inspect screenshots at three sizes.
- [x] Record module matrix and remaining gaps: exciter anti-aliasing/models, restoration quality, complete routing/Brain, MASTER/INSERT latency, telemetry and GUI layout.
- [ ] Windows build, CTest, pluginval and packaging on the exact repair commit; verify PE/ZIP/dependencies/checksums.
- [ ] Complete all still-open specification items and user FL Studio/audio/visual acceptance before any production claim. No merge without approval.

Review focus: final OFF must override old flags; sub-64-sample callbacks must not create fabricated zero samples in scopes; input/output traces must reference the same time; silence/reset must remove old traces; UI snapshots do not establish listening quality or FL Studio stability.
