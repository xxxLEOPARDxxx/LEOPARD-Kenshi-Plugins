# Native biome selection inputs, read-only preparation

This is preparation for private detailed terrain materials. The renderer still
uses distant colours; this report does not claim a detailed capture or final map.

Process8124/startUTC2026-11-01T10:12:33.5156518Z remained paused with the map open,
in the background. ReadProcessMemory only; no native function calls, zone
activation, window/input operations or game state writes.

KenshiLib's loaded export thunks resolve to actual Steam functions:

| Function | Actual RVA |
|---|---:|
| ZoneMap.getMaterialValues | A0CAF0 |
| ZoneMap.getBiomeTextureArrayData | A0C420 |
| ZoneMap.getTerrainMaterial_DX11 | A13DC0 |
| ZoneMap.materialCallback | A16400 |
| ZoneManager.getBiomeCode | 6D7DB0 |
| ZoneManager.getBiomeMap | 9D5840 |
| BiomeMap selector, resolved from callback's direct thunk | A08E50..A0928C |

SDK reference RVAs differ; none were called speculatively. Native material
accessors use inserting map access, so merely labeling those getters read-only
would be wrong. The global material callback additionally accesses ZoneMaps,
overlays and shared bookkeeping. Do not call it for the private Terrain.

Bounded snapshots in private research/global-satellite-20261031/biome-native-20261101:

- active-biomes.json:75 colour codes,74 actual BIOMES records and one null mapping;
  merged live floats/integers/files.485068bytes read. No FCS load-order emulation.
- biome-selection-snapshot.json:32x32 coarse cells,32x32 fine leaves per cell;
  656 fine trees,57 distinct biome records,1446421bytes read. Header/table identity
  rechecked after copying, with map count/cycle and finite-property guards.
- Both maps cover origin(-147456,-147456), span(294912,294912). The native1024²
  biome image and saved source dimensions agree. Active records reference254
  nonempty terrain texture paths;253 exist directly under the game root. This
  is not final resource resolution: active resource groups and missing-layer
  behavior still require proof. Do not silently replace the missing path.

The pure BiomeSelection.h matches native coarse boundaries, Morton ancestry,
five source slots descending, null-slot channel retention and explicit native
first-four material limit. It allocates nothing. All1024 fine leaf coordinates
and real mask bytes from cell16,2 are tested. Full source tree SHA256:
e6d444c3f0385b274c8a89e90f33593f26d70b4325810e3627e418f741e03a0f.
Tests use only the selected observed nodes, not an invented full source tree.

Independent review caught an extra restriction on projected sub-epsilon boxes:
valid100..100.25m bounds select leaf1024 in native code despite maxAdjusted<min.
Removed that restriction; signed floor comparisons also handle a thin box at
the lower world boundary.29 production selector checks pass, alongside the full
probe build/ABI/GPU suite. This is disassembly/fixture equivalence, not a claim
that the private selector has already been compared by executing native queries.

Next implement independently owned texture arrays/material parameters and
profile private Terrain callbacks. Preserve actual resource resolution, painted
overlays and blend channels; disable distance fade only on owned material state.
Do not mutate shared shaders or use a placeholder material as final imagery.
