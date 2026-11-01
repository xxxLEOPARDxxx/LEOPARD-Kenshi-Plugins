# Satellite terrain and unloaded-geometry audit — 2026-11-01

This is a static, read-only audit for the optional GLOBAL map satellite renderer. No engine/process calls, UI actions, capture, installation or world/save changes were performed for this audit. BestMinimap remains unchanged. A correct local 512-square image is evidence for the diagnostic pipeline, not full-world coverage or completion of the requested feature.

## Exact native profile and evidence

All addresses below are RVAs in the following installed files, not the GOG addresses in SDK comments:

| File, relative to the installed Kenshi directory | SHA-256 | PE timestamp / image size |
| --- | --- | --- |
| `RE_Kenshi/Kenshi_x64.exe` | `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e` | `65D604D7` / `232C000` |
| `OgreMain_x64.dll` | `7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6` | `5CA5F929` / `9C9000` |
| `Plugin_Terrain_x64.dll` | `53afd1e4faf35020556d33f57007d51c2a9d2330b630e39a67161ee850b7a239` | `65FB1113` / `56000` |

Evidence was obtained with static PE/import/export parsing and Microsoft dumpbin disassembly. Matching SDK declarations are in `D:/DEV/Kenshi/deps/KenshiLib/Include/kenshi/Terrain.h`, `Town.h`, `ZoneManager.h`, and `Include/ogre/OgreSceneManager.h`. Headers are declaration references; their direct Camera/Terrain field layouts are unsafe in the probe's NDEBUG build. Continue using exact guarded native exports and proven byte offsets rather than SDK object sizes or derived fields.

The canonical diagnostic is `tools/diagnostics/satellite-capture-probe`. The historical research copy under `D:/DEV/Kenshi/research/global-satellite-20261031/capture-probe` is frozen.

## World extent and the required 16 primary cells

RE `KingOfRenderThread::loadTerrain` is RVA `873A60`. At `873C19` it calls imported `Terrain::setTerrainScale` with width **294912.0** (constant `17221E8`) and height **9800.0** (`170F87C`). Terrain implementation `14070` stores these values and establishes centered horizontal bounds **[-147456,+147456]**. The native ambient-map constants independently use the same +/-147456 extent.

For full terrain extent, the requested 4x4 primary grid therefore has:

- 73728 world units per cell side; centers -110592, -36864, 36864, 110592 on both axes.
- 8192x8192 output pixels per cell; 9 world units per output pixel.
- 32768x32768 logical output across all 16 cells.
- Fixed world-grid framing and orientation across captures. Neighboring capture gutters must extend the same projection, then be cropped to the primary cell; changing height-dependent ground planes would reintroduce seams.

Do not bake the earlier illustrative 51200-unit cell width into production. Before output, reconcile the exact terrain extent with the actual native global-map affine mapping. `MapScreen::worldToMapCoords` (`48B950`) uses worldBounds origins at194/198 and world sizes at1A4/1A8; `BestMinimap/BestMinimap.cpp` lines704 onward calibrates through that native function. A map crop or replacement texture may differ from the terrain bounds.

## Terrain data is available without activating simulation zones

Installed `data/newland/land/fullmap.tif` has one classic little-endian TIFF IFD: width/height16385, one16-bit sample, compression1, one strip. File length is536980180 bytes. This is a genuine global height source; no terrain image was synthesized during the audit.

Native `openStream` (`141C0`) opens the TIFF and stores height scale **9800/65535** at `142C9`; horizontal sample scale uses the even part of the image width divided by terrain size (`142F5..1431B`). Thus the height source interval is18 world units, while the requested output pixel interval is9. Increasing output resolution cannot invent more height detail.

Native `getRawData` (`135B0`) locks the Landscape's TIFF-access mutex, invokes rectangle reader `21800`, then unlocks. The reader clips image edges and performs per-row seeks/reads. No ZoneManager/Town activation path was observed in this data-read path. A future caller must still validate region dimensions, multiplication overflow, destination size and cancellation before entering it. Do not issue millions of individual native height queries.

## Exact available entry points

Terrain exports are from `Plugin_Terrain_x64.dll`; Ogre exports are from `OgreMain_x64.dll`. Listed source signatures describe methods, not a promise that a free-function typedef has identical aggregate-return ABI.

