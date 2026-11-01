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
This corrected the observed setup failure. Subsequent live controls produced images as
recorded below; this does not establish a complete atlas or correct world lighting.

## Live controls, 2026-11-01

The installed attachment-corrected DLL (SHA256 `d8db85d1a8d4d7864b7a96cfbd92e8f124752b60e83e599396f3183095cf4a42`)
produced all five modes in a paused Hub scene, process31268,
startUTC2026-10-31T22:19:51.2039120Z. Raw BMPs, file hashes and filtered log are private
evidence under game/_codex_work/global-satellite-20261101/controls-31268/manifest.json.

- Mode0 contains Hub buildings and terrain; the earlier attachment exception is gone.
- Mode1 is an incorrect clone: GBuffer has101 encoded colors versus4367 in selected mode0,
  with different depth and missing buildings. This is not just HDR tint.
- Mode2/3 have byte-identical GBuffer0 BMPs (SHA256 `90b771b154ae8ce243ec72c9e87e1cbe0d199bcca915488a0521f37e797b5219`)
  but different final lighting: channel mean absolute difference38.276/16.504/10.864 of255.
  Mode3 temporarily binds the independent camera to Renderer::camera; mode2 does not.
- Mode3/4 visually share top-down geometry, with small nonzero final differences
  (mean absolute0.199/0.133/0.124). Correct shadows/fog/water remain unproven.
- After mode4, the global map and original perspective background remain visible.
  This does not prove quantitative FPS, long-memory, save/load or unloaded-world coverage.

The telemetry build added manual-request-only pose/matrix samples, no corrective pose refresh.
Node convertWorldToLocalPosition/Orientation read cached derived transforms, which the SDK
does not guarantee immediately after createChildSceneNode. Mode1 copies at creation, while
orthographic Aim repeats in the compositor callback. NodeSample records cached/local parent
data before copying; after-clone, before-aim and pre-shot record view/projection, culling/LOD
camera identity and nonfinite-entry count. That build called no Node *Updated getter, since it could
hide the baseline. Native review confirms camera getters update only camera caches. No new
long-lived ownership or frame-wide scan was added.

### Floating-origin cause established and candidate correction

Telemetry DLL SHA256 `67a2fcb75095c55b3fedc1013871f669e39e86379bc02583519c79a026449c8c`
ran in process7664/startUTC2026-10-31T22:44:16.6138408Z. Newly allocated ownNode cached
translation was0,0,0 at clone. Main and owned camera initially had the same derived position
230.612732,3120.27393,2399.58643 (view delta0.0491943). Before rendering, normal node update
changed the parent's cached translation to51212.375,0,-2622.64819. Owned derived position
became51442.9883,3120.27393,-223.061768, while main remained unchanged; view delta50057.6874.
Projection delta was0 and nonfinite count0 throughout. Thus copying with stale parent caches
applies the floating-origin translation twice; this explains the incorrect mode1 viewpoint.

`OwnedCameraPose.h` now refreshes the owned node's derived cache before converting/copying.
It accepts only the owned camera, its own node and that node's direct scene-root parent,
and refuses a root with further parents. The public getter may refresh the root/owned
ancestry and call node listeners; it never traverses all scene children. Ordinary camera
setters invalidate its view cache; no raw-field writes or extra invalidate call are needed.
The extracted production helper reproduced the live51km defect (RED), then passed floating
origin changes, borrowed-resource refusal, ancestry bounds and exception-before-write tests.
The reviewed fixture models ancestor refresh, rather than pretending only one node changes.
Runtime check of this correction succeeded in process6628/startUTC2026-10-31T22:54:02.5161986Z,
installed DLL SHA256 `ac91b11ff6f91582763d0cd2ec67a0be282874901070bddbadf8f79c7c5a62ca`.
Parent cached translation is now correct immediately after cloning and remains stable at
pre-shot. Main/owned positions differ by about0.00124m; view delta stays0.0927734 instead
of jumping to50057.6874, projection delta0/nonfinite0. A mode1 image again shows Hub
terrain and buildings. Exact matrix equality is not claimed (camera quaternion rounding
remains); this confirms removal of the spurious floating-origin shift only. Orthographic
lighting, shadows/fog/water and full-world coverage remain separate unfinished stages.

