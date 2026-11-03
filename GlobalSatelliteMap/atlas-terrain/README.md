# Fixed world terrain and maximum-zoom detail

The user authorized changing the terrain method on2026-11-03, and explicitly
requested finer detail at maximum zoom. The earlier no-rewrite restriction no
longer applies. Existing16x8192 atlas files remain a fallback until a complete
replacement passes validation; no incomplete diagnostic replaces ready.json.

## Implemented components

- `fixed-geometry/`: fixed global18m source lattice, native65-square patches,
  native40-byte packets and diagonal, two-sample halo. No camera-dependent LOD,
  patch-edge collapse or live terrain queue.
- `compact-material/`: bounded two-SID real material fixture and native-derived
  shader inputs. Explicitly a frozen source fixture, not a universal live-world
  material selector.
- `raster/`: context-safe fixed terrain render with real1m/3m/9m controls.
- `raw-rgb/`: direct terrain RGB16F writer; removes checker-parity neighbour
  reconstruction from material storage without changing native material math.
- `rgb-raster/`: state-safe direct-RGB render, typed frame validation and guarded
  publication; still a fixed two-SID component.
- `world-source/`: authenticated retained-file height supplier; streams at most
  one2,117,682B window, with no complete-world allocation.
- `world-material/`: all1024 base-snapshot cell plans, original selector order,
  per-SID role remaps and explicit staged-source arithmetic/provenance limits.
- `staged-material/`: four ordered float32 accumulation passes and one common
  material tail, verified against the pinned private monolithic reference.
  This does not claim stock game DXBC identity or implement a streaming loader.
- `fine-windows/`: bounded globally anchored1m/3m/9m planning and complete-key
  admission, including source/material epochs and explicit capacity/reservations.

The private game adapter is documented in
`tools/diagnostics/satellite-capture-probe/FixedTerrainControl.md`. It tests the
new pass on the real game device, leaving the active atlas untouched. The
installed viewer still uses its earlier atlas until production integration.

## Production work

1. Verify the private adapter in the actual game and record all four captures,
   device/context recovery and bounded resource release. WARP tests alone do
   not establish live-game acceptance.
2. Read authenticated height windows from the real global source with bounded
   partial-row I/O. Share the same world coordinates and halo at all levels.
3. Generalize material selection from the pinned fixture to actual source
   winners, biome SID combinations, paint maps and native scalar semantics.
   Batch source banks; never silently fall back to the fixture biome elsewhere.
4. Use the direct-RGB fixed raster for both base atlas generation and finer
   visible windows. Keep exact world anchors and pixel phase independent of
   camera movement, output-file boundaries and asynchronous job completion.
5. Stream maximum-zoom material detail only for the visible area plus a bounded
   guard. Aim for1m samples when source material and budgets permit. The native
   height source remains18m: finer texture samples do not invent terrain shape.
   Keep the requested16x8192 base layout; do not multiply global disk storage
   by81 to make the entire world permanently1m.
6. Admit source banks, geometry, raster/readback, old/pending windows, lighting,
   water and city owners against one actual GPU ledger. The current189MiB
   viewer reservation cannot coexist with the86MiB diagnostic producer under
   the192MiB cap. Production must reduce/reuse reservations and evict only
   unneeded detail before admission, not merely add two independent budgets.
7. Publish replacement detail atomically after a complete validated window.
   Retain a valid coarser satellite window during pan/zoom/load. No native-map
   strip, torn half-window, abrupt border colour or camera-dependent seam.
8. Verify the reported seam sites, loaded city/wall/prop coverage, native
   lighting/day-night, source changes, cancellation, save/load, map reopen,
   repeated pan/zoom and actual FPS/memory before marking the map ready.

Source texture detail, loaded mesh/LOD detail and screen zoom are separate
limits. Increased zoom alone must not be presented as improved rendered detail.
No public release, full-world seam guarantee or absence of all leaks follows
from the isolated components or a single successful capture.

For visible detail integration, reserve the complete maximum source/raster and
fixed non-detail owners before sizing the reusable fine-window pool. Charge
that pool once: do not count cached reusable fine entries as retained AND as
new windows. Protect in-flight leases explicitly. Keep reservations stable over
ordinary pan/zoom so filling a cache cannot itself force repeated1m/3m switches.
The planner alone neither owns that pool nor authorizes a renderer: the old
90MiB diagnostic helpers retain their checks, and production must account all
current, pending and retired owners against the unchanged192MiB shared cap.
