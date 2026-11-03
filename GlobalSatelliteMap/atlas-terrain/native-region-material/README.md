# Native regional material snapshot

This is the small material-only extraction of the working private terrain
probe. It owns no texture, camera, renderer, worker, file handle, persistent
cache or engine object. It does not load cities or change game state.

Include `NativeAdapter.h` and call:

```cpp
NativeRegionMaterial::Snapshot snapshot;
bool ok=NativeRegionMaterial::Capture(scope,region,gameRoot,
                                    regionMaterialGeneration,nativeFade,snapshot);
```

`Snapshot` contains copied `scope`, `WorldRaster::Material material`, and full
`parameterSha[32]` / `distortionSha[32]` hashes over the exact900/60float bytes.
The material has the caller's current source epoch, pinned source profile,
region and supplied region-specific generation. The base-wide material identity
is different; the owning coordinator compares both hashes against verified
base provenance before admitting fine rendering. Do not accept an incomplete
base or invent that proof from these copied values alone.

The caller establishes the real Main thread and current world/load epoch before
calling. Capture checks the thread against scope, current `ou` pointer against
full64bit `scope.world`, engine PE identity and the same two24byte geography
code pins used by the private probe. It verifies the32x32native map table and
reads the selected16 cells with ReadProcessMemory, rechecking the world, map,
table, cell source and five record pointers. Ordered source pointers and tree
bytes/hashes must exactly match the frozen region catalog. GameRoot must equal
the actual executable's game directory, handling the RE_Kenshi subdirectory.
All capture output is withdrawn/zeroed on refusal. The owning coordinator must
still guard same-ou reload generations, map visibility, device/current context,
queued results and eventual publication; this adapter has no independent load
hook and must not be used as a substitute for those checks.

`RegionMaterial.h` exposes `CaptureWith<Access>` for host controls. Access
substitutes only engine record lookup/type/profile/cell reads. The float-field
map, slope conversion, fade/distortion policy, authored file-path comparison,
layer packing, all57referenced source identities and zero-unused rows are
extracted from `PrivateGeography::FillRecord/ParametersRegion`, with provenance
in `extraction.json`. No new shading formula is introduced. Filename copies
are bounded by MAX_PATH and native tree output is bounded to1..2048bytes.
The remaining conversion and comparison semantics match the original.

At most57 record lookups,15packed material rows,180authored paths and16native
cells are processed. Strings are bounded metadata; vectors are transient native
trees of at most2048bytes, and copied snapshots have fixed size. No source-byte
file IO, texture upload, growing queue or world traversal occurs. These calls
are cold metadata preparation: cache immutable snapshots under the coordinator's
verified world/base identity and account for their copied CPU payload. Do not
capture all regions every frame. The SDK lookup implementation and Windows
crypto/runtime internal allocations are outside a logical payload proof.

Run `python -B run_tests.py`, then `run_mutants.py`. VC10/Od and/O2 each pass
200,880 focused production controls: all64regions off/on/off; original working
method bit patterns; independent57-record field maps; all960floats identical to
the actual native region2 case2 snapshot; full SHA pins; exact lookup/cell bounds;
stale64bit scope, profile changes, missing/incorrect SID/type, changed slot/tree,
nonfinite scalar, wrong/oversized path, oversized returned tree and foreign thread.
Three runtime mutants are killed (slope scale, tree hash, stale publication).
Host PE imports are inspected before execution and contain no engine DLLs.

The real SDK adapter is separately compiled and linked with /GL+/LTCG, Unicode,
KenshiLib and the profiled headers. It is inspected but never loaded/executed by
host tests. Its exported compile contract imports the native getData entry;
actual native capture remains a game control. No FPS, long-run leak, complete
atlas or live fine-viewer integration claim follows from these tests.
