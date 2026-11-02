# One cached town LOD fallback — 2026-11-02

Development-only, not a public release. Runtime import/visual acceptance pending.

User reported absent Hub although existing distant city LODs render. Exact
read-only snapshot PID29908 confirms actual Hub container2 has no proxy;
its existing distant_2.mesh108057B is native-format,2818verts/3201indices,
107850primaryVB/IBbytes,323550conservative3xGPUreservation. Native DistantTown
material/diffuse already loaded. This supports a cached LOD fallback, not full
loaded building geometry or freshness against mod/save town changes.

Selection reuses1Hz coherent town values: nearest missing-proxy/resolvedY town
within8192m of the native camera, <=4 native checks per selection, fixed64memo.
Current Town/manager/root identity, pose, generation, scene, origin and device
are checked; identity root/rawpose+origin mapping is proven, not guessed.
One existing worker reads <=512KiB locked canonical native basename, <=64KiB
chunks, with cancellation, SHA, identity and strict production parser. It returns
only owned bytes/scalars. Typed Poll completion precedes generic terrain paths,
with scoped job deletion on exceptions; optional failures do not disable terrain.
Errors always log, repeated selection reasons are latched; Debug remains last.

Serialized adoption imports into one owned creatorNULL/unregistered Mesh via
existing proven native importer; no new Entity, town, resource-group registration
or zone activation. Its Packet remains owned through synchronous model replay,
index64 after64 borrowed slots. Pristine terrain is copied once before composition.
Every model/local/water allocation guard charges the cached reservation, including
quarantine. Current worst-case shared layers196086608 +Hub323550 =196410158B,
below192MiB cap201326592. No world recapture or 8K source change.

Explicit normal Clear retires while renderer alive. Implicit exit destructor
is passive: if normal Clear is omitted, one bounded owner graph is retained until
OS teardown rather than calling Ogre after renderer destruction. This is not
leak-free shutdown or support for runtime DLL unloading. Worker metadata/bytes
retire independently; old valid cached slot remains on unavailable replacement.

Fresh root fullviewer exit0 and nativeVC10/LTCG+83imports PASS. New focused gates:
61fileworker+9mutants;287state/owner+26actualnativeInspector+25mutants;32actual
Poll/selection+11mutants;57actualmodelcaller+10mutants. Private importer2919checks
and14mutants, actual282nativefiles/ABI/fullSDK compile independently verified.
Three interface locales stay22messages, Debuglast. Fullviewer also includes all
previous geometry/zoom/water/model controls. Review reports in research:
gsm-cached-town-job-review-20261102, gsm-cached-town-review-20261102,
gsm-cached-town-main-review-20261102, gsm-private-lod-review-20261102.
Host seams do not prove native runtime import, actual cleanup/resource counts,
FPS or long-term process memory. Those are the next controlled game checks.


Runtime follow-up: user restarted and confirmed Hub buildings visible and zoom
edge strips gone. PID4184 logged one cached import at59.834: id2,
323550ownedGPU/349469CPU payload; modelCount29 borrowed+onecached, total196410158B
within201326592cap, all16coarse masks valid/shaded/bound. Missing walls/market
props/lamp posts and poorclose LOD fidelity are explicitly reported and remain
unfinished loaded-geometry work, not a claimed complete city render. No native
cached import error observed. Normal reset/reload cleanup and longmemory/FPS
still need controlled runtime proof. User subsequently closed Kenshi.

Observed717modelpacket calls/56.589seconds withunchanged read/localrender counts.
Exact invalidator unknown: existinglogs lack light/model/cachedrevisions, native
temporal input advanced but thisdoesnotprove pause state. Lighting changes
reraster models bycurrentarchitecture. Readonlyaudit:
research/gsm-model-raster-idle-audit-20261102. Nounproven geometrybug/fpsclaim.
