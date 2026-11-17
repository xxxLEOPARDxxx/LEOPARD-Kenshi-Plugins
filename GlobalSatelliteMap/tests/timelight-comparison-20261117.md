# TimeLight comparison and proposed cold-detail path

Research date: 2026-11-17. No map runtime changes or new map measurements.
The owner requested comparison/design while a separate full KEO save-load
profile is prepared. Existing map multithreading must be preserved.

## Reproducible upstream references

- TimeLight Map 1.3.0: `pizzy00/timelight-map`, commit
  `96eccdc1b66e519c5a195a082d72efd1dab2d74f`.
- Its documented build target: MeitouClient v0.20.0, commit
  `da6af12e612f5d3d5368f351146152393e6283b6`.
- Read-only research checkouts reside under `D:/DEV/Kenshi/tools/research`.

TimeLight `TopCam.cs:492-631` creates colour/depth targets and a second
orthographic scene pass. It reuses the engine's TerrainRenderer, Objects,
Foliage and Characters rendering. `Marks.cs:515-557` supplies site/outpost
labels separately. `README.txt:248-272` describes the loaded-surroundings
limitation and cost of the additional scene pass. It is not a globally baked
high-detail tile pyramid. Targets remain on GPU through presentation.

This is not an entirely single-threaded renderer. TopCam calls
`BeginHostPassParallel`; Meitou `Gpu/Recording.cs:248-274` records secondary
Vulkan command buffers using RenderJobs where draw/job thresholds permit.
Shared state preparation remains on the render thread. Those facilities do
not exist automatically in a native Ogre/D3D11 plugin.

Meitou already supplies world coarse heights, streamed fine height windows,
patch LOD/quadtree culling, texture arrays and streamed object instances:
`TerrainRenderer.cs:109-111,179-206,788-807`,
`TerrainStreamer.cs:85-130,292-320`, `TerrainTextures.cs:16-23`, and
`WorldObjectRenderer.cs:1840-1872`. Copying the small camera mod does not copy
these engine systems into Kenshi.

## Current local cold path

There is no special hardcoded expensive 2x branch. FineJob::MakePlan changes
spacing to 3 m at <=9 m/pixel and to 1 m below 3 m/pixel if the window budget
allows. These are different cache keys and create new cold work.

Verified source dependencies:

1. `world-cohort/WorldCohort.h:127-129`: GPU raster, staging copy/readback,
   then worker validation.
2. `fine-coordinator/FineCoordinator.h:118`: SubmitPut borrows the producer.
3. `fine-worker/FineWorker.h:34-43`: Session::Put completes before RAM
   RememberAccepted and PutDone acknowledgement.
4. `fine-store/FineStore.h:114-115`: write/digest and FlushFileBuffers for
   each fresh window.
5. Producer GPU drainage precedes the read/upload phase; SubmitGet then
   obtains the accepted payload again.
6. `GsmFineViewer.h:84` / `GsmFineGpu.h:39`: accepted planes upload into new
   immutable GPU textures, followed by shading/presentation.

Thus first display depends on durable persistence even when sufficient RAM
is available. This dependency is established by source inspection; its share
of elapsed cold-fill time has NOT been measured.

Other potential costs: per-window raster object/shader/resource creation,
main-thread fixed geometry construction, serial source inputs, repeated DDS
hash validation. Existing source/cache workers, batched neighboring pairs,
RAM retention, GPU LRU, fine-pump limits and cheap FinePresent already exist;
they must not be presented as new proposals.

## Recommended first implementation

Keep the tile architecture and both persistent workers. Separate readiness
for display from durability in the disk cache:

1. Preserve the existing producer and mandatory validation.
2. Build an independently owned immutable AcceptedCrop on a worker.
3. Publish the validated crop to a bounded display-ready queue immediately.
4. Upload/shade on the render owner without awaiting disk Put/flush/Get.
5. Persist the same owned payload in the background; retain explicit
   acknowledgements for display consumption and persistence separately.

`atlas-cache/accepted-crop/AcceptedCrop.h` already provides a worker-built
owned crop with no surviving source/window borrow and canonical normal
alpha. Initial queue budget can be two 256x256 crops (~1 MiB each), charged
against the existing aggregate CPU/GPU ledgers. Exact admission must include
all live producers and consumers rather than bypassing the current
producerReservedGpu guard.

Visible centre tiles take priority over persistence and speculative work;
the coarse atlas stays visible until accepted fine data is ready. World,
source, material and generation identity must accompany every result.
Pan/zoom may discard stale presentation while caching still-current data;
world/load/OFF requires cancellation and bounded drainage. Worker data must
be immutable and owned, with fence-based retirement for GPU dependencies.

Next, reuse raster objects and group missing windows by source cell where
the measured cold path justifies it. Move only pure geometry preparation to
workers, with owned immutable packets and owner-thread GPU submission.

## Deferred alternatives and validation

A raw GPU-target handoff is not a pointer swap: the ValidatePixels worker
command calls WorldPixels::Accepted::Build to check material/normal/radial
values and construct canonical world height using WorldPixels::Triangle.
The producer's radial plane cannot be passed
to a height consumer. A GPU-resident path needs equivalent canonicalization,
validation, fences and lifetime/budget accounting as a separate change.

A second Ogre camera can be useful for a local live view, but it limits
detail to loaded surroundings and adds another scene pass. It does not
replace this globally available map's source streamer.

Before claiming improvement, future implementation must compare cold time
to first accepted tile/full coverage, source/decode/raster/readback/
validation/persistence/upload/shade timing, warm RAM hits, cancellation,
bounded memory and save/load shutdown. No numerical speedup or FPS claim
is supported by this research alone.
