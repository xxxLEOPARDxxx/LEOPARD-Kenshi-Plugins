# WorldFrameControl caller independent review

Verdict: CLEAR for the opt-in private caller at the source pin below. No remaining confirmed P1/P2 finding in this reviewed caller. This permits the next controlled native one-frame test only after the separate controller review and final native build pass; it does not establish image acceptance, whole-atlas readiness, throughput, or absence of runtime leaks.

Reviewed file: `D:/DEV/Kenshi/ours/tools/diagnostics/satellite-capture-probe/CaptureProbe.cpp`, SHA256 `29e5068a1bef8ef080298601030a7aaf27f861f4235496fa8573e20f59e52c87`. The scope is its current git delta: includes/globals, PrepareWorldFrame/WorldFrameNow/WorldFrameFinished, workspace callback, Update admission/draining, Drop ownership change, startup flags and opt-in load/new-game hooks. Production sources were not edited by this reviewer. No game, native API, UI, install, INI, commit or publication action was performed.

## Correctness and lifecycle

The workspace callback requires Main before controller access or UI queries. The current admission expression short-circuits stop and FixedContext before querying either widget. Two fixed global WidgetRef objects register unlink callbacks and refuse admission if registration fails; both inherited visibility values are checked freshly in the callback. Existing previous-listener forwarding remains once per callback, including refusal and caught exceptions.

The Update path consumes changed request tokens before checking availability, so busy/unavailable requests cannot later replay. At most one controller/job is active. OFF returns through the existing caller without starting this mode. New requests require the loaded executable/code profile, Main, ABI, current world/renderer/workspace, map visibility, save-idle state, safe restoration/listener chain, no competing capture and usable UI references. FixedViewerOff requires both the startup-disabled checkpoint and current viewer Enabled=0; the resource census also refuses any viewer textures. These checks precede device acquisition/Begin.

Exceptions, explicit cancellation, hidden/unavailable context and world changes latch worldFrameStop. Main Update continues Tick(false,NULL,...) draining independently of compositor callbacks and device retrieval. Normal Drop requires inactivity; quarantine is the explicit retained-owner terminal alternative. Known manager-owned listeners restore the recorded previous listener. Wrapped or unknown nonnull workspaces retain forwarding and make chainSafe false, preventing another request from overwriting the chain.

The two SaveFileSystem wrappers are installed only for this opt-in mode, increment an atomic local generation before forwarding the unchanged original call, and must both install before the MapScreen hook is attempted. Prepare captures that generation at entry, so a nested load invalidates even preparation. WorldFrameNow flips the copied source epoch when the generation changes, rejecting a same-ou-address load/new-game result. The local generation is a 32-bit change detector, suitable for this single private diagnostic request, not a persistent world identifier.

Prepare constructs a fixed 512-square/1m value camera/grid/job and copies bounded native material parameters. ParametersRegion was inspected: it checks bounded region/SID/order/path/mask data; texture source reads/hash/decode remain in the separate source pipeline. It does not borrow a native render camera or mutate the global diagnostic Side. This caller creates only a fresh private output name and supplies no ready/active atlas publication path. Artifact writing and delayed final publication remain the separately reviewed controller's responsibility.

## Fresh verification

Run bundled Python on `check.py` in this directory. It extracts the actual callback, Update branch, Drop ownership branch, WorldFrameNow/Finished and both lifecycle wrappers from the pinned source. Native/UI/device/INI/controller operations are explicit host stubs; this tests caller ordering and control flow, not Ogre execution or D3D output. The generated executable has no Ogre/Kenshi imports.

- VC10 x64 /MT /O2: 42 checks PASS.
- VC10 x64 /MT /Od: 42 checks PASS.
- Controls cover foreign-thread refusal before UI/native access, context-before-widget sequencing, hidden/deleted widget rejection, admitted device Tick, exception stop/drain, no active Drop, known/wrapped/unknown workspace cleanup, token non-replay, busy cancellation, availability guards, OFF/startup latching and full-width scope invalidation on same-address load/new-game.
- Three compiled negative controls were rejected at runtime: removing the Main callback guard; removing generation increments; removing epoch invalidation. Build/run logs are retained under `build/`.
- Source hash was rechecked after the final tests and exactly matches the pin above. Root reports a full native probe compile/link and loader-contract run; this review does not substitute host tests for that native build.

## Closed findings and limits

During review, root corrected stale map visibility, UI access before Main/context checks, and rearm after unknown workspace ownership. The final extracted controls cover those corrections. Root additionally supplied the stop/drain latch and load-generation guard; both were reviewed in their final source form.

The per-callback/current Update code can repeat INI checks and a bounded 32,768-entry native texture census, while preparation reads bounded native material metadata on Main. This is acceptable only for the short opt-in diagnostic scope reviewed here. It is not evidence of acceptable permanent viewer/frame latency; a production scheduler must remove this repeated work. Two WidgetRefs and one lazy controller remain process-lifetime bounded objects. Native map skin deletion, external listener lifetime, actual reload timing, current-source provenance and the complete RGB/normal/height artifact must still be checked in the controlled native test. No runtime FPS, universal cleanup, current historical GPU match, or full-world rendering claim follows from this review.
