# Passive loaded-building census

Synchronous render-thread debug helper; not yet integrated into the main plugin.

```cpp
#include "atlas-models/loaded-census/NativeCensus.h"
NativeLoadedCensus::Result result;
HRESULT hr = NativeLoadedCensus::Read(enabled, alreadyResolvedTown,
    expectedScene, expectedGeneration, &GSM::generation,
    verifiedNativeProfile, result);
```

Root owns nearest already-loaded-town selection, Debug-only scheduling/logging and
runtime validation. This helper reads only its supplied live primary Town. No
borrowed Entity pointer is exposed. Result contains copied scalar data and names;
there is no worker, cache, GPU resource, COM retention, loader, texture getter,
manager notification, visibility/instancing mutation or gameplay factory call.

OFF clears result and returns S_FALSE before any profile/world query. Missing
verification/zero generation refuses before queries. Failure publishes no partial
records. S_OK with zero selected means no qualifying source in this bounded census,
not proof that the whole town has no geometry. Incomplete is explicit.

Actual source path: primary Town type, embedded TownBuildingsManager (+250/native
569960), existing buildingEntities map, four floor FastArrays, ordinary Entity type,
exterior PhysicalEntity association, same current SceneManager/typed registry and
parent, existing loaded mesh/SubEntity material, first existing technique/pass.
InstancedEntity is counted and skipped. Grouped furniture/interior ownership may
fail the deliberately strict exterior check; missingOwner records that refusal.
Animated status is copied, not claimed supported by the DistantTown renderer.

Bounds:4096 map keys,4096 visited registry slots across all lists,16 selected
Entities,16 subentities per Entity,64 total material records,16 declaration elements
per subentity. Oversized input refuses or marks incomplete without silent truncation.
Names allow127 bytes plus NUL; overlong/control-containing names refuse that Entity.
Result<64KiB and transient NativeOps<100KiB are compile-asserted.

Physical ownership uses **4096 total initial part-pointer comparisons**, shared
across repeated unready/owner-missing candidates, plus a separate4096 publication
recheck budget. physicalVisited reports initial reads. Exhaustion sets incomplete
and physicalBudgetExhausted. The multiplicative4096x4096 scan is removed. These are
passive memory-comparison bounds, not measured CPU/FPS guarantees. Root's future
Debug caller should run at most once per5seconds, not on every UI tick.

Material records include actual material and first-pass VS/PS names, technique/
pass/texture counts, LOD0 topology/indexed flag and vertex/index ranges, semantic/
type masks, plus full declaration source/type/semantic/index/offset. First pass is
not a claim about the native camera's chosen pass. No texture binding or shader
bytecode is read; a supported ordinary-building material adapter remains separate.

Publication rechecks generation/current scene/type, map key/value identities,
last floor headers, each selected source's floor header/item, physical ownership,
scene registration, parent, subentity count and material identities. Volatile reads
ensure reloads under /O2/GL; they do not provide concurrent native lifetime safety.
Caller must keep the world/town alive at a serialized safe render boundary.

Run `./run_tests.ps1` with VC10/SDK7.1 and bundled Python. Fresh:14 actual native
code24/layout windows and InstancedEntity RTTI,49 side-effect/profile contracts,
52 shared production route/capacity/name checks,21 extracted actual native world/
floor/physical/membership checks,19 compiled functional mutants and actual native
adapter /O2 /GL compilation PASS. See `_generated/full.log` and verification.json.
Native getter calls have compile and exact-file evidence, not game runtime proof.

Root must invoke generate_profile.py when wiring the build. It validates the actual
installed EXE/Ogre SHA and emits _generated/LoadedProfile.h. Existing pinned model
getters/profile are reused. The separate root/container D5FD5 constructor-tail
profile correction handles the observed KEP hook; this helper does not weaken it.
No game/process/main DLL/install/INI/UI/commit actions were performed here.
Independent read-only review CLEAR: fresh full14/49/52/21 +19 compiled mutants
and actual VC10 compile. Peer also found zero mismatches in all14 census windows
and updated model/origin/core windows in the current process; no getter/census
execution was claimed. Canonical report:
`D:/DEV/Kenshi/research/gsm-loaded-census-review-20261102/report.md`
SHA4b802e9acb8b0f7190db8181777e0476e65a572c899bc060e0e99da6fdb67249.
Actual Hub material census, current source availability and measured CPU cost
remain runtime acceptance.
