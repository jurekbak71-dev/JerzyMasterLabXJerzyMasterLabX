# MasterLabX autonomous coding agent — execution contract

Repository: jurekbak71-dev/JerzyMasterLabXJerzyMasterLabX
Working branch: feat/masterlabx-live-brain-20261107.
Never change main, merge pull requests, publish releases, or delete existing features.
Follow repository AGENTS.md and docs/superpowers/plans/2026-11-07-masterlabx-live-stem-brain.md.
Treat user-reported problems as reports requiring reproduction, not proof of a specific cause.

Goal: incrementally complete a professional Windows x64 VST3 for FL Studio with:
- stable, actually neural LIVE STEMS and high quality offline separation;
- four independent stem processing chains and real-time controls;
- 11-module routing plus MASTER BRAIN MANUAL/AUTO/HYBRID;
- Analog Color, Harmonic Exciter, Analogizer tube/transistor/tape based on Jerzy Tape Drive where licensed and appropriate;
- expanded Dynamic EQ, before/after spectrum, heatmaps;
- Restoration/Denoiser, two real MASTER/INSERT modes;
- backward-compatible automation, presets, Concept C GUI and final installer.

Each invocation is limited to ONE independently testable improvement, prioritized:
1. Fix reproducible build or test failures at the current revision.
2. Resolve live worker deadline/fallback/transport failures with measured evidence.
3. Improve real neural source separation quality (no fake frequency-band substitutes).
4. Proceed through per-stem DSP, routing, analog DSP, EQ, restoration and packaging.

Mandatory process:
1. Inspect source and current tests; identify a precise root cause.
2. Add a failing regression test when feasible. Run it (RED), then implement minimal correction.
3. Run focused tests (GREEN) and related regressions. On Linux, use CMake with
   -DMASTERLAB_PLUGIN=OFF for native DSP tests if Windows/JUCE environment unavailable.
4. Record exact commands, results and limitations. Never claim host/Windows/audio
   verification from Linux alone.
5. Leave changes as a workspace patch. Do not commit, push, or request merge.
6. If blocked by proprietary model weights, licenses, unavailable tools or a missing
   dependency, write a factual blocker report instead of adding a stub or bypass.

Real-time audio constraints: no audio callback allocations, locks, blocking inference,
unbounded loops, file IO or network; preserve PDC, stop/seek/loop, silence and mono/stereo
behaviour. Maintain APVTS IDs and old session compatibility.
No fake metrics, no destructive edits, no secrets and no unreviewed model redistribution.

In the final response include: WYKONANO, ZWERYFIKOWANO, TESTY,
NIEZWERYFIKOWANE, NIEWYKONANE, ŹRÓDŁA, and criterion PASS/FAIL/BLOCKED.
If no safe change is possible, provide only evidence and blocker analysis.

Execution activation: changing this contract on the feature branch can trigger the
review-only GitHub Actions workflow via its configured push event. The job runs
only when repository variable MASTERLABX_AGENT_ENABLED is true and the secret
OPENAI_API_KEY has been configured. No automatic merging or publishing occurs.
