# Loaded-town integration candidate, 2026-11-04

Terrain and fixed zoom were accepted by the user today. This change leaves the
atlas intact and starts the next requested city-geometry stage.

Installed private DLL: 1,226,752 bytes, SHA256
`5cae6c2287c69f023e77950784210f0103d64af11c92b496532862521f2ce0c1`.
VC10 x64 /GL /LTCG build and startup/MCM/86 native-import checks pass. Personal
INI remains `63d3ca9d5db3013724288fd70eacad35367df25e4e449a6012ff675ead6156f8`.
Previous DLL and three catalogs are in `D:/DEV/Kenshi/backup/gsm-loaded-town-20261104`.

## Scope

The existing model option adds one nearby loaded-town instance packet to the
existing distant/cached fallback. Supported families are the two plaster variants,
Wall4, Gate4 and market alpha materials. Current active resource resolution and
21 pinned files are qualified before capture; unsupported/ambiguous resources
keep LOD. No zone activation, native model creation or gameplay writes.

The native capture is shortlisted to four town candidates on the existing1Hz
catalog update; no native lookup is done per raster window. Only copied matrices,
bounded metadata and retained COM buffers/textures survive capture. The new
Packet::SameContents accessor compares actual contents and sampler values, so
unchanged captures do not invalidate map windows. Static shader/raster behaviour
is unchanged by that accessor; historical helper freezes describe their original
baseline rather than silently acquiring a new hash.

Raw RGB/gloss and native normal/translucency are lit before separate accepted
coverage is emitted. Current sun/ambient resources remain owned by GsmNativeLight;
origin/world/source checks are maintained. A post-HDR accepted-cell callback
supplies model height/normal to both local illumination and water. Owned city
raster/compositor surfaces total2,884,080 bytes, included in live/fine/retired
accounting. Lazy creation checks AdmitGpu, including uncertain retirement latches.
Fine CPU accounting reserves3MiB for the capture and overlapping packet lifetime.

Model/instance/cached jobs carry an option epoch. OFF/reset advances it; old
completions cannot poison a newly enabled layer or mutate its verified state.

## Evidence

- Native build log: `D:/DEV/Kenshi/build/GlobalSatelliteMap/loaded-town-build.log`.
- TownCompose native SunProbe/actual raster WARP:46 controls per/Od,/O2 and7
  functional mutants. Independent compositor review CLEAR. The immutable
  compositor production hashes remain in its `town-verification.json`.
- Loaded packet refresh:9 WARP controls per/Od,/O2; actual SDK adapter compiles.
- Actual Poll prefix/reset epoch with real closed Win32 handles:35 controls
  per/Od,/O2, including OFF/ON, stale model/instance/cached results, world/cancel,
  wrap and exactly-once disposal.
- Existing local/water caller regression:64 checks and11 compiled negative
  controls. This CPU caller fixture intentionally leaves detailed towns OFF;
  it does not substitute for a real town draw.
- Actual water pipeline:262329 pixel/state controls pass, native HDR error0,
  UNORM difference<=1; a newly accepted raised building remains dry while old
  terrain is submerged, and an invalid surface does not overwrite the display.
- Independent integration review CLEAR after the stale-result and failed-
  retirement admission findings were corrected.
- All three installed MO catalogs contain the updated city-option tooltip.

## Remaining acceptance

Actual town rendering, source-family coverage, CPU/FPS and long-session memory
are not established by these offline checks. User was asked to load the Hub test
scene after installation; no gameplay result is claimed yet. Unsupported material
families, ordinary non-instanced objects/lampposts, full player construction and
shadows remain unfinished. This is not a public release or an entry in the
personal published-update journal.
