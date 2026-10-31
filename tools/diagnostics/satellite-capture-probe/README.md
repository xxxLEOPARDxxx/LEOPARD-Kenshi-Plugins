# Versioned diagnostic source, 2026-11-01

Canonical source now lives in ours/tools/diagnostics/satellite-capture-probe.
The research-directory copy is historical. The diagnostic was installed by root
on2026-11-01 after the user requested satellite-map work. RE logs confirm its
guard/startup succeeded. Control captures are still pending; the historical
built-only notes below describe the earlier stage and are not a runtime claim.
This is not a public mod or a completed16-tile renderer. Build with build.ps1;
images belong beside the installed diagnostic DLL, never inside this source tree.

# Private native capture probe — diagnostic development build

## Camera attachment correction, 2026-11-01

The first actual mode0 request reached the probe but failed in `SceneNode::attachObject`:
`Object already attached to a SceneNode or a Bone`. The matching local Ogre2 SDK documents
the cause in `OgreCamera.h:74-76` and `OgreSceneManager.h:1183-1186`: `createCamera` already
attaches the camera to the root scene node. The production `OwnedCameraAttachment.h` helper
now detaches only the newly owned camera before moving it to the probe node. It refuses the
main camera and main-camera node; an unattached camera needs no detach, and repeated attachment
to the requested node is idempotent. No shared/root node is deleted. Existing Update failure
handling and tracked-camera/node cleanup remain unchanged.

The extracted production helper first failed the already-attached regression (RED), then
passed11 attachment/ownership/partial-failure checks (GREEN). The scoped build runs these in
addition to13 prior policy checks, source contracts and the actual PE startup/import audit.
This corrects the observed setup failure; it is not a claim that mode0 rendering has succeeded.

This is a feasibility experiment for the requested global 4×4 atlas of 8192-square tiles and a native dynamic layer. It does not implement that full feature and does not generate any map art automatically. Source, build products, diagnostics and tests stay in this private directory. No global-map viewer/shared source files were changed during this spike.

`SatelliteCaptureProbe.dll` attaches only the native map update observer after matching the actual RE executable SHA-256 `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`, loaded PE timestamp/image size, and three native code windows. Normal KenshiLib exports install the observer; raw RVA function calls are absent. The byte profile also guards the DistantTown evidence against a different game build. Other plugins may cause a guard refusal if they modify these checked functions.

Only a fresh Ctrl+Shift+Y press while Kenshi owns foreground focus and its global map is open can request one shot. There is no timer capture, automatic traversal or batch renderer. `SatelliteCaptureProbe.ini` selects one mode:

| Mode | Controlled comparison |
|---|---|
| 0 | Actual main camera, unchanged view/projection |
| 1 | Independent scene-node camera copying that view/projection |
| 2 | Independent true orthographic camera |
| 3 | Same orthographic camera, temporarily also bound to `Renderer::camera` during workspace construction/update |
| 4 | Actual main camera temporarily changed to the same orthographic view |

One request writes a 512×512 final BMP and raw encoded GBuffer0 BMP, logs radial-depth coverage and compositor scene-pass camera bindings. Raw GBuffer0 is diagnostic YCoCg/material data, not a finished image. The target remains 512×512: it is not an 8K/full-world capture. `CurrentCell=1` chooses the current camera's cell in a fixed global 1024-metre grid; manual CellX/CellZ can compare adjoining or already resident distant regions. All orthographic modes use Y=4000 and the same world-space width/height, with north=-Z upward. There are no terrain-height queries, terrain-plane fitting or perspective microtiles. Rendering-distance/minimum-pixel camera culls are temporarily disabled for orthographic modes; this only exposes resources already resident in the scene.

The probe creates a disabled second instance of the actual native workspace definition. It refuses shared main/probe GBuffer surfaces, disables only its own `GUI` compositor node, then manually calls its workspace `_update()` inside the main workspace's documented `workspacePreUpdate` callback. MYGUI is a custom compositor pass, which explains why the old branch's viewport overlay switch did not remove it. Main UI widgets are not hidden or edited. Probe destruction happens in the following native map update, outside compositor iteration; identity membership includes both active and queued engine workspaces, including lists larger than64.

Exposure is held by a synchronous scoped `HDRSettings` change: MIN_LUMINANCE=MAX_LUMINANCE=0.9. The installed `hdrfp4.hlsl:195` clamps adapted luminance to that interval, so this removes per-region exposure variation on the first pass. MIN/MAX/adaptation rate are restored before main-view passes, including C++ exceptions. The original game camera pose, projection, clips, aspect, orthographic window, auto-aspect, render-distance/minimum-pixel flags, Renderer::camera binding and last viewport are restored by scope. Custom projection/view, windowed frustum, reflection and oblique near plane states are refused. A separate scope restores the same Terrain instance's mCamera and mCameraPosition to avoid leaving a destroyed temporary camera in this cache.

