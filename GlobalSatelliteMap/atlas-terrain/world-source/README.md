# Bounded pinned world-height source

This is a worker-only source/cache primitive for fixed global terrain. It does
not render, create engine/D3D objects, launch threads, touch INIs, write atlases,
resolve active native material winners or change the existing geometry helper.
The caller schedules one owning worker and honors cancellation/join/borrowed
pointer lifetime. Errors are returned as Win32 codes; the outer plugin logs them.

## Exact source and authentication

Supported `fullmap.tif` is536,980,180 bytes, SHA256
`6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18`.
The parser admits its actual classic TIFF profile: II/42, IFD at8 with21 unique
entries and no next IFD;16385-square, one unsigned16-bit channel, orientation1,
uncompressed black-is-zero, one strip at43686 with536,936,450 pixel bytes.
The entire file, including metadata/footer, is hashed after format admission.
Unknown content/profile is refused, never substituted or silently reinterpreted.

`Source::Open(path,epoch,operation)` requires a nonzero64-bit caller epoch. It
uses the existing `AtlasView::Hash` and bounded `ReadAt` implementation, then
retains the SAME authenticated read-only file handle. FILE_SHARE_READ denies
ordinary write/delete opens and replacements throughout its lifetime. No reopen
by path occurs for later windows. Identity records epoch, volume/file index,
last-write FILETIME and full content SHA256. This is exact filesystem source
provenance for this profile, not proof of which height source an arbitrary live
native/mod resolver selected.

Opening reads266 profile bytes then536,980,180 hash bytes, with64KiB stack
scratch. It never allocates a whole TIFF or full-world height/vertex array.
Authenticate once per source/worker epoch, not per visible window or frame.
The handle and identity are published only after full authentication and the
final cancellation check. Failure closes the unpublished handle.

## Interface and ownership

```cpp
WorldHeight::Source source;
WorldHeight::Operation op;
// On the owning worker; caller chooses a stable nonzero source/world generation.
if (!source.Open(path, epoch, op)) { /* report op.Error(), accept no window */ }
WorldHeight::Window window;
WorldHeight::Region vertices = {7104,7360,7168,7424}; // inclusive global vertices
if (source.Read(vertices, epoch, window, op)) {
    FixedTerrain::Samples samples = window.View();
    // samples is borrowed until the next Read/Clear/destruction of window.
    // FixedTerrain::Build(samples,7104,7360,callerVertices,4225) is unchanged.
}
```

`Region` is an inclusive global source-vertex rectangle in0..16384, at most1025
vertices per axis. A caller requesting geometry must cover every needed aligned
patch; the supplier does not guess a camera or patch layout. It adds TWO samples
on every side for the native normal/coarse-normal stencil, clipping only at the
WORLD edge. Internal capture/chunk boundaries never clamp. It reads precisely
the required segment of each row; no hidden full16385-sample rows are copied.

The largest request is1024 source intervals/axis,16x16 native64-interval patches:
one2048-square9m base block. Across the294912m world, the16x16 such source blocks
cover the future4x4 logical8192-pixel tiles. Smaller1–3m visible windows use the
same source vertices/halos and fixed geometry. This does not invent finer heights.

One `Window` owns at most1029x1029 uint16 samples: **2,117,682 bytes**. Growing
releases the previous allocation before creating the larger one, preventing
vector growth from exceeding that bound. Reuse may retain that bounded capacity.
An exact rectangle+source identity+epoch cache hit performs zero file reads and
does not reallocate. There is no global cache, LRU, queue or unbounded entry set.
The caller must include every separate Window in its64MiB CPU owner ledger.

Any failed/cancelled Read clears the supplied window's data/stamp/publication;
it never returns a partially replaced or stale accepted view. A completed copied
window can outlive the Source handle, but the consumer must compare its stamp
against the current caller epoch/source before use. Closing a Source prevents
it from validating even a previously cached window. Reopening at a new epoch
forces a fresh authenticated read; an older window cannot bypass it as a hit.

Source is noncopyable and Read is confined to the thread that authenticated it.
This prevents shared seek-position races; it is not a general concurrent reader.
Window is noncopyable. Only Operation::Cancel is cross-thread safe. Error,
ReadBytes, Reset, observer setup, Close and object/window destruction require
the operation to have finished or its worker joined. Reset explicitly clears
cancellation and observer state; an operation start never silently clears cancel.
Observe is a bounded after-chunk progress/cancellation callback on the worker;
it must not reenter Source/Window or access engine objects. The caller retains
its callback/context lifetime through completion. Cancellation is checked before
open/cache hits, each up-to64KiB auth chunk, each partial row and final publication.

No whole-world allocation is needed. One maximum window plus auth scratch is
below2.2MiB own payload; geometry Build's existing53,868-byte scratch/169000-byte
caller packet and any upload/readback owners remain additional caller charges.
No192MiB GPU/1GiB capture cap is changed by this CPU-only component.

## Verification

Run bundled Python `run_tests.py`; `--skip-mutants` omits only compiled controls.
VC2010x64/SDK7.1 and the actual pinned local TIFF are required. Test files stay
in `_generated`. No process/native DLL is loaded, and no installed file is written.

The compiled missing implementation first failed `actual source authenticated`.
The real implementation passes3,478 checks in EACH /O2 and /Od build:

- All256 world blocks have exact expected halo extents, independent real-file
  sample checks and **541,863,200 total partial-row bytes /263,360 row reads**.
  Extra bytes beyond the native pixel strip are the intentional shared halos.
- The old atlas boundary at global sample7168 has exact raw40B shared-edge and
  four-way corner packets from separately read windows. Every packet of the
  pinned reference patch matches the independent frozen native-order fixture.
- Four world corners preserve the geometry helper's outside-world zero rules.
- Cache hits perform zero I/O; epoch changes, stale epochs, closed sources,
  another thread, invalid/oversized/reversed/negative bounds, allocation failure
  and cancellation refuse without a published/stale window.
- Profile mutations reject before hashing. An owned format-correct sparse
  impostor of the exact logical size fails full-file SHA256. Short files refuse
  before reads. Tested refusal/cancellation paths balance file/crypto handles.

Instrumented C++ allocation evidence: largest allocation2,117,682 bytes, total
test peak2,624,682 bytes. The test allocator rejects any request above12MiB and
checks the live peak below8MiB. This covers these C++ owners, not all Windows
crypto/file-cache/process memory. Fourteen separately compiled runtime controls
cover hash/format, halo/row offsets, epoch/cache, partial cleanup, world edges,
inclusive cap, cancelled cache hits, worker ownership, vector growth and hidden
whole-image allocation. Compiler failures/crashes are not counted as kills.

The two negative TIFFs are sparse owned test files, not downloaded/copied full
images; their large logical length does not allocate a whole TIFF in memory.
No runtime FPS, all-leaks, live source-winner or final map/image acceptance follows
from these checks. Renderer/SID material/cache integration is a separate step.
