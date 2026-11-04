# Fine GPU-key index

`GsmFineIndex::Index` is a pure metadata owner with `std::map<Key,unsigned,Less>`
and a fixed6144-byte occupied-slot bitmap. There are no textures/blobs, native
calls, file access, GUI calls or eager GiB allocations. Keys admit only valid
fine spacings1/3 with full nonzero64-bit source/material stamps. Slots range
0..`MaxFineCacheSlots-1`; the caller independently enforces the active GPU profile.

`Find(key)` returns its physical slot or-1 in O(logN). Insert rejects duplicate
key/slot, bad key/slot and map allocation failure transactionally. Erase requires
the exact key+slot pair. A successful change increments nonzero Revision; an
empty Clear/refusal does not. Main is the sole owner; no internal locking.

`Visible(view,unsigned* out,unsigned capacity)` requires capacity>=192. It
validates the request, clips it to the world and enumerates exact overlapping
half-open grid coordinates using FineWindows::Internal::{Clip,Index,Last}.
Each of spacing1/3 is skipped if its Worst view bound exceeds96; at most192 map
lookups and192 result indices are produced, in spacing/z/x order. Stale stamps
do not match. It does not allocate. Invalid view/output returns0 without writes.
The caller caches this list against both demand revision and Index::Revision,
then only hides the previous short list when rebuilding visibility.

`MetadataBytes()` charges sizeof(Index) plus256B per node and one sentinel.
That conservative VC10 x64 default-allocator ledger is tested against measured
operator-new bytes; at6144 entries it reports1,579,304B, independent of texture
memory. Charge this ownership once in the external RAM ledger. Clear releases
map nodes while retaining only the small fixed object/map sentinel.

Run bundled Python `run.py`: real production header compiled in /Od and /O2,
6144 metadata-only records, mixed spacing/current64-bit stamps, exact world
edges, missing/invalid views, duplicate slots, OOM rollback, no allocation in
10000 Find/Visible iterations, plus four functional negative controls. The
initial stub failed the first actual insertion before implementation. These
tests do not claim Main integration, GUI performance or live FPS measurements.
