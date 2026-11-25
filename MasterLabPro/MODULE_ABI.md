# MasterLabPro C ABI 1.0

The public boundary contains fixed-width integers, C-compatible structures, function pointers and an opaque
`MlpInstance`. It contains no STL, JUCE object, exception, compiler-specific virtual class, or cross-library
allocator ownership.

Every extensible structure starts with `struct_size`. The host rejects a truncated structure, unknown major
version, module minor newer than the host, missing required function, zero type identifier, invalid channel
range, excessive parameter count, duplicate parameter ID, or invalid parameter range.

ABI major changes mean binary incompatibility. A minor revision may append fields and capabilities; old
fields retain position and semantics. Callers initialize `struct_size` before negotiation. A module accepts
the host's current major and returns the API version it implements.

On Windows the exported function uses `extern "C"`, `__cdecl` and `__declspec(dllexport)`. Unix builds use
default symbol visibility and `dlopen(..., RTLD_NOW | RTLD_LOCAL)`. Windows uses `LoadLibraryExW` with the
module directory and system directory search flags. Loading is limited to canonical files whose direct
parent equals the configured module directory.

The ABI does not promise compatibility with every compiler/runtime combination. No CRT allocation crosses
the boundary: module-created instances are destroyed by that module, and serialized state is written into
caller-provided storage.
