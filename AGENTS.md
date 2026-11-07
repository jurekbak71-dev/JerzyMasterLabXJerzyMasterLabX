# AGENTS.md — MasterLabX AI coding contract (work branch)

## Active objective
Implement the user-approved Live Stem Brain project. Read `docs/superpowers/plans/2026-11-07-masterlabx-live-stem-brain.md` first and execute packages A through F in order, with independently verified commits and tests. Base branch is `feat/phase5f-hybrid-3d-controls-20261106`, primary tracking PR is #19; do not merge to main without explicit approval.

## Critical user requirements
- True live AI music source separation **from incoming FL Studio mixer audio**, not only WAV import or frequency-band splitting.
- Removable/reconfigurable four stems (drums, bass, other, vocals), each with independent mastering-quality EQ, dynamics, stereo and gain controls.
- Mouse-operated EQ nodes with stable VST3 automation ID assignments; user can add and delete bands.
- Master Brain can choose ALL supported internal modules MANUAL/AUTO/HYBRID, with locks, real preview, Apply, Undo.
- At least +15% safe useful effect authority where applicable, tested at matched loudness; do not apply a blind 1.15 multiplier to all parameters.
- Current professional mastering and audio restoration research must directly inform implemented algorithms; respect licenses.
- Final target: installable Windows x64 VST3, model/runtime included, FL Studio verified; preserve existing DSP and presets.

## Engineering safety / compatibility
- Plugin identity, host-visible existing APVTS IDs and state migration MUST remain compatible with the current working build.
- Audio callback: never allocate, wait on network/model/disk, lock mutexes or spawn child processes.
- Sample-accurate timeline: handle DAW seek/loop, stop/start, different block sizes and sample rates.
- When inference cannot meet real-time budget, report measurable latency/real-time factor and make a deterministic click-free dry fallback. Do not label offline mode as live.
- Do not claim FL Studio testing based solely on CI or pluginval.
- Keep GUI Concept C design decision and working base state intact.
- Do not touch, release or merge unrelated project branches.

## Tools and checkpoints
- Prefer Superpowers skills: writing-plans, test-driven-development, systematic-debugging, verification-before-completion, requesting-code-review.
- Use Context7/current JUCE docs and official Steinberg VST3 API docs when unsure; verify APIs against JUCE 8.0.6 used by this repository.
- For each task: write a failing test -> demonstrate RED -> minimal fix -> demonstrate GREEN -> run relevant regression tests -> commit.
- Windows workflow `.github/workflows/windows.yml` plus local DSP/GUI tests must pass before labeling milestone complete.
- All model binaries/data require license and redistribution review.
- Deliver quantitative latency, CPU, RAM, SDR/leakage and True Peak evidence where appropriate; reference the test input and reproducible commands.

## Current state (2026-11-07)
- Current release separates offline WAV via native HTDemucs; `Source/StemBrainPanel.h` invokes `stems-split.exe` and `Source/StemRemix.h` plays a precomputed four-stem bank.
- This branch added `Source/LiveStemQueue.h` and `tests/LiveStemQueueTests.cpp`: a bounded SPSC frame queue **not yet wired to DSP**, with tests for concurrency and overflow. Do not mistake this for completed live separation.
- Plan and status: `docs/superpowers/plans/2026-11-07-masterlabx-live-stem-brain.md`.
