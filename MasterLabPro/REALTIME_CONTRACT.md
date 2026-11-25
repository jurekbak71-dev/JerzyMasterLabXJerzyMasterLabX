# Realtime contract

The audio callback may perform bounded arithmetic, atomic loads/stores, fixed-capacity SPSC operations,
preallocated delay access, module `process` calls and monotonic timing for diagnostics.

It must not perform heap allocation, library load/unload, instance creation/destruction, filesystem or
network access, blocking locks, GUI waits, synchronous logging, unbounded queue draining, or graph deletion.

Graph snapshots contain at most ten raw non-owning instance references. Ownership remains on the control
side. A release/acquire SPSC publication makes a fully prepared snapshot visible to audio. An old snapshot
is placed on a second SPSC retirement queue before the new snapshot becomes active. If retirement capacity
is unavailable, audio delays the switch; it never loses the lifetime acknowledgement. Destruction is legal
only after playback stops or after the control side proves that no active, pending, or queued snapshot refers
to the instance.

Module parameters used during playback are atomics in the reference modules. A latency-affecting update is
detected at a block boundary and handled by a bounded fade/reset/fade transition. State save/load is not
realtime-safe and is restricted to the stopped state.

Native third-party code is trusted to honor the same rules. The host cannot enforce them in-process.
