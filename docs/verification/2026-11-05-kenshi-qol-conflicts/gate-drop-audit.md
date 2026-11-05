# Gate registration / post-drop: native follow-up, 05.11.2026

READ-ONLY. KEO base `e6082d6264b68daaa4297ede7f6fe1cc5f9fb4f4`; QOL source hash `955b340eb25b0f66c1e08479ec2e8d4636a39f622962582d9bff8e6bed27363e`. No production/build/game/UI/native calls. Native images: Steam `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`, GOG `2defc287c813798b9682b93acf68d340da895cf5164d342145ba0faeb9024681`. SDK resolution/provenance in `evidence.json`; ten actual instruction-window comparisons and hashes in `gate-drop-native-proof.json`.

## Verdict

**Gate hook: conditional native gap is real in control flow, but actual vanilla-data bug is NOT yet proved. Do not import completion registration as a universal fix.** Native already has two registration paths; QOL comment “native never registers a player-built gate” is too broad. The important remaining question is the pair of data flags and the handle/town at creation, not a missing addGate function.

**Post-drop refresh: potentially useful small feature, but QOL's exact implementation has a proven native failure-case defect.** Must reject query result -1 as well as0, preserve original bookkeeping on failed query, and resolve ownership/lifetime before adoption. This audit does not claim a Squin/NPC-position fix: QOL writes a building handle, not Character::pos.

## 1. Gate native chain and registration conditions

QOL `src/KenshiQOL/KenshiQOL.cpp:1296–1305`: post-original Building::notifyConstructionComplete; if isGate, `getRealTown()` then `town->addGate(getHandle())`.

Actual Steam:

1. `RootObjectFactory::createBuilding` tests gamedata key **"is wall gate"** at **57D29D–57D2F2**. True → allocation0x360 and thunk49E31 → **GatewayBuilding ctor5626B0** (call57D33F). It passes a building handle through the ctor arguments.
2. Gateway ctor **5626F9** calls base Building ctor through414D9 → **55C000**. Base ctor copies the supplied handle through its RootObject initialization at55C03A–55C072; installs **Building's base vtable** at55C086–55C08D; at **55C403–55C406** calls base **Building::setup551090** directly via122D8. Gateway vtable is installed only AFTER base ctor returns, at **5626FF–562706**. This distinction matters: base-ctor calls cannot assume derived Gateway::isGate/setup dispatch.
3. Base setup registration decision **5510F3–5511AE**: first virtual isGate slot3D8; non-null skips generic registration. Next isWall slot2C8+field1F0, otherwise a DIFFERENT gamedata property **"is gateway"** at **551132–55116B**. On admitted path it obtains **getRealTown296900** viaD6CA at551193, and calls town virtual **addGate slot2F8** at **5511A7** with `this+58` hand.
4. During base construction, base Building vtable3D8 resolves thunk82FB→F6BC0, and2C8 resolves153C0→F6AE0; both actual bodies are `xor eax,eax; ret`. Therefore the generic base-ctor registration for these gates depends on **is gateway=true**. It is not controlled by the factory's **is wall gate** predicate.
5. **GatewayBuilding::setup552740** separately calls base setup551090, then native `Building::getTownF6BE0` (not getRealTown), and unconditionally tail-dispatches addGate at **552771**. But Gateway ctor does NOT call this override: its only post-base call562720→25C93→29E080 is **Building::setupFromData**. No direct callsites of Gateway::setup were found; known relevant virtual450 dispatches occur in **Building::upgrade2A49A0 at2A4A07** and **downgrade2A4D70 at2A4DC8**. The normal base constructor path is different.
6. Completion **5618B0–561A0C** marks build state and invokes visibility/power/broken/lights/effects, child notifyConstructionComplete, related native notifications. It has **no direct addGate or virtual addGate2F8 call**. These facts do not prove absence in every callback, but they refute treating the function as a second unconditional registration path.

Concrete conditional gap: `is wall gate=true` creates GatewayBuilding; `is gateway=false` suppresses base-ctor registration; no later Gateway::setup call occurs; gate handle/town must otherwise be valid. QOL completion registration can fill that gap. Conversely `is gateway=true` with valid native registration makes the extra completion call redundant. The actual default/modded gate records and whether native handle/town changes between construction and completion have **not** been inspected, so neither “all gates broken” nor “always redundant” is established.

## 2. Duplicate behavior is proved

- Steam **Town::addGate9512F0–951317** / GOG **950A10**: inserts hand into `Town::gates`, offset **390**, consistent with SDK `Include/kenshi/Town.h:458` (`ogre_unordered_set<hand>::type`).
- Steam insertion thunk730B→**293B90**; GOG actual target **293720**. Steam293C25 calls hash/key lookup D3480; **293C2F–293C3B** tests found node, returns that node and inserted=false before any allocation.
- D3480 checks bucket/hash, then at **D3510–D351C** calls hand equality through vtable+8; an equal key reaches found return. Thus calling addGate repeatedly with the SAME stable hand does not create duplicate entries or allocate another node.
- Only on miss does293C6D allocate0x30, copy the five hand identity fields and increment set size at293D41. New serial/re-key is a different identity: this is not stale-entry cleanup, town reassignment management or proof that duplicate registered gates are harmless across worlds.
- addGate itself has no lock. Existing native phase/thread guarantees must be preserved; a new completion post-hook cannot assume that every caller owns the same town-mutation phase as initial construction. Main/AI ownership was not proved by this static audit.

