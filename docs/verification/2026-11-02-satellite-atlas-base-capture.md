# Whole-world terrain capture pipeline: source stage

This is a private implementation checkpoint, not whole-map runtime acceptance.
The user requested a usable optional global map and early genuine previews.
Existing `geography-cliff-preview-20261102.png` was shown: actual terrain, no
buildings/water/local-light claim. No generated illustration stands in for it.

The base exporter now covers all256 nonoverlapping2048 passes at9m/pixel,
writing directly into exactly16 logical8192-square GST2 files. Each footprint
fits one of64 regional SID catalogs; no native terrain rescale or simulation-zone
activation. Private Terrain/Scene resources retain the established stop/join,
queue, resource-credit and RTV cleanup path. File work and world-height math use
one worker holding copied data only:48MiB inputs +32MiB material staging at peak.
No Ogre/world/UI API on that worker. Manual reentry is denied while any payload
or worker is owned. Ambiguous wait/close ownership is quarantined without retry.

GST2 preserves raw packed BGRA in8-byte material slots until offline global
neighbor/parity decoding, avoiding2048-block YCoCg seams. Final material uses
RGBA16F: measured actual decoded native albedo reached1.1196079, so UNORM8 would
clip. Normal/coverage RGBA8 and world height R32 stay separate for relighting.
The builder performs semantic mips, hashes all18 planes/tile and publishes one
atomic ready manifest only after all16 finalized files. Capture metadata is pinned
and must remain unchanged through finalization. Partial/error/cancel files remain
unreadable as a ready atlas. Total uncompressed16-file storage22,900,916,224bytes.

Fresh scoped evidence:

- Full native probe build passed after preserving the historical cleanup sentinel;
  four pre-existing signed/unsigned comparison warnings remain. Final recompile
  after close-quarantine hardening passed with159 resolved native imports and the
  actual decorated RE_Kenshi startup export.
- Actual coverage/height policy:8166 checks. Extracted real request admission:
  130 checks. Independent extracted production worker:81 checks/eight full-size
  payload cases, including byte packing, height, allocation failure, invalid
  geometry with zero plane I/O, and injected failure in each plane.
- Region suite: seven generator tests;3,282,564 C++ selection/boundary checks;
  12,157 extracted adapters;200,094 actual parameter routes;8,403,536 GPU checks.
  Fourteen metadata mutations and six GPU mutations rejected. Existing native
  VS pins/legacy fixtures, bank layouts, limits and eight SRVs remain intact.
- Cache writer101 VC10 checks; root reran21 Python suites and five negative
  controls after the metadata addition. Traced raw decode26,348,036bytes,
  semantic stripe12,225,136bytes, not complete-process RSS guarantees.
- GPU viewer helper independently reviewed:163 checks/six mutations, native
  SunProbe fixture error at most1 channel level. Whole viewer integration is
  separate work, still awaiting its own complete source/live acceptance.

Installed private probe:557056bytes,
SHA256 `cba1cde7d7381f78276efc7f3e5c304de3d24c3cc477fe863ecb9000cd9f68cc`.
Kenshi was verified closed. Backup/installation record:
game/_codex_work/global-satellite-20261102/before-atlas-capture/installation.json.
Latched diagnostic side2048, AtlasCapture1, bounded control AtlasBlocks1,
Request0, file/background opt-in. No capture starts at startup. Next new token1517.
The user was asked to prepare the paused SAT-render-proof-20261101 map scene;
no new runtime result is claimed here. Full256-pass run follows a successful
control without another DLL restart. Native seams/LOD, driver staging, FPS and
long-session retention are still measured acceptance work.

The independent source collector froze221 local inputs/44835metadata bytes in
2.53s and reverified them unchanged. Its current identity predates installing
the new viewer wrapper; regenerate before the full capture if mods.cfg changes.
Never publish this unfinished private module/cache with the37 completed public
updates. DF remains paused; KEO research is parallel, implementation follows map.

## Live control and bounded full-run refusal

PID32116 loaded the new viewer wrapper and unchanged222-input profile, metadata
SHA2569817601d14e50ed0d35a4557f5d584d7e6fad3d8862f3ffa370546df2f33e3f8.
Control1517 produced4,194,304 valid height/coverage samples and no invalids.
All four owned RTV references returned0, Terrain/Scene were destroyed and native
texture/material counters returned2286514623/14146655 exactly. This proves the
bounded control cleanup, not whole-map quality or long-session leak freedom.

Full1518 stopped after11/256 blocks, at a more complex portion of region1,1.
Mesh grew to462281472B atstep14; the next step exceeded the unchanged1GiB combined
limit. Cleanup again returned the same counters and the game remained alive.
The partial atlas has no ready manifest and must not be finalized/displayed.

Native error calculation uses viewportHeight/max(1,AABBdistance)*patchError;
it does not include the orthographic window span. Atlas2048/T8 therefore
requested substantially finer geometry than the accepted512/T8 control.
The narrow repair uses equal32/32 thresholds only in atlas capture. At the same
camera/patch this preserves512/T8 refinement decisions, while texture raster
remains2048 and9m per pixel. It deliberately coarsens the failed2048/T8 mesh;
it does not prove lossless height/normals, seam quality or memory fit everywhere.
Fixed textures/targets leave503317708B for native mesh; the1GiB cap is unchanged.

The extracted actual api.detail call failed before the repair, then passed184
checks;196 pinned native arithmetic vectors independently passed. Full probe
build and159 imports passed after updating the old exact-call source assertion.
Review: research/atlas-lod-budget-20261102/report.md. Latest candidate557056B
SHA25647b93cee1a095b786cc8561cca20b0b6fb34bc72c10d42eee981c7d2d05cd3ae.
Installed with Kenshi verified closed, together with viewer a1ea4055...f6a6;
personal viewer INI unchanged and all222 source hashes reverified. Backup:
game/_codex_work/global-satellite-20261102/before-atlas-lod32/installation.json.
Next request1519 remains pending a fresh loaded scene. Full256 runtime fit and
whole-map display remain unaccepted.
