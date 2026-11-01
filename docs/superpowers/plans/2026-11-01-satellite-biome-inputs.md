# Detailed satellite terrain inputs

The successful 8K control still samples DistantTerrain. Preserve the independent
Terrain/RTV lifecycle and use a private material callback with an immutable
snapshot of active biome inputs. Do not call global ZoneMap material callbacks,
register private materials with overlays, activate zones or edit shared programs.

Approaches considered: reuse the live ZoneMap callback (reject: global overlay
and material bookkeeping); bake only the distant texture (reject: visibly loses
detail); private inputs and private material construction (chosen). A read-only
snapshot observes actual modded GameData and the native biome selection tree.
It avoids reproducing mod load order and avoids parsing undocumented blendinfo
serialization into gameplay state. Textures are resolved through verified active
resource locations, with a bounded per-capture cache rather than loading the
entire world's texture set.

1. Freeze native selection semantics from resolved KenshiLib thunks and loaded
Steam code, not SDK reference RVAs. Validate a pure, allocation-free selector:
32x32 coarse cells, 32x32 Morton leaves, 0.0001 max-bound epsilon, shared ancestor,
five source slots descending, four-material native limit and preserved channels.
Cover world/coarse boundaries and malformed snapshots. Keep real snapshot fixtures.
2. Snapshot at a safe main-thread boundary with exact code/layout/identity guards;
copy values and masks, never retain raw GameData/tree pointers across worlds.
Read existing maps with const find; native getMaterialValues/getBiomeTextureArrayData
use inserting accessors and are not unconditionally read-only. Capture source
identity and refuse changed/missing inputs instead of silently inventing materials.
3. Independently construct bounded private biome arrays/materials from the installed
native Terrain shader and copied parameters. Preserve native biome channels,
overlay/colour coordinates, slope units, normal mapping and missing-layer rules.
Disable orbital distance fade only on owned material parameters. Verify full
texture resolution and compiled bindings before replacing distant colours.
4. Profile native Terrain's material callback setter and callback ownership/thread
contract. Bind only the private Terrain; default material remains a clear fallback
for native coarse patches spanning multiple selection cells. Bound material/array
cache and byte budgets; destroy private Terrain before callback-owned resources.
5. Compare a small reference capture, repeat cleanup and biome boundaries before
large output. Then advance geometry/seams, buildings/LODs, water/shadows and native
day/night, and integrate the optional global-map viewer. BestMinimap unchanged.

Evidence already read, without native calls or input/focus:
research/global-satellite-20261031/biome-native-20261101. Active75-code table has
74 records and one absent mapping. The1024 selection cells use57 distinct records;
656 cells have fine mask trees. Metadata reads485068B and selection reads1446421B.
Actual native selector RVA A08E50..A0928C; reference wrappers are resolved through
loaded KenshiLib. All data is diagnostic, not an accepted detailed render.
