# Parameter mapping

Each module descriptor supplies a local 32-bit parameter ID, range, default, neutral value, step, smoothing
hint and flags. `ParameterRegistry` assigns one of ten stable instance lanes, with 64 reserved IDs per lane:

```text
host_parameter_id = instance_lane * 64 + parameter_index
```

Rack reorder changes only DSP order; it does not change the instance lane or host ID. A lane remains attached
to an instance for that project state. Right-click reset uses the descriptor's neutral value.

This is preparation for, not proof of, VST3/FL Studio support. The selected future VST3 strategy is a fixed
pool of 640 host parameters plus explicit per-project instance-to-lane mapping. This follows the VST3 rule
that exported parameter assignments must not change during a plug-in instance's lifetime. Unused lanes will
be hidden/non-automatable until mapped, subject to validation in Package C.

FL Studio Last Tweaked, Browse Parameters, Automation Clips and Link to Controller remain unverified until a
real VST3 is tested in that DAW.