## Isolated position-only shader pass (candidate; not live accepted)

For orthographic modes2/3/4 the next build also runs one private512-square fullscreen quad
over the captured radial-depth texture. It uses the tested original helper at
GlobalSatelliteMap/shaders/OrthographicPosition.hlsl, embedded together with PositionProbe.hlsl
at build time. No game shader/material file, native MainLight provider or shared parameter
set is modified. The depth texture is explicitly CONTENT_NAMED; programs, material and
PF_FLOAT32_RGBA target are private. Native final BMP lighting remains unchanged.

The function retains per-pixel lateral offsets and solves forward distance from radial
depth, using Ogre's negative-Z view direction. Invalid/nonfinite/overflowing samples return
zero and an invalid flag. A conservative relative float-rounding allowance covers near
grazing subtraction; large-offset grazing depth is inherently ill-conditioned, so this
does not establish0.1m accuracy for arbitrary surfaces near the camera plane. The WARP
fixtures establish sampled4000m-forward cases and explicit tolerance boundaries.

Build runs actual HLSL on an isolated D3D11 WARP device (33cases), and compiles both real
probe vertex/pixel entry points as SM4. The initial perspective reconstruction failed23
cases before correction. At runtime the private output is read back as float4 and compared
with the current projection/depth: valid/invalid/nonfinite counts, maximum NDC reprojection
and radial error. Those checks establish algebra/pipeline execution only: native image Y
orientation, real geometry correspondence and all lighting still require live evidence.

One fixed private node/workspace definition pair is reused for the manager lifetime,
because matching Ogre exposes no singular definition-removal API. No per-shot definition
growth is permitted. A partial definition failure disables this diagnostic until restart.
The auxiliary workspace is removed before its material, programs, target or camera. If
manager identity changes while it is tracked, ownership is unknown: keep this one bounded
resource set, disable requests and require process restart rather than freeing a raw Pass
still referenced by another workspace. Production policy regressions cover this path.
All scans, allocations/readbacks and logs are explicit-request-only, not normal-frame work.

Position-only runtime acceptance: DLL `d6e8bf679497398f261dd048c8d73932f0e9f43a95a1cb6dd1d34e96f4e027c0`
in process20484/startUTC2026-10-31T23:19:01.2682270Z, mode3/cell-51,3 produced262144 valid,
0invalid/0nonfinite, maximum NDC error0, maximum radial error0.000370985419m. This confirms
real game shader compilation and float-target readback/algebra, not all geometry conventions.

## Private sun/ambient candidate (not runtime accepted)

SunProbe.hlsl reuses installed constants/PBR includes and a compatible explicit GBuffer
decoder; game shader assets are not copied into this repository. It reconstructs orthographic view position, uses explicit
camera right/up/back axes and a constant surface-to-camera direction. It retains native
metalness, gloss, emissive/translucency and sun/environment equations. The private material
clones configuration from Main_Lighting_NoShadow: slots0..2 become explicit named private
GBuffer references,3ambientmap/4irradiance/5specularity retain that source's configuration.
This is not proof of active RTW/CSM runtime texture identity or arbitrary shader-mod support.

SharedSkyParams, AmbientParams, SharedWaterParams and SharedDeferredParams are read through
const access after the geometry shot. Values are frozen into private shader parameters,
with no automatic or shared subscriptions. Native shared values/materials/programs are not
written. Matching native code stores a reference camera and bakes its X/Z into worldSize.zw;
therefore relativeWorld uses capturePosition-referenceCamera+captureBasis*viewPosition before
the stock ambient-map UV equation. This avoids double-counting reference-camera position.
The45 additional actual-HLSL WARP cases compare against independent global UV targets;
omitting camera delta in a test-only mutant fails all45 cases. Real sun PS compiles as SM4.

