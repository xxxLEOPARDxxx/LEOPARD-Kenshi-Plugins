# Satellite global map: unattended native-terrain acceptance

Scope: private tooling, not a public plugin release. DF development remains paused.
The requested final map is4x4 primary cells, each8192x8192. The evidence below is
512x512 per primary cell, genuine native Terrain, not a completed8K atlas.

## Reproducible whole-world terrain

Canonical source: `tools/diagnostics/satellite-capture-probe`.
Installed Ogre SHA256: `7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6`.
Terrain SHA256: `53afd1e4faf35020556d33f57007d51c2a9d2330b630e39a67161ee850b7a239`.
Native world width294912m; each primary cell73728m. A fresh private SceneManager,
Terrain, TIFF stream and camera render genuine installed DistantTerrain material.
No shared ZoneMap callback, forced NPC zone activation or actor movement is used.

Root loaded `SAT-render-proof-20261101` normally through CUA, without import/reset,
and kept the game paused with the map open. Named DF/SAT source slots were not
manually overwritten. Native autosaving continued normally.

PID30032/startUTC2026-11-01T01:55:26.3274481Z, source8d4ebf2, installedDLL
`502fac6be5d4c4ca4e5a92ca01f20116ead0a2e762528958ab81203951923c18`:

-60 successful primary-cell captures and one clean abort when native autosave
  became busy. All61 requests logged worker kernel join, corrected HANDLE close
  and Terrain destruction before its borrowed camera/SceneManager.
-Two complete4x4 primary-grid passes at altitude16000m. All16 corresponding raw
  GBuffer BMP pairs were byte-identical. This is repeatable coarse surface data;
  it does not establish final geometric refinement or building coverage.
-262144 valid reconstructed positions per512 capture, no NaN/Inf; radial residual
  about0.004m for this much larger framing. This is an algebra/readback check,
  not a universal world-space accuracy guarantee.
-The former native +1thread HANDLE per fresh Terrain no longer occurred. Process
  totals fluctuated with game/plugins/save activity; those totals are not an
  exact census of only the diagnostic's resources.

Evidence under the installed game's `_codex_work/global-satellite-20261101`:
`isolated-native-worker-fixed-30032` (final log,61/61 cleanup counts,10repeat
metrics, read-only VAD inventories), `isolated-grid-30032` (first16 cells and
labelled diagnostic plot), `isolated-grid-repeat-30032` (second16 and byte
comparison), `isolated-grid-fourth-30032` (later16-cell memory pass).

## Memory is not accepted yet

After a later16-cell pass, read-only VirtualQueryEx reported +217567232 committed
private bytes; mapped/image totals were unchanged. Some burst allocation returned
during idle, while another retained trend remained. No source-level proportional
collection was found; native ownership paths for scene pools, Terrain buffers,
workspace textures and readback staging were independently checked. Their release
paths do not prove that CRT/driver committed memory returns immediately.

PID35284/startUTC2026-11-01T02:24:23.6256228Z, source44084ac, installedDLL
`825bed9f121fd343f2421c392bb57e9d99ef4722aeca8126555cf103c419a121`:

-Warmup +two16-cell GBuffer-only passes, then one16-cell full Position/Sun pass.
-After every Drop: private scene absent, nine named private resources absent,
  one active game workspace. Texture bytes1957652437 and high-program bytes914017
  returned to constant values; low-program reported bytes0.
-Idle private memory grew roughly75MB,70MB,218MB respectively. The full path
  contributes more retention, but this ordered test is not a randomized causal
  proof. Compiler heaps, persistent native depth/input-layout caches and general
  allocator retention remain candidates; no global cleanup is attempted.

The current diagnostic's explicit GPU colour targets cost48bytes/pixel:12MiB
at512,3GiB at8192, excluding native depth/staging/driver buffers. Overlapping CPU
depth, floating Sun data and conversion/readback would add significant memory.
Changing a single Side constant is therefore not an acceptable final8K design.
The final path needs bounded sampling/staging and reusable render resources.

Evidence: `isolated-AB-35284`, `isolated-AB-35284-warmup`,
`isolated-AB-35284-geometry1`, `isolated-AB-35284-geometry2`,
`isolated-AB-35284-full1`.

## Native material accounting defect: fixed in private copies

