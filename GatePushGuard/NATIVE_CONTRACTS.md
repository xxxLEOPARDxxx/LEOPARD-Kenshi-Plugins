# Native contracts and independent review

All addresses below are **actual Steam RVAs**, resolved from the installed manual KenshiLib export stubs and `Steam_1.0.65.br` with `D:/DEV/Kenshi/tools/slots.py`. The audited executable is `RE_Kenshi/Kenshi_x64.exe`, SHA-256 `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`. Header comment addresses are not used as Steam evidence.

The reproducible read-only audit is `tests/audit_native.py`. Full native instructions remain in the private build evidence `D:/DEV/Kenshi/build/GatePushGuard-native/native-disassembly.txt` and `native-audit.json`; the release summary records hashes, export/slot bindings and import validation.

## Absolute movement target

`Character::teleport(Vector3 const&)`, slot1767 / RVA`0x5c9bf0`, assigns the supplied vector to `Character+0x48`. At`0x5c9c04` it calls IAT`0x22461e0`, verified as Ogre `Vector3::operator=`. It then passes the same absolute vector to the movement object at`Character+0x640`, vtable`+0xb8` (`_setPositionAndTeleport`). Passing a displacement would corrupt placement; the adapter passes the last observed absolute outside position. A regression contract was observed failing against the preliminary displacement call and passing against the corrected adapter.

Both teleport overloads are observed as discontinuities; the rotation overload is slot1766 / RVA`0x5c7a50`. The plugin's own correction has a thread-local actor guard restored by `__finally`; another actor's nested teleport remains an external discontinuity. A per-actor revision invalidates other doors' pre-correction history, while the confirmed outside target at this door remains armed against repeated pressure. External teleports have0.5s suppression; own corrections have no suppression window. Existing revision records are refreshed by each observation and retained at least0.5s afterward (valid sample gap is at most0.35s), so expiry cannot silently erase a discontinuity while an old history remains valid. Rehashing preserves exact revisions.

## Door identity and geometry markers

The native getter prefixes establish these fields:

| Getter | Steam RVA | Field |
|---|---|---|
| `getDoorPosition` | `0x332810` | `DoorStuff+0x3b0` |
| `getDoorPosOutside` | `0x2969f0` | `DoorStuff+0x3bc` |
| `getDoorPosInside` | `0x338630` | `DoorStuff+0x3c8` |

`setupPhysicalUT`, RVA`0x29cc50`, derives the markers from its own physical actor/shape, direction and placement. At`0x29d1a3` the parent building's construction state is consulted, so completion is checked on the parent. The render aperture comes from the door object's own `physical->staticEnts` local model boxes transformed by their scene nodes. Parent building/wall bounds are not used. Compound/nested or ambiguous model layouts are skipped. Live coverage across vanilla and modded door assets remains required.

Independent review checked the apparent child/parent handle ambiguity: `DoorStuff::_NV_setHandle` RVA`0x556750` copies all five supplied fields unchanged. In `RootObjectFactory::createBuilding` RVA`0x57c1e0`, the door branch calls the door constructor at`0x57d25c`, appends that child to the parent's door list, then enters the common registration path. At`0x57df01..0x57df29` it registers the created **DoorStuff child** in the zone-owned manager (`zone+0x98`);`0x57df2c..0x57df4f` copies the returned full handle to the child. `hand::getBuilding` RVA`0x791d70` reaches that manager through the same full handle. The parent-only lookup concern was not substantiated.

## Spatial query rejected after independent native review

The SDK's `GameWorld::getCharactersWithinSphere` argument named `maxFar` **does not bound the initial native scan or output allocation**. At Steam`0x785b90`, the comparison at`0x785c65` merely selects an additional near query. Its underlying six-argument `ZoneSpacialGrid::getObjects` (thunk`0x25d7e` →`0x9fe340`) does not use the supplied maximum to truncate cell output. The five-argument wrapper at`0x100980` forwards into the same implementation.

This primitive was removed completely from the plugin. Fixed post-filtering of an unbounded native query is not claimed as bounded work.

## Bounded native movement observations

The replacement hooks `CharMovement::_NV_update(float)`, slot2093 / Steam`0x65f510`, calls the original first, then reads POD values only. `CharMovement::getCharacter`, slot2135 / Steam`0x2ae030`, is a direct `mov rax,[rcx+0x3a8]`; its byte prefix is checked at runtime before hooks are installed. `Character+0x640` is independently corroborated by the native teleport path.

A private 512-cell index rejects movements away from watched doors; 64 regions of radius below64m occupy at most256 of those hash cells. No whole-world registry is walked. A fixed table of4096slots holds at most2048 deduplicated actor events and rejects a saturated batch in constant time. Only full identities, positions and timestamps cross from the movement callback; no native pointer survives it. The main-loop callback drains that queue and resolves at most128 nearby actor observations for each of8 door slots per frame. No native spatial query or native result buffer is used.

## Actual crossing provenance

Independent review found a curved-path false positive in an initial hysteresis implementation. With half-width2, path`(0,-.2)→(2.5,-.01)→(2.5,+.01)→(0,+.2)` crosses the plane outside the aperture. Joining the old outside anchor to the final point invented a straight path through the door. The production core now records whether the **actual consecutive segment** crossed the aperture. The old anchor supplies only the return target. This exact path and a path traveling along the plane to an outside edge were observed RED, then GREEN after correction.

## Acceptance limits

Native prefix/import checks establish matching code/layout, and production tests establish the tested geometry/queue/cache decisions. They do not prove hook scheduling, asset coverage, correct collision synchronization, game performance, or sustained memory behavior. Those require the live acceptance cases listed in README. No process was opened, native game function executed, game DLL installed or Workshop item published by this implementation task.
