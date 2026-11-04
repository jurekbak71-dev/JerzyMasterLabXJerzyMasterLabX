# Parallel routing Phase 2A

This slice implements only Issue #3 Phase 2A. Smart Dynamics Phase 2B and Stabilizer Phase 2C are outside this change.

## Audit and signal equation

The previous graph created one branch for every **present** module, aligned every branch to the limiter's fixed 5 ms latency, summed them and divided by the number of branches. A present but disabled module therefore contributed delayed dry audio; a removed module did not contribute. Card order did not affect the parallel sum. A limiter in Parallel constrained only its own branch, so any other branch could raise the final peak above its ceiling.

For present branches `i`, the legacy result was:

`y = sum(process_i(x)) / N`

That is intentional and is retained for old states, but one processed branch mixed with `N-1` disabled/dry branches is diluted. Phase 2A adds an aligned branch dry/wet mix `m_i` and gain `g_i`:

`b_i = g_i * (alignedDry + m_i * (alignedWet_i - alignedDry))`

Legacy mode remains `y = sum(b_i) / N`. Optional **Normalize branch weights** uses `y = sum(b_i) / sum(g_i)`. The denominator crossfades over the existing smoothing interval, and trim and wet controls use sample smoothing. The normalized mode changes relative branch weight without turning every trim adjustment into an unexplained master boost. Its denominator is based on positive linear gains and cannot reach zero within the bounded -24 to +12 dB ranges.

Limiter wet and dry use the same five millisecond alignment. Every other branch blends its module output with dry first and then passes through the same alignment delay. Reported PDC therefore remains `ceil(sampleRate * 0.005)` at every supported rate.

## State and preset compatibility

Schema 8 appends thirteen parameters after the previous 163 IDs: one compensation switch plus trim and wet controls for the six existing module families. Existing IDs, indexes, ranges and instance IDs are unchanged. Schemas 1 through 7 are accepted at their historical counts and receive defaults:

- Normalize branch weights: off
- Every branch trim: 0 dB
- Every branch wet: 100%

Those values select the old equal-weight 1/N equation. The regression test compares the current legacy path against independently rendered, latency-aligned module branches at 44.1, 48, 96 and 192 kHz. State tests cover schema 1–7 migration and schema 8 save/restore. Old binaries are not expected to load schema 8 states.

## Hybrid Studio slice

Selecting Parallel opens the first Hybrid Studio branch mixer. Its six cards show real post-trim branch RMS copied from bounded audio-thread snapshots, plus separate REMOVED and BYPASSED states. Each card exposes APVTS-backed trim and wet controls. The normalization switch is also APVTS-backed. The limiter warning states that other branches bypass the limiter ceiling; it is based on graph state rather than a decorative animation.

The GUI smoke test exercises the bindings, feeds real DSP, reopens and resizes at 750×550, 1000×700 and 2000×1400, and writes `parallel-smoke.png`.

## Limits

The limiter remains a sample-peak limiter. Parallel mode has no final post-sum limiter, so users who need a hard final ceiling must use a serial limiter after the parallel sum when that graph capability is delivered; the current UI warns about the condition. Manual FL Studio save/reopen, automation and export validation remains a release gate and is not claimed by these automated tests.
