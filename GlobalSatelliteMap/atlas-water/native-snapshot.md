# Native water snapshot plan and contract — 2026-11-02

Implement only a render-thread adapter in new `NativeWaterSnapshot.h` plus dedicated tests/audit. Reuse the verified terrain profile/SRV identity and COM helpers. The adapter performs no rendering, disk polling, world traversal, resource loading, native writes or water-manager lookup.

1. Test actual production validation of typed finite parameters, absolute height, stock basis, sunlight/ambient semantics and native samplers including bias.
2. Test retained five-view/device/sampler ownership with real D3D11 WARP, including a one-visible-mip specular cube and unchanged-descriptor reuse.
3. Add the pinned native export adapter, current material/parameter/resource reads, fixed error text and a no-query OFF branch. Compile it with VC10 and the current SDK without invoking it.
4. Audit current source/module bytes against the prior research and new embedded profile. Run fair functional negative controls; document the remaining live-observation boundary.

`NativeWater::Snapshot::Read(enabled, sun, ambient, generation, sourceProfileVerified)` returns S_FALSE without native queries when disabled. Enabled reads require validated background water source pins and a current world. The caller serializes Read/Render/Clear on the render thread, clears on world/device changes and retains the snapshot through every block in one water update. No snapshots are taken while water is OFF. Errors remain available in a fixed buffer for the caller's always-visible error path; successful detail belongs behind the module's Debug switch.

No existing source/build/INI/helper files are changed by this task. Root owns runtime diagnostics, the terrain HDR cutpoint, renderer mip admission and integration.

## Implemented interface

The proposed interface above is implemented. `Read` returns S_OK only after the whole snapshot validates. `Valid(generation)` must remain true before use. `Values()` returns the copied 192-byte `AtlasWater::Uniforms`; its water/material/light/time rows are populated, while tile/dispatch/source-window rows remain zero for the caller to fill with the selected GST2 window. Sun OFF zeros native water RGB; ambient OFF zeros native environment W. The current absolute scalar comes from `SharedWaterParams.waterHeight`, not `waterHeightRel`, an owner pointer or initial100. Game time is the current `SharedSkyParams.gameTime`, with no wall-clock substitution. The scene-root getter receives the audited native **index1**, via `WaterRootIndex`; it deliberately does not use this SDK's `SCENE_DYNAMIC`, which equals0.

`Inputs().View(i)` and `.Sampler(i)` expose the five retained native inputs, ordered colour, flow, normal, irradiance and **water** specularity. These map to helper t3..t7 and s0..s4. `.Mips(i)` reports the real visible range, retaining MostDetailedMip, view gamma and the native SRV itself. `.Device()` is obtained from the first actual view; all five must match its COM identity. Each native TexturePtr and an independent SRV reference remain retained, together with material, VP/FP parameter owners and the three shared sets. The shared sets are obtained once each per read. Five equivalent sampler objects are created only at initial capture, device replacement or descriptor change; unchanged reads reuse them. All filter triples, addresses, anisotropy, border colour and noncomparison mip bias are captured; native FO_NONE still uses MaxLOD=FLT_MAX. `DepthBiasConstant()` and `DepthBiasSlope()` record the actual finite pass values for diagnostics; no coastline-bias correction is invented.

Texture selection is **passive**: guarded native reads inspect current frame+0, named-content+0x13c, failure flag+0xb8, native name span+0x148/+0x150 (40-byte entries), and TexturePtr span+0x168/+0x170 (16-byte entries). Counts must agree and be1..64; the current frame must lie inside both spans, and the selected pointer/controlblock must already exist. The concrete texture type and pure Resource::isLoaded must validate before retaining the pointer. Missing/unloaded resources leave water unavailable. `_getTexturePtr` is never invoked: the audited native implementation calls ensureLoaded and can load/allocate even when the parent material is loaded. Current rendering/retirement serialization and the pinned material owner protect these borrowed spans while copying the existing native owner.

`LastError()` is a fixed 256-byte buffer. Read errors clear every retained snapshot resource/value and return failure; generation changes clear the prior snapshot before reading the new one. An unverified source profile fails before native queries. Disabled Read clears owned references and returns S_FALSE before profile/world/material queries. Caller code should avoid invoking Read entirely while water is OFF, except once to retire an existing snapshot.

Example caller sequence (no integration is added here):

```cpp
HRESULT hr = water.Read(waterEnabled, sunlight, ambient, generation,
                       validatedWaterSourceProfile);
if (hr == S_OK && water.Valid(generation)) {
    AtlasWater::Uniforms values = water.Values();
    // Fill current GST2 tileBounds/sourceWindow, then combine retained inputs
    // with the caller's <=512 RGBA16F terrain HDR and coverage/height views.
    // Retain this snapshot through every block and final exposure operation.
} else if (FAILED(hr)) {
    // Report water.LastError() through the caller's always-visible error path.
    // Keep displaying terrain. Detailed successful diagnostics require Debug.
}
```