Resource telemetry exposed a distinct deterministic error: MaterialManager bytes
decreased13198 per GBuffer-only request and18995 per full request. Native
Material::clone156800 invokes assignment156420, copying Resource loading stateD0
and sizeE0 without a manager load credit. Later unload debits the copied size.
copyDetailsTo1569C0 invokes the same assignment and is not a safe replacement.
Unchecked native subtraction can eventually wrap unsigned accounting.

Both private copies now create cleanup-owned fresh materials, then use exact
Technique assignment3729E0 to copy configuration only. Parent identity is checked
before/after; receive/transparency-shadow flags use exact accessors. Neither
Resource metadata nor the manager counter is manually patched. No engine-wide
fix is applied to unrelated game materials.

PID28204/startUTC2026-11-01T02:38:21.0755222Z, source3950a8e, installedDLL
`c1b2af60d316c251bbd58b549480ab45d2889770dd4dcd08834f2e416f44de75`:

-17 full captures succeeded, including every primary cell.
-All17 before/after MaterialManager values were exactly10767332.
-All17 cleanups joined/closed their worker and restored scene0/names0/workspaces1.
-All16 raw GBuffer images matched the earlier pre-fix grid byte-for-byte.
-Actual private-byte retention still remained. Correct accounting is not a claim
  that all memory/FPS issues are fixed.

Evidence: `isolated-material-fixed-28204`, `isolated-material-fixed-28204-warmup`,
`isolated-material-fixed-28204-grid` (accounting proof and image comparison).

## One-lifetime batch follow-up

PID3036/startUTC2026-11-01T03:02:52.0513899Z, source083ee5d, installed DLL
5c1f0ba6e09fbaa44575a5eb725f296bbe18d03be0d00664d2b88257bd37776b.
Three full16-cell batches completed in9858/7199/7481ms. All48 raw pixel arrays
matched the independent fresh-cell references exactly (comparison JSON in each
isolated-grid-batch-3036-first/second/third evidence folder). One worker/Scene
lifetime per whole batch; three kernel joins/handle closes and cleanups. Native
terrain first-batch peak400946656B. After each cleanup: scene0/privateNames3,
workspace1, materials10766652, textures1957652437, highPrograms925825.

Baseline process private7726104576B; successive idle observations approximately
7768481792/7780630528/7801016320B. Whole-process/allocator growth is still present,
and other plugins/autosaves were not disabled. No attribution or leak-free claim.
Batching avoids the observed ~200MB per16 from repeated full scene recreation;
bounded shader reuse alone did not remove that growth.

## Outstanding

## Native estimate underflow at2048 (first trial refused)

PID28788/start03:39:27.6007385Z, c78a1e7/DLL9fbae335, actual2048primary1,1,
token105. Native step10: memory132005248,patches841,visible56,ogre649,unbuilt41,
triangles18446744073709551614. Existing guard stopped capture without output,
joined/closed the worker, destroyed the private scene and balanced materials.
Evidence isolated-2048-28788-first/failure.log.txt. This is not an8K acceptance.

Independent exact-native audit confirms getDebugInfo14BEB..14C00 estimates
visibleCount*FIRSTvisibleGeometry.indexCount-2, not an exact sum of drawn
triangles. Worker47A0 publishes Patch.vertexD8 at5A16 before indexbuilder5AA0
writes Patch.indexCountD0 at5C68 (8572 for patch64). Native readiness35F0 tests
vertexD8 only. Selection can therefore expose a geometry with indexCount0,
producing exactly UINT64_MAX-1 during unfinished construction. Aggregate48B ABI
remains correct. No game function/counter is patched.

Private readiness now waits ONLY for this exact value when visible>0 and
unbuilt>0. Memory/patch/object budgets always apply; visible*8572 has its own
conservative16M bound, persistent invalid telemetry expires, all other excess
values still refuse. Readiness resets and requires three subsequent ordinary
stable samples.9 new real production-policy cases cover this observed fixture
and refusal boundaries; old code fails the fixture. Full370 checks/116imports,
independent review clear. Actual recovery must still be observed after install.

## Remaining work

Further allocator/cache isolation and refinement/seams need proof before heavier
rendering. Then prove native exposure, unloaded/player/modded building
proxies, water/shadows/fog, interactive global-map integration and dynamic day/night
refresh. The512 diagnostics must not be advertised as the requested finished map.
BestMinimap is unchanged. No satellite/DF public package or publication was made.
