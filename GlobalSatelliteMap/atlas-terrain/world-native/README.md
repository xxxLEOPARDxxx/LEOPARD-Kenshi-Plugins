# Main-only raw indexed source adapter

`NativeProvider.h` supplies `WorldNative::Provider` for the existing templated
`WorldStream::Controller`. This folder does not include WorldStream.h or alter
Main/probe/installation. All provider operations and destruction belong to the
owning Main callback thread. No adapter/native/token pointer goes to a worker.

Construct one provider **per job**, call `Init(job.scope)`, then
`CaptureSources(job.frame,job.plan,job.scope,captured.token)`. `captured.token`
is the existing `MainToken[4][15]` array. The adapter first validates the frozen
plan and binds all roles, then captures all selected stage tokens. Capture is
single-attempt; failure clears every output token and makes the owner unusable
for another capture. Destruction releases all Main-owned provider state. Init
and CaptureSources return HRESULT; do not submit partially captured output.

There are four bounded NativeMetadata owners, each explicitly constructed with
`OwnedRawIndexedFile`. Only the plan's selected SIDs are initialized, and each
binds15 roles: painted overlay, painted colour, shared blend, six diffuse array
layers, six normal array layers. Padding DDS layers have distinct roles but use
the first canonical bank asset. The raw mode never binds an observed native
resource or claims equality to a listener-transformed stream/historical GPU
texture. The default NativeMetadata mode remains unchanged elsewhere.

The provider Capture/Refresh route through `request.role.slot`. Before routing,
the factory checks exact scope, role, canonical ID, encoded byte count/hash,
logical name and group. Refresh preserves the original spelling and compares
the complete snapshot, including64-bit listener identity and semantics.
BeginBank requires exactly15 unique roles in one SID and forwards one transaction
to that SID's NativeMetadata owner; EndBank releases that owner. Combined64-role,
mixed SID, duplicate or incomplete transactions refuse.

## Asset identity and logical names

| Input index | Logical resource | Explicit group |
| --- | --- | --- |
|0 painted overlay|Pinned asset's basename|Overlaymaps|
|1 painted colour|Pinned asset's basename|Overlaymaps|
|2 blend|`blendMap.png`, exact capital M|Overlaymaps|
|3..8 diffuse DDS|Pinned asset's basename|Landscape|
|9..14 normal DDS|Pinned asset's basename|Landscape|

Canonical IDs are `DDS catalog index+1` and `0x10000+PNG pin index`.
`CanonicalPath(role.asset)` returns the immutable catalog path; unknown IDs
return NULL. The logical name is separate: the blend's physical catalog entry
is lowercase `data/newland/land/blendmap.png`. No path/case normalization is
performed on the captured original request. NativeMetadata performs its actual
exact-then-insensitive lookup while preserving this spelling.

`ValidateSourceName(request,canonicalPath,inputIndex)` is the narrow helper for
the controller's existing pin/role validation. It checks the exact approved
logical pair. The caller still validates scope, selected stage/role and encoded
byte/hash pin. It does not accept full canonical paths, alternative group names,
arbitrary aliases or silently search other groups. WorkerRequest path/archive
fields are filled only by Main's actual indexed-winner capture; the source
factory supplies no guessed physical file path.

The controller can retain its own plain `SourceFactory`: call `Clear()` at job
reset, then `Init(frame,plan,scope)` once per Submit. `Matches(request)` is a
bounded role/pin/name comparison against that retained plan; it does not rebuild
the plan per source. `InputIndex(request.role)` maps the type/index to0..14.
The native Provider remains single-job-owned and cannot be reset this way.

The DDS basename/Landscape route and blend image's blendMap.png/Overlaymaps route
were traced from actual native callers. Native material texture loading uses
AUTODETECT for painted/shared images; these explicit pairs are the fresh owned
raw input contract, not proof of those historical textures' resolved groups.
See `D:/DEV/Kenshi/research/gsm-raw-indexed-semantics-20261103/report.md` and
`blend-note.md` for exact callsites and the boundary of that evidence.

## Lifetime, cost and verification

Each job has fixed four stage owners and at most60 Main tokens. This adapter
adds no thread, file I/O, image decoder, GPU object, cache or growing queue.
It does not retain file blobs. Snapshot metadata and its bounded role checks
are performed only on explicit capture/accept/commit operations; no per-frame
world or archive scan is introduced. NativeMetadata still validates its code
profile and locks current group indexes; actual callback latency and long-run
memory behavior require game measurements. Controller CPU admission must include
`sizeof(WorldNative::Provider)` and external token owners when retained outside
the controller. Do not put a provider plus60 tokens on a small callback stack.

Run bundled Python on `run_tests.py`. Both VC10 `/Od` and `/O2` host binaries
exercise the real SourceFactory/StageAdapter with fake stage metadata. The full
64x16 catalog sweep covers1,999 selected SIDs and29,985 requests. Host PE imports
are checked to contain no Ogre/Kenshi module. Four independent source mutations
(wrong group, canonical-path alias, wrong SID routing, truncated64-role bank)
must fail an assertion. NativeProvider is separately compiled/linked against
real Ogre headers/import library into `_generated/native-contract.dll`;
the runner **does not load or execute this DLL**. Actual Main native capture,
file receipt acceptance, GPU rendering and controller integration remain the
host application's checks.

`run_tests.py` also obtains the compiler's exact `/showIncludes` closure before
the run and confirms source/dependency hashes afterward. After a full successful
run, `python freeze.py --record` records current sources, all935 borrowed/compiler
dependency files, logs, binary hashes and exact PE import names in
`verification.json`. `python freeze.py --verify` checks those existing pins
without rebuilding or refreshing them. Reruns change binary timestamps/hashes;
record a new manifest after the complete successful run. Neither operation loads
the native contract DLL or certifies future/live-game behavior.