One additional fixed definition pair and per-request private material/fragment/FP32 target
are owned by the same manager guard. Sun workspace is removed before its resources and the
shared private position vertex program. Output logs the shared reference, camera delta,
world offset/size, sun direction/colour and linear HDR mean/max/nonfinite statistics. The
extra BMP is explicitly labelled noShadow_linear_clamped: it is a diagnostic conversion,
not a tonemapped finished map. There are no shadows, fog, local lights or water in this pass.
All16x8K geometry, day/night/shadow/water correctness and performance remain unfinished.

### Actual sun-colour failure and explicit binding correction

Two captures on process33260/startUTC2026-10-31T23:51:32.4721216Z recreated the private
resources without name-collision/ownership errors. Position readback remained262144valid,
NDC0/radial residual0.000354068638m. Sun output was finite but visually wrong yellow/green;
262138 valid pixels and HDR statistics alone did not establish correct colour.

Actual compiled DXBC explains the defect: legacy `sampler albedoBuffer:register(s0)` does
not also promise texture t0. The compiler assigned depthBuffer=t0,albedoBuffer=t1,
normalBuffer=t2 by first use, while material units remained albedo0/normal1/depth2.
The new reflection regression first rejected all three mismatches. Sun now explicitly
declares Texture2D t0..3, TextureCube t4..5 and SamplerState s3..5, and the build checks exact
names/types/dimensions/counts/slots for both original and embedded DXBC.

The decoder uses clamped integer Load, native30/255 edge weights, +X fallback and checker
Co/Cg parity. Its64 actual-HLSL WARP cases compare with an independent CPU reference,
including both parities, every border/corner and all-zero fallback. A no-clamp mutant fails
28 cases. The modern IBL adapter retains native cube mip/alpha scaling and pure BRDF calls,
without invoking legacy sampler-taking functions. Correct colours still need a new live shot.

For further shader iteration, a fixed optional SunShaderOverride.hlsl beside the DLL is
read only on an explicit capture. Missing file uses the embedded shader; read errors fail
that request. Source is bounded32KiB ASCII/noNUL, with a locked read handle through hashing;
the actual source SHA256 is logged. There is no filesystem polling. Build generates and
compiles/reflection-checks this exact expanded source before deployment. Validate future
overrides the same way before copying; the loader validates bytes, not shader semantics.
This is private diagnostic tooling, not a player-facing arbitrary shader loader.

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

## Exact native ABI correction and unattended capture requests, 2026-11-01

The compiled NDEBUG SDK removes MovableObject's debug member, shifting later
Camera/Terrain fields by eight bytes relative to the installed DLLs. Previous
private-field restoration assumptions were invalid; earlier runs do not establish
correct camera-cache restoration. Camera lastViewport/minPixel/culling now use
five typed native exports, checked against exact module hashes, PE metadata,
export RVAs and code windows. Terrain cache uses guarded primary/FrameListener
vtable identities and native camera178/position180 offsets, preserving all other
bytes. Unknown world/Terrain identity or failed restoration retains the one bounded
resource set and disables further requests. No shared SDK layout was changed.
Build verifies both installed DLL profiles before generating the included profile.
Twelve production byte-layout checks include the original FrameListener-vptr
regression and exact restoration of surrounding sentinel bytes.

For unattended root-controlled experiments, opt-in EnableRequestFile=1 in the
private INI permits changing Request to a new nonzero integer. The first visible
foreground-map poll latches the existing token without replay. A changed token
uses the same bounded capture path and native/save/load guards as Ctrl+Shift+Y.
Polling is limited to four reads per second, only in that foreground map; disabled
by default, with no autonomous traversal or timer captures. Zero never captures.
Shader source remains read only upon an explicit request. Ten token/clock checks
cover restart latching, repeat suppression, cancellation and unsigned clock wrap.

Full build: 142 actual HLSL GPU cases,33 policy,11 attachment,13 pose,12 native
cache checks;111 native imports resolve. Independent review caught and corrected
Terrain identity loss and post-restoration arming paths. Live colour acceptance
still requires a fresh game after installing this candidate. This is not the
finished global 16x8192 atlas or complete day/night/shadow/water renderer.

