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
