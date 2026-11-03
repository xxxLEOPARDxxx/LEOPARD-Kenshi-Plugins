# Bounded fine visible windows

`FineWindowPlanner.h` is a pure C++03 planner. It has no engine, D3D, file,
worker, cache or allocator dependency. It proposes at most128 visible/apron jobs;
it does not allocate windows, discard the coarse map, publish textures, alter Main
or imply in-game readiness. Existing atlas and terrain helpers are unchanged.

## Global grid and screen density

The actual world is294912m square, with origin(-147456,-147456). Each key is
256-square pixels, anchored to the world origin, independent of camera position.

| Output spacing | World-window span | Global window grid | Windows per73728m logical tile |
| --- | ---: | ---: | ---: |
| Base9m | 2304m | 128x128 | 32x32 |
| Fine3m | 768m | 384x384 | 96x96 |
| Fine1m | 256m | 1152x1152 | 288x288 |

Bounds and pixel centers use integer-anchored world positions. Pixel center is
`origin + globalWindow*256*spacing + (pixel+0.5)*spacing`. All4x4 logical tile
boundaries are exact. Because the spacing ratios are odd, every9m center is also
a3m and1m center; no pixel phase derives from camera translation. This concerns
grid coordinates, not perfect equality of independently rasterized shader floats.
The original geography shader's camera drift remains a separate fact.

Request contains a `Rect{x,z,w,h}`, screen width/height and two nonzero64-bit
generations (height/source epoch and material generation). Dimensions must be
positive, finite and representable, and screen dimensions are1..16384. The planner
clips the half-open rectangle to the world. Invalid data produces no jobs; a
disjoint viewport produces Empty. It checks finite endpoints before conversions.
Exact integer-boundary comparisons repair quotient rounding near zero without
an arbitrary epsilon, including tiny rectangles straddling a global boundary.

Screen density uses the ORIGINAL untranslated width/height and screen dimensions,
not the clipped rectangle. The smaller metres-per-screen-pixel axis determines
detail: below3m requests1m; otherwise below9m requests3m; otherwise base9m is
sufficient. Equality uses the coarser matching spacing. If1m cannot fit the
reservation,3m may still be worthwhile; otherwise the result is Coarse with no
fine jobs. The supplied9m grid maps the existing base; this component never queues
a partial set of base jobs or regenerates the full world.

There is no assumed universal1m at native zoom8. For example, a294912/8-m wide
world viewport on1920 pixels is19.2m per screen pixel and uses the base. Smaller
actual visible rectangles can warrant1m. The input must describe the actual
projected viewport; zoom labels alone cannot establish screen/world density.

## Capacity and GPU reservations

Admission uses the projection principle from the unchanged AtlasWindowPolicy:
for each axis reserve `min(globalGrid,ceil(originalExtent/windowSpan)+1)`, then
multiply the axes. This bounds every visible pan phase. Only after the complete
worst visible phase fits are actual clipped keys enumerated. Identical extents,
screen dimensions and budget choose the same level under translation, including
partial world-edge clipping. A genuinely different source/residency reservation
may change admission; a completely disjoint world view has no detail jobs.

Budget has NO implicit free192MiB pool. Every field is explicit:

- `limit`: the smallest applicable pipeline cap, positive and no higher than
  the existing192MiB viewer cap. The unchanged RGB raster also enforces90MiB;
  callers using it must honor that smaller cap. This planner does not waive it.
- `retained`: every existing/pending GPU byte outside prospective jobs, including
  all coarse fallbacks, old fine outputs and externally held/pending owners.
- `sourcePeak`: additional authenticated/planned material-source peak, not already
  counted in retained. It must be positive and admitted for the current stamp.
- `rasterPeak`: additional actual transient raster/geometry/depth/CB peak, also
  positive. Caller accounts overlap instead of silently reusing in-flight owners.
- `windowBytes`: actual per-new-window cache/display payload, at least1048576B
  for RGBA16F RGB/gloss +RGBA8 normal +R32 radial.1310720B also includes anRGBA8
  display output;1572864B includes anRGBA16F display output. No presentation
  format is silently assumed.
