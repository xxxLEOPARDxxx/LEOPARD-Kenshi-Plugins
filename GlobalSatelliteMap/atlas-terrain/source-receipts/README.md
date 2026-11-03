# Profiled FileSystem source receipts

This C++03/Win64 component supplies a production metadata provider, a Win32-only
file lease and a source acceptance gate. It is separate from Main, the private
probe, texture upload, staging shaders, atlas writing and installed mods.
The actual native metadata API has now been exercised by an opt-in Main audit: the default strict-null policy correctly refused the normal RE_Kenshi listener; explicit raw-indexed metadata found the one expected Overlaymaps file. This is metadata capture, not yet worker acceptance or a completed rendered frame.
`NativeMetadata::LiveFrameAdapterTested` is explicitly false. The next integration
must provide the checked owning-frame/world/job adapter and actual runtime tests.
Compilation and host controls are not presented as a live source-winner result.

## Admission and native profile

`Scope` carries owning thread ID and64-bit world identity, source/base epoch and
job ID. `Role` carries64-bit asset identity, selected slot, family and role index.
Diffuse/normal indices are0..5, slots0..3; painted colour/overlay/blend index is0.
Named request group/name, coherent base pin(epoch,asset,byte count,SHA256), scope
and role must be valid before metadata access or file I/O. Scope comparison
retains all64 bits. Current scope supplied by the caller must come from its
checked live frame/job state; this library does not manufacture a world validator
or establish a base manifest from arbitrary disk metadata.

The exact installed Ogre profile is SHA256
`7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6`.
`prepare_profile.py` verifies immutable `sdk-pins.json`, the complete DLL hash,
18 required code windows, the14-entry FileSystem vtable and absence of ASLR
relocations inside hashed code. Its private generated header contains hashes,
RVAs and export names, not copied native shader/code bodies. SDK/header and Boost
pins, code windows and lock ABI are recorded in `_generated/profile-contract.json`.

At runtime Init holds a module reference and CryptoAPI verification context.
Every metadata capture/revalidation checks the owning thread/module identity,
PE timestamp/image shape, current code-window digests, exact exported addresses,
all14 relocated FileSystem vtable entries and the actual imported targets used
by the helper. The only Ogre imports are protected getResourceGroup and the
SharedPtr allocator's NedPooling deallocation; both targets/code are gated.
The MSVC member-pointer import thunk/direct target is checked; its this-adjustment
must be zero. PMF padding is not mistaken for an ABI field.

The local VC10 SDK compiled offsets match native code: String40B, manager
listener0x98/group-map0xa0, group name0x30/locations0x60/exact-index0x80/lower-index
0xa8, Archive name8/type0x30 and Resource name0x60/group0x88/manual flag0xe8.
Protected Group/getResourceGroup are exposed through a legal derived declaration
and a base-class member pointer applied to the **actual base object**. No fake
derived object or unchecked RVA call is made.

The native recursive-lock algorithm uses recursion@0, thread@4, basic mutex@8,
event@16, lock bit31 and event bit30, matching pinned Boost1.60. The provider uses
SDK **try_lock**, manager before group; busy locks refuse without waiting. Native
getResourceGroup's recursive manager acquisition is on the same owning thread.
The standalone lock test uses only test-owned SDK mutexes, not engine objects.

## Source kinds and exact winner

`NativeMetadata::Init(scope)` binds an owning scope. Bind each requested role with
`NamedEvidence`. Unknown kinds refuse. `OwnedNamedFile` requires no ResourcePtr:
it explicitly means a new owned named-file input, not evidence about historical
GPU texture bytes. The checked adapter must establish its real request name/group
and associate its content pin with the coherent base source epoch.

`ObservedNativeResource` requires a matching retained `Ogre::ResourcePtr`, held
Main-only and bounded by the role cap. Capture checks the profiled manual flag
is false and copies/compares actual native name/group. Null, manual, mismatching
or unsupported evidence refuses. The native provider and retained references
must be created/destroyed on Main and released before world/renderer shutdown.

