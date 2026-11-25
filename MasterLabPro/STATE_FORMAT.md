# Rack state format 1

`RackStateCodec` uses an explicit little-endian tagged binary stream, not raw C++ structure serialization.
The header stores magic `MLPB`, format version, module count, analysis source and DELTA-listen state. Each
module record stores instance ID, type ID, module version, lane, bypass, parameter `(ID, IEEE-754 value)`
pairs and an opaque module-state blob.

Limits are ten module records, 64 parameters per module, 1 MiB opaque state per module and 4 MiB total.
Decoding rejects truncation, trailing bytes, invalid versions, invalid lanes, excessive counts and oversized
blobs. Unknown parameter IDs are ignored by a module. A missing module record is retained verbatim in the
unavailable-state collection and included in later saves so reinstalling the module can recover it.

Module state callbacks validate their own versioned payload. The technical modules use a 16-byte payload
with magic, processor kind and an explicitly little-endian parameter bit pattern.

State restore and migration run only while processing is stopped. Format 1 has no older production format;
future versions require an explicit migration path rather than memory reinterpretation.
