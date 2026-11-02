# Cached native model admission

`Snapshot::Read` remains forced-fresh. `Refresh` repeats bounded source admission,
then compares a fixed descriptor before any sampler, input-layout or rasterizer
creation. The descriptor includes generation, device and retained COM identities,
shape, pose, origin, bounds, material identity and sampler values. An unchanged
snapshot creates no GPU objects. Expensive module/export validation is cached per
generation; `ResetProfile()` must run at world/module retirement.

The Packet retains buffers and diffuse views; source Town/Entity/Node/Mesh/Material
pointers are never dereferenced from the cached descriptor. `RetainedBuffer` and
`RetainedDiffuse` expose borrowed references for the caller's global retention
ledger. They do not transfer ownership.

Failure, OFF or missing verification clears the packet. `changed` reports a
successful replacement only, so the caller must also dirty old bounds when a
previously valid packet becomes invalid. Resource identity is not a content hash:
same-resource in-place GPU writes are not detected. This cache is for static
native DistantTown meshes, not arbitrary animated or dynamically rewritten meshes.

Run `test_refresh.py` with bundled Python: 96 extracted production-body checks
and six compiled negative controls. Existing `../run_tests.ps1` additionally
compiles the complete SDK adapter and tests native-shader raster/state behavior.
Independent review also ran actual cache/profile dispatch extraction (115 checks,
five negative controls), retirement controls (33 checks), and real WARP retention
accessors (489 checks). These are offline checks; native runtime and timing need
separate caller integration tests.
