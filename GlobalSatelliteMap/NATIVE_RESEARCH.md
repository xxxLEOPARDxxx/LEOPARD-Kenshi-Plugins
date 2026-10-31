# Native geometry and lighting: read-only evidence, 2026-10-31

Target file: `D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe`, SHA-256 `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`. No process was opened and no native function was invoked. Addresses below are Steam RVAs resolved from the installed KenshiLib stub slots and `RE_Kenshi/RVAs/Steam_1.0.65.br`, then disassembled from this actual file with the installed VS2022 dumpbin. They are not the GOG comments in SDK headers.

## Verified DistantTown path

- `Town::showDistantTown(bool)` = RVA `0x92ced0`, slot 6713. `+0x4f8` matches the SDK Town::distantTown member. When asked to show and this pointer is null, the function checks town GameData field **`distant mesh`** (string at RVA `0x1685d48`; check at `0x92cf46..0x92cf65`). If permitted, it allocates 0x18 bytes and calls RVA `0xd5fd0` through thunk `0x40075`, passing the Town pointer. It then calls RVA `0xd8620` through thunk `0x18fa7` with the requested visibility.
- Small constructor `0xd5fd0..0xd5fdf`: stores Town* at +0; clears a state word at +8 and resource pointer at +0x10. It does NOT load or activate a gameplay zone.
- Visibility path `0xd8620`: checks/stores visibility byte +0x0c. If its resource exists, calls an imported render object function. On first show/state0 it builds mesh-loading arguments and stores returned resource at +0x10 (`0xd86d1..0xd8718`); positions its scene node using the owning Town position (`0xd8792..0xd87b9`). Later state logic can rebuild it; the underlying mesh loader/update callback and ownership thread rules still need auditing before reuse.
- `ZoneManager::loadDistantTowns()` = RVA `0xa0a470`, slot 7480. It reads the engine camera, converts to sector coordinates, compares to cached `distantTownCentre` +0x1681c8, then traverses a bounded sector window. It shows proxies for towns in unloaded zones (`0xa0a645..0xa0a666`, thunk `0x4ba6a` resolves to Town::showDistantTown); it hides town proxies removed from the previous window (`0xa0a800..0xa0a806`). This is NOT a globally resident complete world proxy set.
- Real packaged meshes exist under `data/meshes/distant/distant_*.mesh`. `data/materials/deferred/distant_town.hlsl:47..51` writes albedo/normal/depth to GBuffer, so that geometry can participate in native lighting. `distant_town.material` uses a normal shadow caster and native material shaders.

## Implication

Reusing native distant geometry is promising for the render-only layer. The existing path is explicitly camera-windowed and gated by town data. It does not prove every player outpost/modded town has a mesh, nor that individual construction/demolition is reflected immediately. Calling showDistantTown on every town in the live game would change the game's own render ownership and bypass its streaming window; it is NOT implemented here. Next audit: mesh-name resolution, loader/callback/destructor, rebuild state and dependencies. Prefer isolated geometry-only proxies sharing native mesh/material resources and copied placement/state, with bounded visibility, instead of force-activating zones or stealing the game's scene objects.

## Lighting/capture constraints

The actual `data/materials/compositors/workspace.compositor` connects GBuffer -> Lighting_HDR -> Resolve_HDR -> GUI. `main.compositor:73..130` owns per-workspace luminance/downsample/adaptation. The old branch used many independent perspective shots and per-shot ground heights; exposure correction could not remove geometric parallax or different shadows/LOD. A full capture needs a fixed global orthographic/off-axis projection, fixed exposure per pass, shared world grid and consistent neighboring mips. Native lighting in a proxy scene also needs camera-dependent shadow/frustum and sky/fog inputs isolated from the main view; an extra camera alone was unsuccessful in the historical probes.

The 4 x 4 / 8192 target defines final atlas storage, not a promise that sixteen main-compositor renders can see all buildings. A static atlas contains baked light. Day/night crossfading cannot replace changing native sun direction/shadows, lamp power/state or unsaved building changes. Those remain explicit later implementation/proof stages.

Private disassembly evidence: `D:/DEV/Kenshi/research/global-satellite-20261031/native-*.txt` and `native-distant-targets.json`. No renderer, world activation or capture generation was run.