- `capacity` (0..128) and `unavailable`: total job/cache slots and slots held by
  other owners/jobs. Retained must charge at least unavailable*windowBytes.
- `stamp` plus `sourceAdmitted`: caller's matching source/material reservation
  proof. Missing, zero-peak or stale source plans produce only coarse fallback.

Subtraction checks prevent overflow before combining owners. Usable slots are
the minimum of available slots and remaining GPU bytes/windowBytes. Reported
`peakBytes` includes retained +source +raster +the worst-visible reservation +
the actual optional apron. It may exceed the bytes in currently visible keys
because future pan-phase slots remain reserved. No slot eviction/reuse credit,
memory cap increase, general source inference or hidden source pool is supplied.

Source admission must cover ALL proposed windows, including an optional apron
and the appropriate worst phase, not just one currently visible SID. A caller
without that source proof disables apron or supplies a conservative bounded
source plan and rechecks after concrete source selection. The boolean alone is
not native provenance; that remains the source adapter's obligation.

The concrete test ledger20MiB retained +80MiB source +2MiB raster fits a small
fine plan under192MiB. It correctly produces NO fine job under the unchanged90MiB
raster cap. The previous189,006,000B window reservation plus sources also refuses.
Current full material pools and old fixed cache counts therefore cannot simply
be combined with this planner. A real integration must use an explicitly approved
resource plan, smaller actual source/fallback residency, or an explicit later
architecture decision; this component changes neither existing limit.

## Jobs, publication and ownership

Visible jobs come first, ordered by squared world distance to clipped viewport
center, then Z/X to make ties deterministic. A one-window apron is optional and
uses only spare capacity BEYOND the worst visible phase. Apron jobs are sorted
after all visible jobs. Every queued key is unique.

The producer must render the key's anchored256-square world footprint and
center, not a translated UI camera footprint. Material generation must change
for every pixel-affecting source/parameter/weather change; this pure planner
cannot establish that external snapshot contract.

A key is the full tuple `(spacing,globalX,globalZ,sourceEpoch,materialGeneration)`;
it has no lossy hash or32-bit generation truncation. `CanPublish(plan,requested,
completed,success)` requires an exact successful currently wanted completion.
It accepts current apron jobs for caching. `UseFine` additionally requires a
visible key; false means the caller retains/draws its coarse fallback for that
window. A stale source/material, unrelated key, failed/incomplete or old-view job
cannot replace the coarse image. Coarse remains caller-owned even after one
window completes; this helper does not release it or declare whole-view coverage.

Request, Budget and Plan are value objects. All ownership and synchronization
remain with the caller. There is no thread launch, engine callback, queue, LRU,
disk cache, source pointer retention or texture publication in this header.
Downstream completion must still carry authenticated pixel/source results, not
merely a manufactured matching key. Window/output retirement and CPU/GPU ledgers
remain external.1m material/model output does not invent finer18m height samples.

## Cost and verification

The planner uses one fixed128-job array, insertion ordering and bounded apron
enumeration. Once admitted, each visible axis product is at most128. Expanding
by one window visits at most392 candidates before world-edge clipping. At most
128 jobs are retained; no whole1152-square grid is walked. Sorting is bounded by
that constant. The header performs no heap allocation, I/O or GPU/native query.
No persistent cache or retained external reference can grow. A plan is about6KiB
on the tested VC2010x64 ABI; it is returned by value. This is not an FPS or
whole-process-memory measurement.

Run bundled Python `run_tests.py` (VC2010x64/SDK7.1 selected by the runner).
Actual production code is checked in /O2 and /Od for global/tile/pixel alignment,
independent visible coverage, pan phases, optional apron, density including zoom8,
negative/positive/tiny clipping, invalid floats/screens, exact cap and cap-minus1,
source/raster/reserved owners, full64-bit epochs, exact completion and zero C++
heap allocations. Nine compiled real-body mutations must fail actual asserts;
compiler failures/crashes never count as successful controls. Frozen dependency
hashes are checked before and after the suite. This directory alone is modified.
