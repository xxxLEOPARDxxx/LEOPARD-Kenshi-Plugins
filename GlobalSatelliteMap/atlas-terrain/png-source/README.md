# Worker-only canonical PNG source decoder

`PinnedPng.h` decodes one authenticated base-snapshot material PNG into caller-owned
straight32-bit RGBA. It accepts only the129 canonical `WorldBank::RgbaPins` profiles,
exact encoded length/SHA,1024² or2048²8-bit RGBA IHDR and a separate exact-size
destination. Profiles compare all metadata and both hashes; they remain valid
across translation units and copied bank metadata. The decoded SHA must match the
independently generated Pillow pin.
This is a base-input primitive, not proof of the active game's resource winner.

Create, initialize, use and release the decoder on the same joined worker.
It owns one WIC factory and balanced MTA COM initialization; streams/frames/converter
are local RAII owners. File access, worker creation and native resource metadata
are outside this header. No source paths are opened by the decoder. Cancellation
is checked before hashing/decoding, between16-row groups and before final acceptance.
Bad arguments/hash refuse before output mutation. Decode failure or cancellation
after copying starts can leave partial destination bytes: callers must discard
the entire result unless the HRESULT succeeds. Never publish a partial buffer.

Input and output are each bounded to16MiB and must not overlap. The header allocates
neither blob; callers account for both, retained height, pending readback and other
owned CPU memory. WIC/COM can use opaque internal working memory; these size checks
do not prove a hard64MiB bound on all OS allocator activity or bound decoder latency.
Use only the pinned images and keep decoding/hashing off UI/render callbacks.

`run_tests.py` uses VC10/SDK7.1 and Windows WIC. Fresh controls decode all129 actual
PNG sources and compare exact pixel hashes, reject corruption, aliasing, short
destinations, cancellation and foreign-thread calls, then verify clean reuse.
Five compiled mutants (BGRA, cancelled work, aliased destination, encoded hash,
translation-unit-local identity)
must fail actual runtime assertions. No game or installed file is modified.