### Persistent native camera helper guard
Native TerrainCamera also owns a persistent borrowed Camera pointer. Existing
helpers keep their original camera; a missing helper can be allocated during a
capture and borrow the temporary camera. The diagnostic now refuses construction
unless the guarded Terrain+2f0 descriptor already exists and borrows the main LOD
camera. Scope exit verifies the same descriptor/pointer before178/180 restoration;
cleanup checks again before any owned-camera destruction. A missing/retargeted
helper retains the single resource set and disables requests. Native fields are
not overwritten or freed. Five new regression checks cover these cases; the full
build now passes216 checks and111 import resolutions. Native LOD selection/build
caches remain nontransactional; this is a diagnostic of the resident scene, not
the architecture for full-world rendering. See the separate terrain audit.

## Isolated native Terrain experiment (default off; private tooling)
IsolatedTerrain=1 opts a request into a fresh SceneManager/Terrain/stream/Landscape,
using genuine installed DistantTerrain/distant.dds and the global fullmap.tif.
PrimaryX/PrimaryZ select0..3 on the exact294912-unit world; one cell is73728m.
Altitude is bounded10000..30000 (12000 default); output stays512 for this proof.
This is not a16x8K renderer. Own scene positions are absolute; only the private
material removes shared worldOffset and sets the native map extent. The main
scene, main Terrain, detailed ZoneMap callbacks and NPC simulation are not used.
The sun pass's reference-camera correction is retained with private worldOffset0.

A fixed private global GBuffer definition has three native surfaces and no game
custom-light/GUI passes. Rendering progresses at most10Hz at the main compositor
boundary, within60seconds/600samples. Native build limits32/8; observed native
memory1GiB, patches131072, triangles16million and drawables65536 refuse excess.
These are observed-work budgets, not hard reservations inside the native worker.
Stable nonzero counters on3samples permit a diagnostic readback; they do not
prove correct refinement, missing buildings or final coverage. A timeout writes
no success output. Save/load, map closure, world/Root change abort and clean up.

Normal and partial cleanup remove private shader workspaces before their resources,
then the private GBuffer workspace, fresh Terrain (including FrameListener/worker)
BEFORE its borrowed camera and SceneManager. Never copy Terrain or its callbacks.
An ownership failure retains one bounded resource set and disables further work.
The private material clone has independent GPU params; native programs/assets and
shared parameter sets stay unchanged. Explicit getDebugInfo adapter matches actual
member aggregate ABI (this,out -> out), with exact export/code profile checks.

21 production policy cases cover primary-cell edges/world coverage/9m final pixels,
altitude/refinement readiness/deadline/counter budgets/unsigned clock wrap/owner
identity. Full237-check build and116 real import resolutions pass. Actual native
geometry rendering/lifetime/performance still requires root's controlled game run.

## Native private-worker teardown correction
Seven live factory captures exposed a native terminated-thread HANDLE leak (+1
per creation). Native startE4C2 stores HANDLE at Landscape178/TID180; close23D0
signals request188 and polls running170. Wrapper/callback destructors never close
HANDLE. Running starts0 until the thread is scheduled, so native close alone also
permits a partial-construction late-start race. Request188 is set1 before start;
the worker never resets it and checks it before TIFF work.

Only our fresh private Terrain uses a cleanup adapter. Under the exact module,
vtable and owned-object guards, it reads that worker descriptor, duplicates and
validates its kernel thread handle/process/ID, atomically signals only request188,
and waits up to5s for kernel termination BEFORE native Terrain destruction. On
uncertainty/timeout it retains resources and disables the diagnostic. After native
destruction, CompareObjectHandles verifies actual object identity before repairing
the omitted native HANDLE close; a closed/reused other object is not touched.
The duplicate is then closed. No process handle enumeration, TerminateThread,
main-Terrain modification or native global patch is used. This private diagnostic
requires Windows10+ CompareObjectHandles; public mod platform support is unchanged.

Microsoft documents direct object comparison at
https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-compareobjecthandles
and limits thread-ID validity to thread lifetime at
https://learn.microsoft.com/en-us/windows/win32/procthread/thread-handles-and-identifiers .
We do not infer future identity from a terminated thread ID.

