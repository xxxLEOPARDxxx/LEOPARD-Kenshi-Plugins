# Private fixed256 control adapter — 2026-11-03

This opt-in diagnostic uses the existing map observer and workspace listener.
There is no new hook, native Terrain, scene, camera, viewport, AtlasRun operation,
global Side change, active GST/GSI/ready write, or viewer integration. It captures
the frozen base-fixture geometry/material around(-13824,-13824) on the current
game D3D device. It is not active mod/resource-winner or final-map proof. Native
lighting, water, models and lit previews are deliberately absent from this pass.

## Concrete test-session configuration

Installation and personal INI changes belong to the parent after independent
review and after the game closes. Preserve/restore the user's prior INIs.

Before starting that game process, set the sibling installed viewer INI:

```
mods/GlobalSatelliteMap/GlobalSatelliteMap.ini
[GlobalSatelliteMap]
Enabled=0
```

In the private probe INI beside its DLL:

```
[Probe]
FixedTerrainControl=1
FixedTerrainGsmStartupDisabled=1
FixedTerrainRequest=0
FixedTerrainCancel=0
DiagnosticSide=512
ResolutionOnlyControl=0
```

Defaults remain OFF/zero in the source template. The checkpoint and viewer OFF
state are latched at plugin startup. Changing them after startup does not arm an
initially refused process. Each admission/active step rechecks viewer Enabled=0
and refuses every texture whose group is LEOPARD_GlobalSatelliteMap_Development
or whose name begins GlobalSatelliteMap_. A bounded32768-resource census refuses
on overflow. It evicts nothing and never silently changes the192MiB viewer cap.
The189MiB viewer reservation cannot coexist with this90MiB producer control.

The fixed mode uses only FixedTerrainRequest, consumes the startup token without
replay, and consumes new tokens even when busy/ineligible. After the map is open
and the probe idle, changing that one nonzero token requests one capture set.
FixedTerrainCancel=1 cancels an active set and also refuses new requests. Clear
it and issue a different token after the prior job joins. Fixed mode bypasses
the legacy keyboard/Request/AtlasRun path. It does not focus or control the UI.

## Prepared inputs

Run bundled Python `prepare_fixed_terrain.py`. It writes only the probe build's
`fixed-terrain-inputs/` staging folder, not the game installation. The parent can
copy this folder beside the reviewed DLL for the test session. All16 exact
inputs total75,534,626 bytes: twelve original pinned DDS blobs, three already
decoded/hash-pinned RGBA PNG bases and one raw133-square LE16 height fixture.
`source-pins.json` is an audit inventory, not a trusted runtime override.

The worker independently checks exact file sizes and SHA256 before handing one
typed immutable blob to the render thread. File handles permit read sharing
only. Files cannot be replaced/written while that read/hash is open. The staged
source owner rechecks DDS/RGBA pins on upload. No PNG decode, source path choice,
whole TIFF load, native material lookup or engine object access runs on the worker.
The typed height fixture is specifically global samples7358..7490 from the
source-pinned TIFF. Compiled material contract SHA256 is
afe79b4152ba35cc364e4f7c92a59771a428d4dec2d63217d442bfba2993f0b9.

This is an honest base-fixture limit: the prepared DDS/PNG/TIFF bytes and frozen
two-SID material defaults are exact, but do not establish current native source
winner, mod overrides, material/weather values or the currently visible world.

## Scheduling, resources and lifetime

The one owned worker has a maximum16MiB input slot, released before the next
input or output. Render-thread upload consumes at most one blob per callback.
After all uploads, the same slot hands four bounded readbacks to that worker;
it never holds all75MB of source blobs. Own CPU payload plus223KiB mesh-builder
stack and a786576-byte output remains below64MiB. Driver/internal process memory
is not measured by that count.