## 3. Smallest next diagnostic for gates

Before production change, use existing/read-only offline gamedata or a bounded external snapshot to identify one affected gate: exact building data ID; `is wall gate` and `is gateway`; final stable hand; stored/native town and getRealTown equivalent data; whether the same hand is in that town's gates set; constructed vs loaded/upgrade path. **No native calls from an external tool.** If layouts/flags cannot be read reliably, report incomplete instead of guessing. Two control records suffice: freshly built affected gate and loaded/known working gate.

If the missing-registration case is then demonstrated, keep a separate optional event-only gate fix, preserve native town-choice policy, validate final stable handle and eligibility, skip existing entry using native set semantics, and make all native/profile dependencies ready before installing. Do not bring nearest-town16000 override, auto rebuild or broad QOL NavMesh toggle along. An idempotent insert does not replace lifecycle/thread admission checks.

## 4. Post-drop: native contract and concrete upstream bug

QOL Q.cpp:1155–1173 calls original getDropped, then only without movement tracer, with non-null ou/navmesh. It calls:

`int r=getClosestPoint(pos,300,0,false,onMesh,faceKey /* initially0 */); if(!r) return; movement->building=getHandle(faceKey);`

Actual native functions (Steam → GOG):

| Function | Steam | GOG | Important contract |
|---|---|---|---|
| Character::getDropped |5CC110|5CC420|487B body; does not directly call NavMesh getHandle/closest-point. It invokes several movement/physics methods, so this alone does not prove the handle always remains stale.|
| NavMesh::getClosestPoint(bool water,...,key)|3A2E60|3A2A80|224B wrapper allocates/frees a16B Havok hit-filter using calling thread TLS, delegates to filter overload, preserves int return.|
| Filter overload/query core|3A21C0|3A1DE0|Read-admission CAS atNavMesh+200. Failure at3A2272–3A2279 returns **-1** before writing key. Normal result key==-1 produces0; success produces1 (3A24A0–3A24B0).|
| NavMesh::getHandle(key)|3A4280|3A3EA0|Read admission/section lookup, unlocks at3A4332, returns borrowed hand reference from tree node+18 or global null hand. Native doesn't return an owning snapshot.|

**Proven bad path:** read-admission failure ⇒ query returns-1 ⇒ `!(-1)` is false ⇒ QOL continues ⇒ initial key0 is treated as real query output ⇒ getHandle(0) may clear or replace `movement->building`. This can happen without a successful nearest-point query. Required acceptance is **r==1**, key initializedINVALID; on -1 or0 leave original building handle untouched. Zero key itself is not necessarily invalid, so do not “fix” this by rejecting key0 after an accepted query.

Other limits for a safe small adapter:

- Query/GetHandle are two separately admitted native operations. Native getHandle releases its read lock before returning a reference. Copying immediately, without callbacks/logging in between, reduces exposure but does not prove the referenced node survives concurrent unload; preserve existing world/navmesh-stop/thread guarantees, or add a verified safe-phase snapshot rather than retaining that reference.
- Do not use `if(navmesh!=null)` as complete readiness. Wrapper first dereferences the calling thread's Havok TLS allocator; wrong-thread/null TLS is not handled by this function. Existing getDropped call-thread and NavMeshStopSeen/world generation contracts need review before issuing any new query.
- A successful nearest point can lie up to300units away and on another floor/section. QOL ignores onMesh position and writes a building handle without teleporting. That is intentional bookkeeping, but nearest != proof of actual supporting building. Verify carry-inside→drop-outside, outside→inside, ragdoll/no tracer, unloaded/stopped navmesh, no result, normal tracer, and both existing KEO throw-out chains.
- Preserve exterior/null-hand success as a legitimate way to clear an old interior handle; blindly retaining the old hand whenever the new hand is null would reintroduce stale interior ownership.
- QOL resolves before/after `.getBuilding()` and creates names/log strings even when verboseLog is off. A production adapter should avoid these extra native lookups/allocations on the event path and use existing bounded diagnostics. Native closest-query itself allocates filter16B; do not advertise zero allocation.
- KEO's existing getDropped post-hook remains owner of walls classification and throw-out hold (`src/fixes/world/throwout.cpp:218–247`). Merge a verified handle refresh as an ordered substep rather than installing a second parallel detour. Neither feature writes NPC positions.

## 5. Steam/GOG verification limits

Ten windows (348/56/39/286/1035/127/487/224/804/370B) independently matched after masking ONLY RIP displacement and external relative call/jump encoding; member offsets, sizes, constants, and internal branch topology remained exact. SDK table pairs independently determine named methods. Full per-window hashes and instruction counts are retained in `gate-drop-native-proof.json`. This is evidence of the contracts above, **not** an audit of every transitive external callee or a claim that an arbitrary QOL raw RVA is supported by KEO's existing GOG profile. `.pdata` is split for several functions: the first13-byte entry at5618B0 is NOT the whole completion function; inspection continued through actual ret561A0B.