The caller must hash all `NativeWater::SourcePins` in its background source-validation job, along with the same exact engine/Ogre/D3D11 module hashes already required by the terrain profile. It sets `sourceProfileVerified` only for the current validated generation. Snapshot performs no file I/O. The pinned source closure comprises water.material, water.hlsl, common.program, lightingFunctions.hlsl and shadowFunctions.hlsl; current runtime pass names must be WaterVP/WaterFPDistant and all five selected unit names must match. These checks do not establish compatibility with arbitrary loaded shader overrides, transformed/replaced water meshes or another compiled native variant. This helper intentionally supplies the existing map layer's DISTANT/NO_REFLECTION path.

## Fresh verification

Run `run_native_snapshot_tests.ps1` using VC10 and Windows SDK7.1. Fresh result: **293 checks PASS**, including real D3D11 WARP resources and extracted actual production Read, StockBasis and UnitTexture methods with bounded fake-native seams. The suite validates finite/ranged rows, ordered bounds, exact float types/array/range checks, overflow, stock quaternion/scale, distinct root0/root1 dispatch, aggregate-return output identity, passive current-frame selection/retention, null/unloaded/wrong-type resources, mismatched/reversed/misaligned/over-cap spans, content/failure flags, all64 filter triples, real sampler descriptors, one-visible-mip water specular cube, nonzero MostDetailedMip, sRGB view preservation, exact retained COM-reference lifetime, unchanged sampler reuse, changed bias, cross-device refusal, wrong dimensionality, unchanged context bindings, OFF/no queries, source guard, generation clearing and error retirement.

Seven independently compiled functional mutants are rejected: sun W-only disabling, omitted mip bias, invented minimum8 specular mips, omitted float-type validation, duplicate retained SRV references, wrong native root0 and the active `_getTexturePtr` path. Each compiles both the complete native adapter and the fixture executable, then fails its named assertion; compile failures are not accepted as negative controls. The initial production value validator and the actual root-selection regression were also observed RED before their respective fixes. Logs are in ignored `_generated/native-snapshot`, `_generated/native-snapshot-root-red.log` and `_generated/native-snapshot-last.log`.

The **full native adapter** is separately compiled to an object with VC10 `/GL`; it is not linked or invoked by the runner. Compile-time assertions verify GpuConstantDefinition48 bytes and field offsets0/8/24/32, native FloatConstantList32, and owner-pointer16-byte layouts. The actual pinned float-list getter and const pointer getter are used for bounded copies; manual fields reject native auto bindings. Scene-root orientation/scale use audited by-value aggregate getters with explicit hidden return pointers, never incompatible SDK reference signatures or a guessed water-owner pointer.

`audit_native_snapshot.py` checks the current three full module SHA256 values, five source SHA256 values, actual water include closure and **25 exact export names/RVAs/first24-byte windows** embedded in the new header. Six additional runtime code windows pin the passive layout: current-frame getter37ca30, frame-name span37dbfd, load-failed branch37dc2b, frame-slot address37dc3c, setter address proof37dcb0 and unload end/begin proof37e5a7. These functions are inspected, not invoked to obtain the slot. The audit also verifies native504b water constructor9d671f passes literal1 and9d6728 calls IAT2246eb0, resolving to getRootSceneNode. The runtime path checks the loaded module profile and export/layout code-window identity before reading native objects. Audit output is `_generated/native-snapshot/profile-audit.json`. The audit does not launch or inspect a running game.

## Performance, ownership and remaining boundary

Read is a caller-scheduled render-thread operation. It traverses one fixed material/pass and five units, checks a bounded25-export table and copies a fixed uniform payload. No world scan, native parameter write, texture loading, material cloning, render dispatch, context binding, per-frame timer, unbounded cache or file polling is introduced. Ogre named-parameter/shared-set lookup can allocate temporary strings and perform bounded container lookups; this is not a claim of zero CPU allocations. Repeated unchanged reads perform no sampler creation. The five borrowed textures are retained, never copied into new payload textures. Read, use and retirement must remain serialized on the render thread; the caller owns cadence, generation checks, animation updates and budget accounting for the separate renderer.

Native objects are assumed valid under that serialized ownership contract. Profile/leaf-copy guards are not universal recovery from corrupt Ogre objects or unrelated heap corruption. No native manager/resource getter has been called by this task. Actual loaded water height, shader values, descriptors, sampler state, pause/time behavior and scene-root basis remain **runtime diagnostic evidence pending**.

The unchanged AtlasWater renderer still requires four irradiance/eight water-specularity visible mips. This snapshot accepts and preserves any valid >=1 native visible mips, including the installed water DDS's one stored mip. Root must observe the actual native view and address renderer admission separately; there is no synthetic mip generation or terrain-cube substitution. No water rendering, player visual acceptance, coastline raster parity, FPS or long-session leak conclusion follows from these tests.

Final candidate header SHA256: `a4ec7bba2c1a8077ab53e3ac53a526f2a847d36f601af162babe6ce3063185d2`. Independent review initially blocked the wrong SDK root index and active texture getter. Both are fixed. The reviewer independently reran the complete293-check suite, seven functional mutants, full adapter/all mutant adapter compiles and native/source/layout audit: **source/offline review CLEAR**, with this exact header hash unchanged. Runtime getter/snapshot/HDR/cadence/mip/budget acceptance remains a separate integration step.