19 actual Win32 fixture checks cover kernel join, delayed completion, one-byte
canaries, wrong type/ID, no duplicate allocation on repeated adoption, actual
CloseHandle, double completion and upstream close with an actually reused event
slot. Total fixture handle count returns to baseline. An omitted-close mutant
fails on GetHandleInformation. Three raw native-layout checks avoid reading the
uninitialized TID when HANDLE is null. Full259 checks/116 game+Ogre imports pass;
repeat game HANDLE/memory behavior must still be measured after installation.

### Live worker correction and world-grid proof
The fixed worker adapter ran in PID30032/startUTC2026-11-01T01:55:26.3274481Z
with DLL SHA502fac6be5d4c4ca4e5a92ca01f20116ead0a2e762528958ab81203951923c18.
61 explicit requests produced60 successful512 captures and one clean abort when
the native autosave became busy. All61 logged kernel join, repaired HANDLE close
and Terrain-before-camera/scene destruction. Ten same-cell repeats no longer
showed the previous +1HANDLE per capture. Whole-process HANDLE totals fluctuate
with the other plugins and save activity; they are not an exact resource census.

Two complete4x4 primary-grid passes succeeded. Every corresponding raw GBuffer
BMP was byte-identical across the16 pairs. This demonstrates repeatable coarse
world terrain coverage, not final8K refinement, shadows, water or buildings.
Evidence: game/_codex_work/global-satellite-20261101/isolated-grid-30032 and
isolated-grid-repeat-30032; worker log/metrics in isolated-native-worker-fixed-30032.

Memory acceptance remains OPEN. After another16-cell pass, read-only VirtualQueryEx
reported +217567232 bytes committed private memory (mapped/image totals unchanged).
Some burst allocation returns during idle, but a remaining growth trend needs
isolation. Do not scale Side to8192: explicit colour targets alone would require
3GiB, plus depth, staging and overlapping CPU readbacks. Fixed buffers/sampled
validation and bounded staging are required for the final renderer.

### Explicit resource and GBuffer-only diagnostic A/B
TerrainGeometryOnly=1 is private, opt-in, default0. It skips both Position/Sun
program/workspace construction and their depth/float readbacks, retaining native
terrain geometry, GBuffer rendering and one raw GBuffer BMP. It is an A/B tool,
not a completed lighting mode. Normal full requests keep the existing outputs.

Before construction and after successful Drop, telemetry uses three additional
exact-profile exports to report scene existence, nine private resource names,
active workspace count and four ResourceManager byte totals. The reported bytes
exclude native/driver caches. No global eviction, depth-pool cleanup, heap
compaction or game-resource removal is attempted. Source/ABI/PE guards and the
full259-check build pass; the new A/B mode still requires controlled live use.

### Material accounting correction after live A/B
The actual49-capture A/B in PID35284/startUTC2026-11-01T02:24:23.6256228Z
confirmed scene0/privateNames0/workspaces1 and stable texture/high-program byte
totals after Drop. Geometry-only16 passes retained roughly75MB then70MB at idle;
the full16 pass retained roughly218MB. These whole-process observations narrow
the issue but do not prove a particular native cache is leaking.

MaterialManager bytes instead decreased exactly13198 per geometry-only capture
and18995 per full capture. Exact Ogre disassembly identified Material::clone's
assignment copying Resource loaded-stateD0/sizeE0 without a manager load credit;
later unload debits that copied size. copyDetailsTo uses the same assignment.
This accounting error is separate from actual committed private memory growth.

Both private clones now use cleanup-owned fresh MaterialManager.create results,
then copy only one technique/pass configuration with the exact native Technique
assignment export. Its destination parent is checked before/after; material
receive/transparency-shadow flags are preserved with four exact accessors.
Resource loading/accounting metadata is never copied or patched. Build259 checks,
115 game/Ogre imports and independent review pass. Fresh-process before/after
MaterialManager balance remains the required live acceptance; no global counter
compensation or game-engine patch is installed.

