# Four-view fine terrain source batching

Two consecutive same-cell `FineJob::Pair` records may share one existing native
four-view cohort. Pair/Plan arrays stay unchanged:80/57504 bytes on the current
x64 build. Coordinator Owner gains no fields. Native cohort already owns four
raster slots and processes a single readback/accepted/borrowed output at a time;
the cache remains256px with its existing payload, validation and identity gates.

Public interfaces:

- `FineJob::CanBatchNext(plan, pair)` checks bounds, same native region/cell,
  unique output/companion keys and exact stamps.
- `FineJob::MakeJob(..., WorldCohort::Job& out, unsigned pairSpan=1)` accepts only
  one or two pairs, creating two or four native views.
- `FineJob::CanRender(..., unsigned views=2)` admits only actual native2/4 counts
  and charges their real GPU reservation.

Main integration remains a small change inside `FineQueue`: select span2 only
for required visible work outside center mode, when CanBatchNext succeeds and
actual current budget passes CanRender with4. Otherwise use default span1.
Pass span as the last MakeJob argument. Center and prefetch remain2 views.

```cpp
unsigned pairSpan=1;FineJob::Budget chunkBudget;
if(required&&!fineCenter&&FineJob::CanBatchNext(plan,pair)&&
   fineOwner->MakeBudget(FineResidency(),FineCpuLedger(),chunkBudget)&&
   FineJob::CanRender(plan,fineDemand,chunkBudget,4))pairSpan=2;
// ... FineJob::MakeJob(...,nativeJob,pairSpan)
```

Coordinator independently regenerates the exact four-view job and checks every
frame/grid before Queue, then checks real4 headroom before native rebind and
again during queued/post-write ticks. Output n maps existing pair+n/2,key[n%2].
Only wanted outputs are written. Every consumer acknowledgment precedes source
ReleaseView, next output or viewport drain. All four views finish their serial
writes before the one cohort acknowledgment/drain. Main can keep its existing
two-key pair read/progress loop: the next pair is already in cache.

Per job source preparation is shared twice as widely; ideal full groups roughly
halve bank allocations, capture/open/hash/PNG decode/DDS upload/drain work per
wanted tile. Raster initialization, terrain pixel work and cache output totals
remain. GPU reservation is187062500B for4 versus137571380B for2. Never use the
two-view allowance for a four-view producer. No CPU-cap increase, new native bank,
larger persistent payload or cache-format change is involved. Driver/raster Init
and single upload/hash calls remain nonpreemptible and can still cause spikes.

Run focused controls from repository root:

```powershell
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' GlobalSatelliteMap/atlas-terrain/four-view-tests/run_tests.py
```

The runner builds real FineJob/native job validation plus extracted production
coordinator PairMatches, Queue and write/queued/Complete branches. Routing/native
I/O boundaries are controlled substitutes, not a native gameplay test. It checks
four output key routing, first/second pair padding, consumer barriers, viewport
drain, real4 budget and changed-residency BUSY retry, exact identity, default2
compatibility, overflow and duplicate companion refusal. No game assets, D3D
device, DLL build or huge fixtures are used. Original code was observed RED at
`two same-cell pairs share one four-view source job` before implementation.
`--legacy-pair` selects the old two-view MakeJob call and reproduces that expected
RED without editing production sources.

Generated snippets, executable and logs remain in ignored `_generated/`.
Game cold latency/FPS improvement requires separate native measurement.
