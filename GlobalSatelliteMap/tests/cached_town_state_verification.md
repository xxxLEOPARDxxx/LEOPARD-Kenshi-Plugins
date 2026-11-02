# One private cached-town owner

2026-11-02. Offline helper scope; no game calls, installation or runtime import performed by this test run.

Production: `GlobalSatelliteMap/GsmCachedTownState.h`, SHA256 `da6ba39f3a58d1b1206cf6d98164ebc2fa45bdc66ddcf4305d9cdfd6e28d8593`.

Run from any directory with the bundled Python:

```
python D:/DEV/Kenshi/ours/GlobalSatelliteMap/tests/cached_town_state_tests.py
```

Fresh result: 287 actual shared controller/owner checks; 26 checks executing extracted actual passive NativeOps inspector bodies; 25 compiled functional mutants rejected; complete actual native adapter compiled with VC10 `/O2` and current SDK, not invoked. The initial missing owner API was RED before implementation. Log: `GlobalSatelliteMap/build/cached-town-state-tests/full.log`, SHA256 `eeef6d4882ef1a0db47aa51a036f1a96fc3afa396820566a95eb4fd5104be20d`.

## Caller contract

`State::Select(values, rawCamera3, provenOrigin3, scene, generation, liveGeneration, sharedKing, engine)` is a synchronous render-thread operation, called only by the existing 1 Hz model-state path while enabled. It selects the nearest copied town identity with no existing proxy, finite stored Y greater than 1, within 8192 XZ metres of `rawCamera + provenOrigin`. The value table has at most 4096 entries. At most four actual source inspections, including validation of an existing slot, occur per Select. Rejected unloaded candidates cool down for five Select calls. At most 64 launched attempts are remembered for the context; unavailable files do not repeat reads or grow the memo.

Use `WantsRequest()/RequestValue()`, launch the existing worker successfully, then `MarkLaunched()`. `Adopt(request,result,currentGeneration,currentOrigin,scene,device,verified,otherOwnedGpu)` runs on the render thread. `otherOwnedGpu` excludes exactly this slot and includes all other live/pending allocations. Every subsequent caller allocation must include `OwnedGpuBytes()` too. `Geometry()` is a reference valid only while this State owns its slot; do not copy/memcpy its Packet or retain the reference across Clear/adoption. `Bounds()` uses the actual worker-parsed six local bounds and the admitted matrix. `Revision()` changes when published geometry or an error changes; invalidate composed window output when it changes. State does not choose a caller cache index.

`Valid(generation)` immediately observes the live generation without native queries. Physical cleanup remains explicit through `Clear()` on the existing live-renderer reset path. No Town pointer is retained. Scene, engine and SharedKing pointers are identity tokens only. Worker data is reparsed before import, its metadata compared exactly, and its SHA must be populated. The worker remains responsible for authenticating the SHA against its bytes.

## Native admission and frame

Before and after import, resolve the ID through the bounded current-world resolver; verify actual Town ID, copied pose and absent proxy; require the exact embedded TownBuildingsManager at Town+0x250, matching owner, and 1..4096 loaded building registry entries. The source requires the existing native model and census profiles, a current live world/scene, the pinned SceneManager origin getter, and typed root0. Root0's full matrix must be exactly identity basis with translation `-origin`. This last guard is necessary for the D8310 native identity-town-node placement shortcut: the private raw matrix is identity with translation `storedXYZ - origin`; it is not an unchecked absolute-world label. A current-world/profile mismatch refuses the optional layer.

Actual read-only evidence is `research/gsm-loaded-render-20261102/hub-cache-29908.json` and `hub-cache-29908-root-proof.json`. The observed Hub is container 2, stored `(-50978.80078125,1533.082763671875,2932.4765625)`, proxy absent. Its cached file has 2818 vertices/3201 indices and 323550 reserved GPU bytes under the native three-buffer reservation. This helper has no hardcoded Hub ID/path. Cached town geometry may differ from the currently loaded/modded buildings; this is a cached town LOD fallback, not a full loaded-material renderer.

## Lifetime, budget and failures

Only one State/owned private mesh slot may exist. OFF creates no Snapshot/mesh/file job. A successful native import is a single render-thread operation. Source/context/generation/origin/device are checked before and after it. No file IO, worker/Ogre calls, world activation or per-window import occurs here. The existing private-lod gate, parser payload limits, exact native ABI and inclusive `AtlasView::GpuCap` remain unchanged. Steady owned GPU and retained quarantine charge are reported, and CPU reporting includes the dynamically allocated Snapshot and its payload. The old valid source stays visible when a replacement file is unavailable; actual source invalidation/proxy appearance retires it. Native failure is locally latched, cleanup uncertainty quarantines the one slot and preserves its charge.

The old Snapshot destructor calls native unload/destruction and is unsafe if Ogre has already shut down. This cached owner therefore uses production-tested `ExplicitOwner<Snapshot>`: lazy creation, explicit live-renderer `Clear()` then deletion, and a passive destructor. If normal Clear is bypassed by process shutdown, at most one complete owner graph is intentionally retained at stable heap addresses until OS process teardown; neither native functions nor COM Release are called from that owner destructor. This is bounded exit retention, not proof of leak-free shutdown. Runtime DLL unloading/reloading is unsupported; module presence is not treated as renderer-liveness proof. A later explicit call to Clear still requires the render-thread/live-renderer caller contract.

The fixtures replace only native boundaries, not controller/owner logic. Native getter extraction tests exercise the actual inspector and filtered native fault path; full SDK compilation checks real declarations. They do not prove a real native import, model visual fidelity, in-game FPS, lifetime balance across real renderer shutdown, or cache freshness against the current modded town. Those require the root-owned controlled runtime trial.
