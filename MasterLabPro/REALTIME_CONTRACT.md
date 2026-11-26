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

## Package C: native instance retirement
Control thread tracks the last successfully submitted revision referencing each instance. An instance marked removed is destroyed only when that revision or a later revision has been acknowledged as retired by the audio thread, or when processing is stopped and all graph references are cleared. Never reuse the instance ID while it is pending retirement. Drain retirement notifications on the control thread even when the order has not changed. Windows native loading no longer changes the process-wide default DLL search policy (important inside a DAW); explicit LoadLibraryExW flags remain in use.

## Package C: automation publication
Active graph nodes own immutable descriptor/lane/reader bindings. The audio callback reads normalized host values only through these bindings, applies parameters before latency detection and never traverses the GUI rack model. Removed instances reserve their automation lane until retirement, preventing a newly added module from changing an old active instance's automation. GUI topology operations and retirement use a separate control mutex, not the JUCE callback lock. State save/restore still uses the callback lock and remains an explicit acceptance blocker.

## Package C: state access (intermediate)
MLP_CAP_CONCURRENT_STATE_READ opts into concurrent get_parameter/state_size/save_state with process/set_parameter; the four reference modules implement this using atomic parameter storage. Explicit parameters are authoritative over opaque data on restore. VST3 saves capable modules without callback locking. Legacy modules and restore use an exclusive atomic gate: control waits for an in-flight callback, audio never waits and outputs silence while excluded. This removes explicit callback lock acquisition but can interrupt audio; it is not the final live-restore design. Replacement graph preparation/publication and uninterrupted legacy state save remain acceptance blockers.

## Live host replacement (development)
State restore constructs a separate host, prepares modules and installs its initial graph on the control side. One outstanding pointer publication is acquired at an audio block boundary. Old and new hosts render independently into fixed buffers and crossfade over 256 samples. The control side destroys the old host only after audio acknowledgement. The outgoing automation context is frozen. Legacy state reads still use the exclusion gate. Unequal latency alignment, module warm-up, repeated restore while stopped and GUI descriptor lifetime need further validation; this is not production acceptance.