**These guards/scopes are source verification, not runtime proof of every native side effect.** Terrain notification may change additional LOD/patch/cache state not described by the SDK; main-light/shadow/fog custom passes may also use native global state outside these explicitly restored fields. This remains a required native/runtime investigation before production use. A C++ scope cannot recover an engine access violation or a driver failure. The diagnostic has not been installed or invoked.

## Concrete native findings

The installed `data/materials/deferred/deferred.hlsl:26..29,156..159` reconstructs world position along interpolated **perspective** far-plane rays: `normalize(ray) * radialDistance`. Geometry shaders (`terrain.hlsl`, `objects.hlsl`, `distant_town.hlsl`) encode `length(worldPos-cameraPos)/farClip`. Under an orthographic projection this reconstruction is wrong away from the reference plane; the CPU model regression demonstrates the mismatch. Therefore `PT_ORTHOGRAPHIC` by itself cannot establish correct native illumination/shadows. The native GBuffer output can be geometrically correct while final lighting is wrong. A later isolated shader/depth-reconstruction path must handle orthographic ray origins consistently across sun/local lights/shadows/fog/water. No game shader file was modified.

Independent-camera black output is still **unproven**. Modes0/1 separate camera registration/culling from view geometry; modes2/3 test Renderer camera binding; mode4 tests camera identity while holding geometry/projection fixed. GBuffer depth coverage, Terrain camera before/after notification and pass-camera logs distinguish missing geometry from custom-lighting assumptions. No result is invented in the absence of a real run.

Exact-PE DistantTown evidence is in the parent research's native disassembly files. The IAT calls from its +0x10 resource resolve to Ogre::Entity::getMesh, MovableObject::getParentSceneNode/setVisible and Node::setPosition. Thus +0x10 is an actual Ogre Entity, not an unknown gameplay wrapper. Its state3 resource can be inspected without activating a zone. Optional `ExistingProxies=1` considers at most8 town handles near the requested cell, selected by a bounded4096 native map-marker traversal. It temporarily enables only already allocated, mesh-loaded, hidden Entity resources and restores their prior Ogre visibility. It never calls Town::showDistantTown (that path can allocate/load/rebuild), changes the DistantTown state word, creates towns/buildings, invokes zone activation or changes save data. Missing/not-yet-loaded proxies are skipped. Discovered-map-marker scope does not prove all-world/unexplored/player/modded building coverage.

Additional exact native targets: DistantTown rebuild path RVA0xd8310, source/signature path0xd6750, cached mesh-name path0xd6360. These routines contain resource replacement and deferred work; the private probe deliberately does not call them. They are candidates for the next read-only audit, not established safe capture APIs.

## Validation and remaining sequence

`build.ps1` compiles VC10 x64 `/GL` and links `/LTCG`, using existing dependencies, writing only `build/`. Offline tests:13 fixed-grid/projection/cleanup checks and source restoration/bounds contracts. All16 KenshiLib and37 Ogre imports resolve against installed DLL exports. `verification.json` records source/DLL hashes and explicitly records zero runtime images.

Independent review found and fixed a >64-workspace cleanup leak. It also required the explicit Terrain camera-cache scope above. Review cleared those corrections while retaining the broader native cache caveat. The final small target-selection change filters existing proxies to the requested region and copies control-camera aspect/frustum properties.

Next root-controlled steps: inspect/review this private probe; if chosen, install only as a diagnostic and capture modes0..4 separately in one unchanged small scene. Compare geometry depth versus final light and verify normal camera/UI after each request. Check one resident hidden distant proxy with the option off/on. Only after those observations should the implementation advance to an isolated native-lit orthographic pipeline, render-only geometry acquisition/invalidation, seam-safe global mips and the16 real 8192-square output tiles. Never activate all NPC zones or relabel a static preview as the requested complete feature.

## Loader contract correction, 2026-11-01

The previous build exported C `startPlugin`; actual RE_Kenshi `Plugins.cpp:43` resolves only `?startPlugin@@YAXXZ`, so that build could never initialize. It was never installed or invoked. Both prior built DLLs were rejected by a new actual-PE regression before changing linkage. The private probe now exports C++ startup; the viewer also retains its separate C `MCM_Describe`. `build.ps1` now fails if the resulting PE cannot satisfy the loader, if native imports are absent, or if the13 offline/source-contract checks fail. Source-contract tests also reject the retained historical broken DLL fixtures. `loader-contract.json` records fresh PE evidence; this is initialization compatibility, not runtime capture proof.
