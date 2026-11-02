# Existing static city models: first integrated development layer

The active viewer now composes already registered native DistantTown models into
a pristine copy of each terrain window before existing sun/ambient/HDR, local
lights and water. CityModels is optional, default OFF, translated in EN/RU/ZH;
Detailed log remains last. This is not full building coverage or a release.

Production Main SHA25676167438e82a426d91a0864dd1ba33709fed78029d3c4b64d293ce5ef83f8692.
Installed DLL780288bytes, SHA2566ef2dc5ebd180e26eb309b27d1ab867e97f96109edd5bc1b93e18d810761ccee.
Installation backup: game/_codex_work/global-satellite-20261102/before-city-models.
Only local QA CityModels=1 was added; all previous settings were preserved.

The catalog contains420town records;29already registered static proxies were
accepted in PID33644. First live stable log at205.701..215.858s reports:
modelReady1/modelCount29/modelRenders1122, borrowedBuffers784908bytes,
borrowedTextures349552bytes, ownedGpuBytes196086608 under201326592byte cap.
Sourcelevel1/density1,50/50visible details,34apron,144resident windows,247reads.
Model/local render counts and window reads stopped changing while paused.
This proves active admission/raster/decode calls and bounded paused invalidation,
not final visual quality, FPS parity, all-world coverage or absence of all leaks.

The pool has64fixed slots, coherent catalog reads at1Hz and at most2native
source admissions per200ms map tick. No source/model loading or resource
creation occurs in per-window rasterization. Native descriptors retain COM data,
not dereferenceable Town/Entity/mesh/material pointers. Static descriptor reuse
does not detect arbitrary in-place GPU writes. Unique retained buffers have a
32MiB steady cap; textures64MiB, separately counted from owned rendering payload.
An incoming snapshot can temporarily retain another32MiB of already existing
native buffers before global admission/refusal. These are not absolute VRAM/RSS
bounds. Native diffuse was independently read as512x512 BC3/10mips/349552bytes.

Each composition replays current models into one fixed pristine working surface;
removal/OFF does not bake permanent model silhouettes into the terrain cache.
World/scene changes retire packets. Origin proof changes invalidate windows even
if packet poses have already refreshed. Failed shared local/model origin proof
retires models as well. These two ordering regressions were found and fixed in
independent review before installation.

Fresh checks: full run_viewer_tests.ps1 and run_local_composition_tests.ps1 PASS;
model caller30checks/5compiledmutants, model file-worker28/4, water caller563/1,
water worker45/4, local composition63/12, local revision108, mapping25.
Independent cache137real-COM checks/15mutants, resolver14/2, origin caller91/3.
Region65CPU plus1376278actual region/native-raster/decoder WARP checks passed
for canonical source windows; maximum measured height error0.0185546875m.
Twenty UI messages verified in three compiled catalogs. Complete VC10 /GL/LTCG
build and actual PE79imports/startPlugin/MCM exports pass.

Independent reports are retained in D:/DEV/Kenshi/research:
gsm-model-main-review-20261102/report.md (0d9108d8...),
gsm-model-cache-review-20261102/report.md (91609676...),
gsm-model-region-review-20261102/report.md (7cef2570...).

Pending: explicit visible building comparison, real CPU/FPS/long-memory and
repeated OFF/ON/world-load observations. Fully loaded buildings, missing proxy
towns, private LOD import, player construction, lighting in unloaded areas and
shadows remain separate work. The isolated loaded-windmill/private-LOD helpers
are not active viewer features.