Producer GPU payload is **86,156,268 bytes**: source83,885,956, raster1,746,024,
two reused format-correct staging planes524,288. Both staged-source and raster
admission include those additional owners, below the strict90MiB control limit
and192MiB cap. No per-view resource creation occurs. Native scene/device memory,
driver overhead and unrelated native game textures are outside this producer
ledger; no total-process peak or absence of all leaks is claimed.

The device is acquired from an existing main GBuffer SRV through the already
pinned D3D texture vtable/getter profile, then held by COM. Each callback repeats
the current-device identity check. World, renderer/workspace owner, profile,
main-thread and save-busy checks precede scene/filesystem/resource inspection.
The map observer cancels when map/world/context changes. No live native camera
or floating-origin position is borrowed. The raster's top-down camera is a value.

Uploads, raster draws and readbacks preserve foreign pipeline state, including
predication. The raster scopes all modified graphics state/read aliases; the
readback scopes predicate only and honors RowPitch. All D3D access stays on the
serialized existing main/render callback. OFF adds no worker/resource/native
queries beyond the single option branch and startup INI read.

Cancellation signals the CPU worker and clears GPU owners on the render thread.
CPU state is deleted only after the exact owned thread signals completion. Poll
is nonblocking. WAIT_FAILED or any unexpected wait result emits exactly one
quarantine event: cancel/clear GPU, retain this single CPU job and unknown handles,
detach the listener using existing ownership checks, and refuse all further jobs
until restart. It never closes/deletes an unknown worker or repeats the error.
The bounded quarantine can retain up to one16MiB CPU slot until process exit.
Dynamic DLL unload while a worker is active is unsupported; normal request/world
cancellation joins via Poll. This diagnostic adds no unload hook.

The active-only texture census and existing hazard queries are intentionally
duplicated bounded checks for this short control. They are not a design for
permanent per-frame visible-window rendering.

## Private artifacts

Every set uses a fresh `fixed-terrain-PID-token-tick/` directory beside the probe
DLL. Directory creation refuses an existing name. The worker writes each artifact
to an exclusive `.partial` file, flushes it and renames without replacement.
It never overwrites an existing output or any active atlas file. On cancellation
or I/O failure, partial/incomplete artifacts may remain; they have no completion
marker and are never published as a map.

`view-0.bin` through `view-3.bin` each contain a144-byte GFTC0001/version1 header
and three256-square four-byte planes. Header includes PID, request token/epoch,
view index, absolute X/Z, spacing, cameraY/far, DXGI formats28/28/41, plane bytes,
height SHA256 and material-contract SHA256. Views are1m,3m,9m and3m shifted192m
on both axes. CameraY10000/far20000; source geometry remains18m/native40B/morph0.
Planes are packed native material RGBA8, native normal RGBA8 and native radialR32.
They require the existing packed-neighbour decoding contract, not direct RGB use.
`complete.txt` appears only after all four files have been flushed and published.

## Verification and limits

`test_fixed_control.py`: real bounded worker/file handshake, WARP upload/draw/
readback/write, byte-for-byte comparison of all planes with four independent
manual compact controls, all256 guard combinations, complete foreign context
snapshots each step under a false predicate, cancellation, bad source and
exclusive output refusal. Poll-loop assertion count varies with scheduling.

`test_fixed_control_calls.py`: actual extracted native request/context/callback
bodies;35 checks plus five compiled controls. Covers startup/no replay, busy and
hidden-map token consumption, cancel-before-I/O, save/world/profile/viewer
refusals, early world short-circuit, changed device and nested callbacks.

`test_fixed_control_mutants.py`: ten compiled worker/control negative controls,
two actual staging-allocation failure/cleanup paths and an injected WAIT_FAILED
quarantine. Compile errors/crashes are not counted as successful mutant rejection.
`build.ps1` performs the unchanged full native build, PE/import audit and existing
legacy probe suites. No test invokes a game process or installs files.

Actual game callback/appearance, long-run memory/FPS, save/load behavior and
current source-winner semantics remain separate acceptance after reviewed test
installation. These controls are not the full regenerated atlas or final detailed map.
