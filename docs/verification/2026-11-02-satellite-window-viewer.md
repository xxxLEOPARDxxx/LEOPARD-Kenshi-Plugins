# Satellite viewer: retain zoom cache and read native details

The user confirmed the first whole-map display, then reported rebuilding on
each zoom click and detail below the ordinary8K map. Production inspection found
two causes: Draw destroyed every visible cell on a level change, and the selected
source level started at2048 rather than8192.

## Implemented and installed

- Read256-square windows from existing GST2 source levels0..5. Native texel
  density is9×2^level metres, and absolute window bounds feed the existing
  native-light relighter. The16 original8192 capture files are unchanged.
- Keep16 pinned overview windows and48 detail LRU slots. Reuse resident windows
  across zoom/pan. Admit completed reads by full tile/level/window key and current
  view plan, never by an obsolete reserved slot.
- Keep ready images while refinement loads. On zoom-out, hide finer retained
  windows only after the aligned desired ancestor is ready. MyGUI depth is based
  on source level, so finer images draw in front of coarse fallback.
- Resize resident coordinates immediately on a native map-size change. The
  unchanged-size path does no projections/setters. Streaming remains200ms and
  native-light snapshots1Hz, with at most four256-square relights per tick.
- GPU payload remains under the existing96MiB cap:64×256²×20 plus256²×4 scratch
  and176-byte constants =84,148,400bytes. One worker stages1MiB of window planes;
  the verified index body is2,096,640bytes. No new world scan or source hash work
  occurs on zoom. These figures exclude driver/native wrapper/OS cache overhead.

DLL493568bytes, SHA256
`c2bf714d46eca180d9f42d178db1de9b3419169a482aab4b03a8a4c531df6f3e`.
The actual PE exports startup/MCM correctly and resolves all77 native imports.
Installed after checking Kenshi was closed, with exact personal INI bytes kept.
All three MO catalogs were copied/verified; no new player-facing text was added.

## Existing atlas/index integration

The offline builder read22,900,899,840 plane bytes once in116.89s, verified all288
original whole-plane hashes, and created one2,097,664-byte GSI1 index. It did not
render terrain, rewrite a GST, create image tiles or duplicate the large atlas.

Index SHA256:
`3e935e8a26de0fba2e1e1f934fbee20256c69adb4f55530dedccf5f2d393b357`.
The index pins capture metadata and all16 GST headers; ready.json pins the index.
Only that hash property was added to ready.json. Original metadata and all16
GST sizes/timestamps were checked unchanged. Publication is atomic; stale or
different control/index files are refused. Full reader/builder details and source
pins are in GlobalSatelliteMap/atlas-view/window-reader-verification.md.

Root separately loaded this real index through the production C++ loader and
read eight real windows across source levels0,1,2,4,5, including first/last cells.
All checksum/shape checks passed; every request returned1,049,600 logical bytes
including its header. Measured worker reads were0..343ms on this compressed atlas.
Those timings include768 strided row reads; they are not GPU/FPS measurements.
First visits refine progressively and can take several seconds. Contiguous
stripe IO is a possible later optimization, not included in this fix.

## Verification

Fresh root runs passed:

- Window policy3412 checks; restoring the old2048-source minimum fails the
  native-resolution regression as expected.
- Extracted production bounds/projection/supersession662137 checks; adoption,
  cancellation and generation26 checks; resize event/fast path10 checks.
- Six independent compiled integration mutants rejected for the intended reason.
- Window/index C++ reader92 checks, including all source levels, real junction
  rejection, write-sharing denial, mid-read cancellation and corruption.
- Eight builder suites and eight reader/builder functional mutants rejected.
- Existing manifest13, worker55, projection17, clocks13, native sampler128 and
  extracted managed-image10 checks; source/MCM/three-locale contracts.
- Existing resource policy29, reader53, native-light WARP92 checks and six
  state/hash/coverage/HDR mutants.

Independent source/lifecycle/performance review is CLEAR after fixing stale fine
overlays on zoom-out. Evidence:
`D:/DEV/Kenshi/research/gsm-window-integration-review-20261102/report.md`.
The reviewer scripts were also retained as versioned tests, and root reran them.

Local real-data and installation evidence:
`D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/global-satellite-20261102/window-streaming/`
and `before-window-streaming/installation.json`.

## Acceptance limits

The user has been asked to reopen the same paused map, allow refinement and zoom
in/out/back. New live display quality, bilinear window-edge appearance, frame
times and long-session memory remain pending. Mathematical projection and short
offline tests do not prove a seamless image or absence of every possible leak.
No-gutter window filtering may need a follow-up if visible seams are observed.

This remains the private terrain viewer. Water, models/buildings, local lights
and shadows are unfinished layers. No public release or version bump was made.
