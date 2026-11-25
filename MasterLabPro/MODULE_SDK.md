# MasterLabPro Module SDK 1.0

Include `sdk/masterlabpro_module_abi.h` and export exactly:

```c
MlpResult masterlabpro_get_module_api(uint32_t host_major,
                                      uint32_t host_minor,
                                      MlpModuleApi* api);
```

Define `MLP_MODULE_BUILD` while compiling a module. A module must return a static descriptor whose strings
and parameter table remain valid until the library is unloaded. It creates and destroys only its own opaque
instances. Buffers passed to state functions are owned by the caller.

Required lifecycle:

1. Host loads the library outside audio processing and negotiates ABI.
2. Host validates the API and descriptor.
3. Host calls `create` and then `prepare` outside the callback.
4. The callback calls only `process`, `get_latency_samples`, and realtime-safe parameter operations.
5. State is saved or loaded while processing is stopped.
6. The host retires every graph reference, calls `destroy`, then unloads the library.

`process` receives non-interleaved mutable `float` channels. It must be bounded and `noexcept` at the ABI
boundary. It must not allocate, lock, access files or network services, log synchronously, throw, or retain
the host's buffer pointers. A module must replace or reject non-finite data and report a finite bounded
latency.

The four SDK validation modules are separate native libraries:

- Unity: state-capable pass-through.
- Gain: atomic full-band gain.
- Saturation Test: normalized nonlinear `tanh` transfer.
- Delay/Latency Test: stateful stereo delay with variable reported latency.

They validate the SDK and are not production mastering effects.