| Method / signature | RVA | Export name or native role |
| --- | --- | --- |
| `SceneManager::_getEntityMemoryManager(SceneMemoryMgrTypes)` | `41E40` | `?_getEntityMemoryManager@SceneManager@Ogre@@QEAAAEAVObjectMemoryManager@2@W4SceneMemoryMgrTypes@2@@Z` |
| `SceneManager::createMovableObject(const String&, ObjectMemoryManager*, const NameValuePairList*)` | `2CB850` | Native factory entry; full exported declaration matches OgreSceneManager.h2884 |
| `SceneManager::destroyMovableObject(MovableObject*)` | `2CC2A0` | `?destroyMovableObject@SceneManager@Ogre@@UEAAXPEAVMovableObject@2@@Z` |
| `Terrain::setTerrainScale(float,float)` | `14070` | `?setTerrainScale@Terrain@@QEAAXMM@Z` |
| `Terrain::setPatchSize(int)` | `13F60` | `?setPatchSize@Terrain@@QEAAXH@Z`; power-of-two requirement |
| `Terrain::setDetailThreshold(float,float,float)` | `13F00` | `?setDetailThreshold@Terrain@@QEAAXMMM@Z` |
| `Terrain::setOgreBuildLimits(int,int)` | `14190` | `?setOgreBuildLimits@Terrain@@QEAAXHH@Z`; onscreen/offscreen limits |
| `Terrain::setMaterialDistance(float)` | `13C80` | `?setMaterialDistance@Terrain@@QEAAXM@Z` |
| `Terrain::setDefaultMaterialName(const std::string&)` | `136C0` | `?setDefaultMaterialName@Terrain@@QEAAXAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z` |
| `Terrain::openStream(const std::string&) -> bool` | `141C0` | `?openStream@Terrain@@QEAA_NAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z` |
| `Terrain::closeStream()` | `14360` | `?closeStream@Terrain@@QEAAXXZ` |
| `Terrain::getRawData(int,int,int,int,char*) const -> uint64` | `135B0` | `?getRawData@Terrain@@QEBA_KHHHHPEAD@Z` |
| `Terrain::getDebugInfo() const -> Info` | `14B40` | `?getDebugInfo@Terrain@@QEBA?AUInfo@1@XZ` |
| `Terrain::_notifyCurrentCamera(Camera*)` | `12810` | Native virtual notification |
| `Terrain::_updateRenderQueue(RenderQueue*,Camera*,const Camera*)` | `129C0` | Native virtual submission/build path |
| `Terrain::frameStarted(const FrameEvent&)` | `12CB0` | Native FrameListener path, this points to secondary base at+170 |

Exact factory export for a future typed GetProcAddress adapter:

```text
?createMovableObject@SceneManager@Ogre@@UEAAPEAVMovableObject@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@PEAVObjectMemoryManager@2@PEBV?$map@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V12@U?$less@V?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@2@V?$STLAllocator@U?$pair@$$CBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@V12@@std@@V?$CategorisedAllocPolicy@$0A@@Ogre@@@Ogre@@@5@@Z
```

`getDebugInfo` has six uint64 fields: memory, patches, visible, triangles, ogre, unbuilt. Disassembly `14B40/14C30` proves **RCX=this, RDX=48-byte output buffer, RAX=output buffer**. A free-function returning Info by value would place its hidden return parameter differently; do not cast this method to that signature. An explicit guarded adapter returning `Info*` with `(const Terrain*,Info*)` matches these observed registers. None was invoked in this audit.

The native game itself creates `"Terrain"` via its SceneManager factory at `873B31`, using `_getEntityMemoryManager(1)` at `873B1E` (SCENE_STATIC). It configures visibility flags64, render queue25, thresholds8/8/0, material distance30000, build limits250/100, patch size16 or64, default material `DistantTerrain`, and the fullmap stream. These are observations, not recommended unconstrained capture budgets.

## Orthographic projection versus native refinement

Installed `data/materials/deferred/terrain.hlsl` applies a generic worldViewProj matrix. Native TerrainCamera construction/update (`12950`, `11B10`) copies world-relative camera position, viewport height and six cached frustum planes. Visibility traversal `3960` calls the six-plane AABB test `11C50`. These paths can render/cull orthographic geometry.

Refinement is different: native patch metric `3B30` computes viewportHeight divided by max(1, AABB.distance(adjustedCameraPosition)), multiplied by patch error at+84 and an additional distance ramp. For this distance calculation it replaces camera Y with `lerp(cameraY, patchCenterY, 0.4)`; the called helper `E690` explicitly computes `a + (b-a)*t`. No orthographic-window width, FOV or projection-type branch occurs in this metric. Consequently changing camera altitude may change detail even when orthographic pixel scale is unchanged. Stable screen-error detail and matching neighboring tile edges require proof or a private refinement policy; valid projection alone does not prove them.

`_notifyCurrentCamera` selects current visible Landscape patches (`1E30`); `_updateRenderQueue` builds missing native drawables subject to onscreen limits. `frameStarted` drives Landscape update (`1920`) and later offscreen builds. An immediate render cannot promise the requested far cell is fully ready. Native `getDebugInfo` can support bounded warmup telemetry, but stable counters alone do not prove correct LOD or complete buildings.

## Ownership, teardown and the retained-camera hazard

Never copy-construct Terrain for isolation. Copy constructor `11570` copies mStream1B8 (`1169F..116A6`) and Landscape1C0 (`116BD..116C4`) as raw pointers. Destructor `12540` closes the stream and deletes the Landscape. Such a copy would share owned native state and risk double destruction.

