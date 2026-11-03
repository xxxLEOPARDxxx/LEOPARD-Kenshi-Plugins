# FineStore independent review: CLEAR after P2 correction

2026-11-03. Reviewed `GlobalSatelliteMap/atlas-cache/fine-store/FineStore.h`, its actual tests/syscall fault injection and the frozen AcceptedWindow/WorldBuildDisk contracts. No production edits, game files/input, installation or commits by reviewer.

Final header SHA256: `0083e461be3c94ad1c18a3c4818376f9ee0d0c76b45c2d75e22248798e75dc58`.

Final author verification SHA256: `6575592aacf16c5522c685b31fb0c02c8bc3e9d0e02e8cf3aca942eb74464b65`.

No open P1/P2 remains in this synchronous worker-owned storage primitive, subject to the explicit outer retention contract below.

## P2 found and corrected

Initial frozen header `0c0c282614cce9af5d0d46872661fa11d37c425d7036372501287a4d09fa4c67` lost the local owned file HANDLE when CloseHandle returned failure in Put/Get/Remove; Close also discarded its directory handle on failure. Its original fault shim closed the real handle and then returned false, so it did not cover failure with a still-live handle.

The independent test uses actual production Put and a CloseHandle shim that returns FALSE **without** closing the target file. Initial observed output after Session destruction: `failed-close HANDLE still live=1`. The reviewer explicitly closed that orphan after observing it; temporary fixture inventory/cleanup remained scoped.

The author corrected all local file close sites to retain pendingFile ownership, retained a directory HANDLE on failed close, exposed `NeedsRetention()`, and made Close retry while preserving the error. The outer coordinator must retain/quarantine the whole Session and parent guards/budget until a later known close clears retention. Destruction while NeedsRetention is true is explicitly outside the caller contract; the primitive cannot keep its own containing storage alive.

The amended independent test checks an actual retained live HANDLE after failed Put close, a second failed Close preserving both owner and handle, known-close recovery after disabling the fault, latched failure preservation, and GetHandleInformation proving the actual handle closed before destruction. **11 additional controls pass under VC10x64 /Od and /O2**, PE imports inspected before execution with no Kenshi/Ogre/D3D/UI DLL. Final observed output in each build: `failed-close HANDLE still live=0; checks=11`.

The author expanded real syscall controls to live failed closes in Put/Get/delete-pending Remove/directory Close as well. Recorded current results:429 ordinary controls each/Od and/O2,54 syscall fault controls each/Od and/O2. No broad redundant rerun was added by reviewer.

## Remaining production contract review

- Put accepts actual production Accepted plus authenticated height window and exact current scope/frame. Anchored256@1m/3m keys, full64bit source/global material identity, independent per-region frame generation and three full32byte base/source/material digest pins are preserved. No9m GST namespace reuse or material upscale is accepted.
- Adapter converts only accepted reserved normal alpha0 to coverage255; original material FP16/HDR/gloss and authoritative worldY bits are copied unchanged. Actual Accepted/window owners are retained by caller until worker acknowledgement/join.
- CPU charges use subtractive cap checks and sizeof(Session), covering the128 fixed index/header/hash/path records; automatic producer/window/adapter charges plus explicit unrelated retainedRaw/other. The Get path clears previous output before allocation; one pending Blob and unrelated Blob accounting remain the external coordinator's duty. Hash/OS internal allocations are not claimed as measured total process RSS.
- Every record is new-only HANDLE-relative. The indexed exact header, physical file ID, full-file hash and per-plane hashes are kept independently of disk headers and verified on Get. New output/receipt is withheld until full write/flush/size and **known close**. Corruption/fault poisons the session, so no later write can bypass a failed close or partial record.
-128 fixed slot names and closed-LRU deletion before replacement prevent a129th owned payload. Replaced physical identity refuses deletion. Active Session plus unique serial invalidates evicted receipts. Disk cap134,348,800B includes all1281MiB+1KiB files; failure does not create an extra slot.
- Current-session private index does not trust cross-process files or a self-consistent disk header. Blob is immutable owned CPU data, but consuming current key/identity does not independently authenticate live world/load/plan. Those and exact complete-base provenance must be checked by the integrating native coordinator before GPU publication.
- No internal thread/queue is created. Session refuses the recorded native Main thread at Begin and all subsequent file operations require its owning worker. Outer worker known join is required before producer mutation or Session destruction.

## Evidence and limits

Independent source/runner: `close_failure.cpp`, `run_close.py`. Current logs: `generated/run-Od.log`, `generated/run-O2.log`, compile/import logs and cleanup manifests. Reviewer used actual canonical height source authentication and the author's production Accepted fixture; supplied finite/HDR colors are synthetic acceptance data, not native-render proof.

No fixture directories remain after the manifest-recorded cleanup. Source files and frozen hashes were rechecked after review. This CLEAR does not prove live coordinator shutdown/quarantine, GPU lighting/output, native game rendering, frame latency/FPS or long-run leak absence.

## Requested private diagnostic diff

The optional WORLD_COHORT_FAILURE refusal hook is default-absent and adds no normal/hot logging in other translation units. The reviewed amendment catches every diagnostic-hook exception before normal Cancel, preserving Refuse cancellation semantics. Case2 selects only region2/cell0 and one four-view cohort; hashes occur once per material-region capture, and the case2 raw dump is exactly3840bytes across120 bounded32byte lines. No rendering/validation arithmetic changed in that diagnostic diff. Its initial unguarded logging allocation could skip Refuse's direct Cancel on exception; the added try/catch resolves that concern.