### Opt-in fixed shader reuse experiment
ReusePrivateShaders=1 is startup-latched and default0. It retains exactly three
private native-manager-owned programs and bounded source text, while releasing
all per-request smart pointers, material parameters, targets, workspaces, cameras
and scenes. Manager identity, resource identity and source must match; a missing,
replaced or changed source refuses reuse until restart. Foreign names are never
adopted. Failed compilation remains Drop-owned and does not enter the cache.
Native managers release the programs at shutdown; the cache retains no Ogre
smart pointer across requests. No game cache is cleared or changed.

16 production policy cases cover bounded identities/source, collision, owner
change, replacement/removal and refusal to overwrite; an always-recompile mutant
fails the reuse case. Full275 checks/115 imports and independent review pass.
The next live A/B must baseline AFTER the first cached full capture and expect
cachedPrograms3/privateNames3 (programs only), scene0/workspaces1 and balanced
Material bytes. It tests compilation/program/driver cache churn together, not
FXC in isolation. No retention improvement is claimed before measurement.

Live reuse proof: PID35236/startUTC2026-11-01T02:50:43.9031687Z, DLL
c1cd6f1bfeb7d3dcc36c7299d7847540e54c59ea8c5d39f3766144b6bd7f35a4.
18 successful captures compiled3 programs total and reused them53 times, including
a deliberate changed-source refusal and successful recovery after restoring the
accepted source bytes. All19 partial/full cleanups joined the worker, restored
scene0/workspaces1/material10775230 and retained only3 named programs. This avoided
compilation churn but did not eliminate retained private memory (~200MB after a
16-cell pass). Evidence isolated-program-reuse-35236 and its related folders.

### One-lifetime primary-grid batch experiment
PrimaryGridBatch=1 is explicit/default0 and starts at primary0,0, ignoring the
single-cell selectors. One private Terrain/Scene/camera/GBuffer/worker covers the
fixed row-major16 cells. Save and both optional readbacks finish before moving
only the private camera to the next absolute cell centre. Native notification
and FrameListener selection run normally; no Landscape cache writes are used.
Each cell resets warmup; the whole batch retains a cooperative120s deadline and
its native memory peak. Existing geometry budgets/context aborts remain active.
Any exception or the final cell uses the same worker-before-camera cleanup once.

Native inactive-patch eviction is age/history dependent (over500 Landscape ticks
in eligible coarsened nodes), not a byte reservation. Fast traversal may retain
prior cells. The existing1GiB observation limit aborts excess rather than forcing
cache eviction. Compare all16 batch GBuffers with independent accepted fresh-cell
captures before accepting the batch's output/refinement.35 new production cases
verify the fixed cursor, no seventeenth cell, total deadline and clock wrap;
the overrun mutant fails. Full310 checks/115 imports and independent review pass.
Live batch proof: PID3036/start03:02:52.0513899Z, DLL5c1f0ba6, source083ee5d.
Three complete batches took9858/7199/7481ms,48 cells total. Every raw GBuffer
pixel array matched its independently accepted fresh-Terrain reference. Each
batch created/joined/closed one worker and removed one private scene; resource
counts returned to scene0/workspaces1/names3(cached programs). Material10766652,
texture1957652437 and highProgram925825 remained balanced. First native terrain
peak400946656B. Process private bytes after idle were approximately7.768/7.781/
7.801GB, versus7.726GB before the first batch. This substantially reduces the
earlier per-cell recreation growth but does NOT prove zero retention or leaks.

### Native exposure preview (private, default off)
NativeExposurePreview=1 reads the current main Resolve_HDR/rt_adaptedLum1x1
log-luminance and HDRSettings.EXPOSURE_KEY once, before private Terrain creation.
Exact exported aggregate-return adapters and shape/format/finite guards refuse
unknown layouts. Local TexturePtr/buffer references are released immediately;
only CPU values persist. Lighting and its paired reference-camera/ambient UV
values are frozen for the whole16-cell batch, never read anew per tile.

Every cell first renders/saves the unchanged linear HDR diagnostic, explicitly
bypassing exposure/gamma. An optional second render overwrites the SAME private
Sun target using installed hdrutils.ToneMap and constants.hdr_to_gamma/hdr_disable.
It saves a separately named native_exposure BMP and reports clipping/nonfinite
pixels. No fourth program/target, adaptation chain, shared HDR writes or per-tile
average. Bloom, shadows, fog, water and complete buildings remain absent; this is
not the full native compositor. Installed gamma is false and the filmic curve
is disabled in native ToneMap; no invented gamma/filmic correction is applied.
351 checks include32 actual HLSL exposure/raw cases and9 production validation
boundaries. Removing the native exposure floor fails10 GPU cases. Live preview
acceptance is recorded separately after installation.

