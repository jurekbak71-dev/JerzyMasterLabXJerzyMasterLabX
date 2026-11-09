# MasterLabX — LIVE safety review (2026-11-09)

Scope: source inspection only, not a completed implementation or runtime test.

## Inspected
- `Source/Processor.h` on `main` and `feat/masterlabx-live-brain-20261107` (first 75 lines of feature branch).
- `Source/LiveStemWorker.cpp` on feature branch (full retrieved source).
- `docs/superpowers/plans/2026-11-07-masterlabx-live-stem-brain.md`.
- Notion project record: https://app.notion.com/p/4115fed1901b81c4a099d97f56279132
- JUCE AudioProcessor API: https://docs.juce.com/master/classjuce_1_1AudioProcessor.html

## Confirmed source observations
1. The feature branch adds `LiveStemEngine`, `LiveStemWorker`, `StemgenRtBackend`, asynchronous model loader and diagnostics, unlike the inspected main processor header.
2. `LiveStemWorker::run` assembles 128-sample hops, calls the backend on its worker thread, checks epoch/position, rejects non-finite outputs and skips expired hops. This is source-level evidence, not proof of stable real-time execution.
3. `LiveStemWorker::~LiveStemWorker` requests stop and wakes the engine; verify object destruction order, blocked inference, and lifetime of `engine_` before release.
4. Existing Dynamic EQ on main declares three bands. Do not repurpose existing parameter IDs when extending.

## Open critical verification gates
- Reproduce reported FL Studio LIVE crash with matching PDB/minidump; map RVA to symbols. No root cause established.
- Audit loader/worker/editor ownership across `prepareToPlay`, state restore, activation and teardown. Test rapid toggle, reload and editor close while inference is active.
- Test audio-thread allocations, locks, stale frames, variable blocks, seek/loop and offline export at 44.1/48/96 kHz; measure deadline misses and PDC.
- Validate missing model, non-commercial model weight license, dry fallback continuity and old preset migration.
- Run Windows CTest, pluginval, build and real FL Studio acceptance before merging or releasing.

## Status
REVIEW ONLY. No DSP source changes, builds, tests or host validation performed in this review. Preserve prior working artifacts and do not merge based on this report alone.
