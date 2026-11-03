# Independent native receipt control review

**CLEAR for the bounded, opt-in read-only metadata audit**, after the caller guard correction described below. No remaining confirmed P1/P2 finding. Reviewed helper SHA256 `e5a555f17bfc9346ae4a26b59e9c77aedb8de36da19716b2f9c4d270439035af`; `candidate.json` pins the exact caller, default INI, compile control and unchanged source-receipts dependencies.

The root-produced full probe candidate was independently checked as687104 bytes, SHA256 `eeaaca325af34e83b20dfef341c4a902e91446d83f77cb64408b143a10e93b90`, with168 native imports. Its full build/regression run was performed by root, not repeated by this reviewer. The independent `/MD` standalone contract build compiled/linked successfully in the review mirror and was **never loaded**. Its only Ogre imports are the expected protected getResourceGroup and NedPooling deallocation targets already checked by the frozen native profile. No game process, UI, installation, INI or production source was touched by this reviewer.

## Confirmed caller finding and correction

The first caller delta obtained `SaveFileSystem::getSingleton()` before token/profile/LoadedGuard/world/map/other-capture checks. That reached a native singleton API even when the request was unavailable. The author moved those short-circuit guards before singleton access, then checks null/busy afterward. The actual extracted corrected branch passes45 host checks under both `/O2` and `/Od`; moving the singleton back to its earlier position produces an explicit runtime assertion failure. No further source correction was requested.

## Host-only evidence

- Legal base PMD representation is4 bytes with offset0xa0. The independently compiled base application is `lea rax,[rcx+160]`. No fake derived/engine object is constructed or dereferenced in the host tests.
-42 checks each under `/O2` and `/Od` exercise the actual extracted bounded copy loop with test-owned standard-allocator maps and real SDK Strings: empty/exact32/overflow33 groups, deterministic name copying,127-byte maximum name,128-byte refusal, unsupported text and canaries. The overflow-cap mutant fails a runtime assertion. This checks loop/ABI properties, not live native map traversal or its allocator.
-45 checks each under `/O2` and `/Od` exercise the actual extracted caller branch with thin native/INI/clock/log shims. OFF and foreign-thread callbacks do zero request/native work. Every unavailable token/profile/loaded-code/world/map/visibility/capture-owner condition reaches zero SaveFileSystem/audit calls. Save-null/busy guard order is checked. Startup seeding, same-token suppression, unavailable-token consumption without replay,250ms boundary and DWORD wrap all work. A successful token runs once; a160-row report emits160 rows plus one summary. Audit failure is an error, not fabricated successful output.
- Host executable imports are inspected before execution and exclude Ogre/Kenshi. Eight original component/caller/dependency files and the eleven SDK/module pins remain unchanged after review. Generated compile/host outputs are confined to this research directory.

## Code and lifetime assessment

The helper first checks nonzero scope and owning thread, initializes the existing exact native profile, verifies manager readability, acquires the existing SDK manager try-lock and applies the legal base data-member pointer to the **actual manager**. The group map is not copied. More than32 groups refuses; names are copied while locked and long/unsupported names refuse. Later group changes can make individual captures fail; the helper does not retain a native iterator or group object after that locked enumeration.

Each copied registered name is tried with five explicit logical forms of the canonical Hub overlay. A new NativeMetadata instance binds exactly one OwnedNamedFile role per group. Capture uses the frozen authoritative exact/lowercase indexes, listener/FileSystem/path/profile checks; fallback group searches, source-file open/hash, native DataStream acquisition, GPU rendering and image writes are absent from the helper. The output cap is160 attempts, with bounded group/resource/path fields. The actual fixed resource-name forms fit their128-byte report fields. Existing NativeMetadata RAII releases locks, module references and CryptoAPI ownership per scope.

The caller caches the startup enable flag (default0), polls the request token at250ms only in that mode, seeds the old token at startup and consumes a new token before availability checks. It uses the existing checked Main callback and does not add a hook or render operation. The caller's log/INI work is separate from the helper's source metadata reads. The helper performs code-memory profile hashing on Main; that is not source-file hashing.

## Meaning of a successful runtime result

`Report.accepted` counts successful **MainGate::Capture metadata snapshots**. It is not MainGate::Accept, file-content authentication, successful image decoding/upload, current rendered image validation or historical equality to native GPU bytes. An indexed file can still fail a later worker open or SHA check. All successful group/name forms are reported independently; this control selects no authoritative winner across groups or aliases. Root must label the log accordingly and use the captured names/paths as evidence for the next separately checked source step.

The supplied scope is a checked callback/request tag, not independent proof of a coherent owned base source capture. The first actual invocation will test live profile/map/getResourceGroup behavior, which remains **unexecuted by this reviewer**. A useful first-runtime result requires reading the returned profile/catalog statuses and actual successful metadata rows; `Run==S_OK` alone can coexist with zero matches. No global live-frame-adapter flag or production source-winner claim should be inferred merely from this audit.

## Performance and execution boundary

This is bounded but substantial one-shot Main work: up to32 groups and160 metadata captures, each with existing profile revalidation and copied names. Do not call it as a recurring frame scan. The request latch is essential. No worker, whole-world traversal, native source-file hashing, D3D allocation or map render is added. Source-receipts profile/SDK and compiler `/MD` contract remain the reviewed exact build; arbitrary ABI, caller-thread/pointer misuse and corrupted engine containers are outside the guarantee.

Root may proceed to the specifically authorized single live Main metadata request after its normal installation/context checks. This review is not evidence of actual runtime acceptance, game FPS, no leaks, map completion or safe use of arbitrary native objects.

## Reproduction

Run bundled Python `review.py` for independent compile and map/PMD checks, then `caller.py` for corrected caller tests and the guard-order mutant. `native-imports.json`, `original-pins.json`, `candidate.json`, host assembly and logs preserve the evidence. Standalone DLL hashes can vary by build timestamp; neither standalone DLL was executed.