Live exposure proof: source340e7ca/DLLb0938b87, PID9868/start03:26:27.8414634Z.
Two16-cell batches completed9854/7460ms. Both read exactly one key/luminance
snapshot (.55/~.9959) and kept identical sun settings across all16 cells.
All32 raw and exposed HDR readbacks were finite; all32 raw GBuffer pixel arrays
matched the accepted fresh-cell references. Material10767052/texture1957652437
returned unchanged; each batch joined/closed its one worker. Clipped highlights
are measured, not silently tone-curved. Evidence isolated-native-exposure-9868-*
and isolated-native-exposure-9868-proof.json; diagnostic contact sheet saved.

### Bounded resolution experiment
DiagnosticSide is startup-latched to512(default),1024 or2048 BEFORE any observer
or compositor definition. Unknown values, including4096/8192, leave the probe
inert. There is no in-session resize of definitions/program ownership. At2048,
the named private color targets account for192MiB and peak CPU readback/staging
vectors96MiB, excluding native geometry, depth/driver/main-scene allocations.
Native Terrain LOD uses actual viewport height: higher output also increases
refinement at the unchanged(8,8,0) detail threshold. Existing1GiB/16Mtriangle
observation limits remain, and may refuse a finer capture. Do not lift those
limits automatically or market2048 as final8K.361 checks include10 raster-bound
cases; actual2048 runtime/resource proof follows separately.

First2048 trial refused a native transient estimate: visible56 with unbuilt41
returned uint64(0)-2, not a real enormous geometry count. Native patch readiness
publishes vertices before its index count. The private warmup now waits only on
that exact underflow during unfinished builds, preserving all original caps and
adding conservative visible*8572<=16M for fixed patch64. Three valid samples are
still required; no output from invalid telemetry.370 checks/116imports and
independent review pass. Recovery was observed live in PID28960/source58a749f:
two2048 captures at16000m matched raw GBuffer and Sun exactly; exposed BMPs
differ by<=1level from live adaptation. The repeat contained two real pending
underflow samples followed by valid stable output. All4194304 position pixels
were finite. Nativepeak~752MB. A16-cell2048 batch saved five then refused the
unchanged1GiB geometry budget; the next standalone capture succeeded after its
cleanup. Altitude20000 also completed. Nine saved cells across five requests,
all five workers joined/closed, five scenes removed and named resources balanced.
Whole-process retention remains open (~8.84GB idle after this run); no zero-leak
or FPS acceptance claim. Private probe was disabled and game closed after proof.
Details/evidence: docs/verification/2026-11-01-satellite-isolated-live.md.

### Recycle native geometry, retain render surfaces (candidate)
RecycleTerrainBetweenCells=1 is explicit/default0 and applies only to the fixed
16-cell batch. One Scene/camera/GBuffer/Position/Sun/resource set stays alive;
after each saved cell, Update (outside compositor iteration) clears ONLY its
private geometry queue25, joins/destroys the old native Terrain, then moves only
the private camera and creates a fresh native Terrain/stream. The old camera
does not move while deferred, preventing next-cell loading in the old cache.
Each cell resets warmup; total120s and existing native budgets remain unchanged.

Native RenderQueue.clear walks ALL scenes and is forbidden here. Exact-profile
RenderQueueGroup.clear(false) is used, with private/main queue identity refusal.
Failed native cleanup retains the one bounded resource set and disables future
requests; full/unsafe abort clears the scheduling latch to prevent an error loop.
425 checks include16 successive actual Win32 thread/helper lifecycles,7 new
production deferred-state cases and an empty-Abort mutant that fails. Independent
review clear after correcting the abort latch. Runtime coverage/memory acceptance
still require new512 then2048 evidence; this is not a final8K mode.