A fresh constructor `11DB0` allocates its own Landscape, installs native geometry callbacks and registers a Root FrameListener at `12442`. Destructor removes that listener (`125A6..125B4`) before stream/Landscape destruction. Landscape stream startup (`2360`) starts a worker. Close (`23D0`) clears its running flag and, when requested, waits for the worker via a10ms polling loop (`2410`); teardown is not guaranteed to fit a strict frame-time budget. Keep the private camera alive through private Terrain destruction, then destroy its node/resources in ownership order. Do not manually allocate using SDK sizeof(Terrain).

Native persistent helper mBloodCamera at Terrain+2F0 is a TerrainCamera descriptor:

- Constructor `12950`, store at `12968`, saves a **borrowed Camera pointer at descriptor+0**.
- Update `11B10` reads that pointer repeatedly; it changes cached position/viewport/planes, not the borrowed pointer.
- Notification `12810` updates an existing descriptor; if null, it allocates78 bytes (`128C3`), constructs from current LOD camera (`128F7`) and stores it at Terrain+2F0 (`12932`). Y<=0 skips that creation.
- Initial native terrain loading normally calls notification with the main camera at `873E2E`, but that does not establish a universal non-null precondition.

The reviewed diagnostic correction refuses uninitialized/foreign helpers before workspace creation, verifies unchanged descriptor identity and borrowed main LOD camera before restoring178/180, and refuses Drop before deleting resources if the helper could borrow ownCamera. Guarded fields and exact code windows are in `NativeTerrainCache.h`, `render_abi_profile.json` and CaptureProbe.cpp. This closes the identified retained-own-camera path. It does **not** make Terrain selection, build queues, materials, helper caches or LOD changes transactionally reversible.

## Genuine distant buildings: useful path and coverage limits

RE `Town::showDistantTown(bool)` is `92CED0`; it may create a DistantTown for the FCS `"distant town"` flag. DistantTown show (`D8620`) can queue a mesh load and rebuild. Rebuild (`D8310`) merges geometry, replaces native Entity/node state and invokes the mesh-cache writer (`D6020`). It is not a read-only visibility toggle.

Native mesh-name builder `D6360` creates `distant_<town-handle-id>.mesh`; cache output path is `./data/meshes/distant/`. The installed directory has282 mesh files (7182137 bytes total). `data/buildings/distant` also contains genuine per-building proxy meshes and their shared texture. This inventory is not proof of matching town coverage in the current modded save.

Installed `data/materials/deferred/distant_town.material` and `.hlsl` are directly useful: world/projection transforms, normals and radial depth already feed the native GBuffer, using `distant_diffuse.dds`. A private Entity instance from an existing mesh, attached at the town's proper transform, avoids Town visibility/async-world mutation. Existing resident proxy Entities can also be used for a tightly scoped visibility comparison with exact restoration.

Do not call `ZoneManager::loadDistantTowns` (`A0A470`) as a capture preloader. It traverses the global zone/town table and changes the global show/hide set around the main-camera center. Cached vanilla town meshes do not establish coverage of mod-added towns, destroyed/changed buildings, or player bases. Those require an explicit geometry-only source and save/mod identity/invalidation policy, without activating NPC zones.

Native detailed terrain-material callbacks are also not isolated: actual callbacks `A16400`/`A161E0` access shared ZoneMap overlays and material bookkeeping. Start the private Terrain experiment with `DistantTerrain` and no shared callbacks. The installed default material uses genuine `distant.dds`; detailed biome/overlay reconstruction remains a separate proof.

## Next controlled experiment

1. Retain the BloodCamera guard for any diagnostic using the main scene; keep requests blocked until root completes verification/installation.
2. Validate native MapScreen affine coverage against the294912-unit Terrain extent. Define the16 primary cell bounds once.
3. Create one **fresh private native Terrain**, with its own Landscape/stream/camera, restricted visibility and bounded build settings. No shared ZoneMap callbacks, NPC activation, Town creation, teleport or save writes.
4. Render one **73728-unit primary cell** at512 square first. Warm the same cell across a bounded number/time of frames; log native build counters, depth coverage and memory. Compare two altitudes at unchanged ortho width to expose refinement dependence. No microtile image traversal.
5. Add one genuine cached-town proxy privately; verify its transform, shading and teardown. Report absent proxies as missing coverage.
6. Prove stable terrain detail/material coverage and neighbor alignment, then raise the same primary-cell target toward8192 and measure allocations/frame time. Produce the16 final cells sequentially, retaining a bounded working set and shared lighting snapshot; internal geometry subdivision is allowed without changing capture-grid framing.

Sun-only private lighting is currently diagnostic. Native local lights, shadows, fog/water, runtime day/night refresh, complete unloaded geometry and final capture assets remain pending until separately implemented and verified.

## Review verification

Fresh independent executions after the BloodState correction:17 production native-terrain ABI checks,33 policy/request/cleanup checks,11 attachment checks,13 pose checks; source contracts and actual PE startup/import contract passed. These executions do not call the renderer. Root separately reported the full216-check build. This audit performed no gameplay validation and makes no FPS/leak-free claim.