The manager singleton slot comes from its pinned getter, not a guessed process
address. Actual groups are obtained through the verified SDK import under the
manager recursive lock. The exact name index wins before the native-CRT-lowercase
index. The original resource name is retained for path construction; the lowered
key is not substituted. Native index population overwrites duplicate names with
later registered indexed archives; this provider reads that actual mapped value,
not all FileInfo matches or a manual mod-priority walk. Under the default `NativeCompatible` policy both loading-listener checks must be NULL. Explicit `OwnedRawIndexedFile` semantics instead describe the original file selected by the actual named group index, before any native listener transformation. This mode binds only new `OwnedNamedFile` inputs, never observed native resources. MainSnapshot stamps the full64-bit listener pointer and semantics; capture checks the pointer before/after, acceptance compares both, and bank commit checks every stamp under its locks. The listener is never called or passed to a worker. A stable pointer is only an invalidation signal, not proof of the listener's internal behavior or delivered stream contents. Only the **exact exported FileSystem vptr0x682dc8** is allowed;
archive type text alone cannot prove it. Missing indexes, ZIP/custom archives,
fallback location/group searches and unknown encodings refuse.

Native path construction is preserved: empty archive root, `/` or `\` rooted
resource, or an alphabetic drive prefix plus colon bypasses the archive root;
otherwise concatenate archiveName + `/` + originalName, unconditionally adding
that separator. ASCII input is resolved with GetFullPathNameW, rejecting any non-ASCII result rather than using a lossy conversion. It captures an absolute local drive path on Main.
CWD is checked before/after resolution; drive-relative names also capture/check
their own drive-current-directory basis. Acceptance repeats this process. Only
bounded printable ASCII and strict ordinary drive paths are supported initially;
UNC/device/extended namespaces, ADS, reserved DOS names, trailing dots/spaces and
unsupported lengths refuse. This is an explicit supported domain, not a guessed
UTF-8 conversion or fallback to a metadata disk path.

## Worker lease and source publication

`MainGate<NativeMetadata>::Capture` returns a Main-only `MainToken` and its copied
plain `WorkerRequest`. The latter has no Ogre pointer, String, iterator, callback
or native reference. Main identity tokens never enter the worker request. Main
does not open texture files, run Archive::open or acquire native DataStreams.

`WorkerFileLease::Open` uses only Win32/CryptoAPI and a copied request. Its one
blob is at most16MiB; caller-owned CPU bytes plus `RequiredBytes()` must fit64MiB.
Every ancestor, from drive root through parent, and the final file is opened with
FILE_SHARE_READ only, retaining handles through Main acceptance. Every handle is
checked for reparse points; directories and file must have the expected kind.
The final file is read-only. This strict policy rejects a real NTFS junction,
rather than merely rejecting link-like path text. It intentionally denies write
and delete sharing while the lease is held.

Size must exactly match the base pin before blob allocation. The worker reads
64KiB chunks, checks cancellation between operations, SHA256-checks exact content,
compares pre/post file ID, volume, size, attributes and creation/write stamps,
and records normalized final path plus physical file identity. The handle lease
stays open through handoff. There is no Ogre or D3D call on the worker. Failures
close every partial ancestor/file handle and release bytes before returning;
Clear is idempotent and supports retry. One instance cannot be opened twice.

Cancellation is cooperative: a pending synchronous local-disk ReadFile or hash
is not preempted. The checked adapter must withdraw/cancel the current job on
Main, publish immutable requests/results with proper synchronization, and never
destroy a lease concurrently with Open. This component does not create threads,
implement a worker queue or guarantee a wall-clock I/O deadline.

Bytes are unavailable through Data until Main acceptance. Accept refreshes the
actual index winner, module/code/thread/scope/listener, names, path and path basis,
compares the worker request/stamps/pin and issues a sealed `AcceptedReceipt`.
The caller uploads accepted bytes and then clears that lease. The summary remains
Main-only, so the bank needs exactly one pending blob rather than12 retained
source blobs. **A source receipt does not prove GPU upload success.** The frame
adapter must pair each summary with the corresponding successful immutable upload
and resource-owner admission.

CommitBank validates every accepted role summary, including the first, under one
manager plus all relevant group recursive-lock transaction. It rejects stale
identities, contexts and duplicate role positions. Group try-lock failures release
all prior locks. No partial bank success is returned. The fixed metadata cap is16
roles, sufficient for a6+6 DDS bank plus three shared paint/blend inputs; this
does not authorize simultaneously resident full four-SID banks or any GPU cap
increase. The final source commit does not need the released blobs/file handles.
Its authority is immutable uploaded content matching the coherent base source
pins plus fresh winner metadata, not a claim that disk contents never change.

Historical equality to already-uploaded native GPU textures is not required or
claimed. If a base pin was only an unassociated disk hash, its winning-source
association must first be established by the owned base capture. Unknown or
changed content is refused instead of silently being used for fine windows.

## Performance and lifecycle limits

All arrays, role records, paths, ancestor handles and metadata transactions have
fixed caps. There is no world scan, wildcard resource enumeration, growing cache,
INI poll or per-frame file I/O. Metadata acquires only the requested job's sources;
the runtime adapter should retain accepted immutable inputs for their source epoch.
Native strings are bounded copied data. Archive identities are Main-only tokens,
never worker lifetime references. Fresh snapshot pointers are used only inside
the owning locked call. Observed resource references and the module/CryptoAPI
lease are explicitly scoped to the provider's Main lifetime.

CPU accounting is an owned-payload ledger, not a claim about CRT/OS/CryptoAPI
resident overhead or native resources retained by external ResourcePtrs. Required
bytes include the lease object and its one exact-sized allocation. The caller
must include its other live/pending buffers and fixed metadata bookkeeping. GPU
streaming, uploads, staging carry and renderer budgets remain separate and must
still respect192MiB. No game FPS or long-run memory measurement is claimed.

## Reproducible standalone verification

Use bundled Python, VC10/SDK7.1, pinned local KenshiLib/Boost and the existing
vendored pefile reader:

```
C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py
```

The native contract DLL is compiled and linked only, **never loaded/executed**.
Before executing either host test, its PE imports are checked to exclude Ogre
and Kenshi. Host tests use actual test-owned files, worker threads, deny-sharing
handles, an NTFS junction, cancellation and test-owned SDK locks/strings/maps.
Dummy metadata providers exercise actual MainGate templates without game calls.
Fault controls compile the actual production headers with thin Win32/allocation/
hash failure boundaries: every partial-resource cutpoint must clean up and retry,
and invalid guards must reach zero file operations. Nine separately compiled
mutants must fail real assertions; compilation failure is not accepted as proof.

All files written by this workflow are inside this new component's ignored output
tree. There are no game/INI/install/hook/process/UI actions. Runtime native source
acceptance, checked-frame integration, format decoding and final GPU publication
remain the parent's next concrete integration stage.

## Live naming evidence2026-11-03

The live DDS Image::load caller uses `Landscape` and the DDS basename. Painted image construction uses `Overlaymaps`; its material texture route uses AUTODETECT, so do not infer historical GPU group identity from a path. The chosen owned raw input contract uses explicit Overlaymaps+basename, with exact `blendMap.png` for the shared blend image. Canonical content pins are distinct from logical resource names. See docs/verification/2026-11-03-satellite-owned-source-inputs.md.

Listener tests run41 checks under both/O2 and/Od and reject four separately compiled listener-high32, semantics, invalid-mode and borrowed-evidence mutants. Actual lease tests also withdraw bytes when either listener identity or semantics changes. The existing29 Win32 fault paths and nine mutations remain green.
