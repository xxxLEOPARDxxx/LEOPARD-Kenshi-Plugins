# Independent review: bounded512 GST2 writer

Final reviewed freeze **`57b1de7c564fb567a11401ba04fa8de17a642818d94fa63d7852a150660eed38`**, header SHA256 `dcf946a92da69810ffa65b118c781d74280f24b4e78a996d731cd30ae2e51d3a`, from `ours/GlobalSatelliteMap/atlas-cache/window-writer512`.

**CLEAR: no remaining finding.** The initial freeze `99e17b7f1e253f0826eb567c0de7f3527b7beaf4f67ff3a83c9f0054b8f82a4d` had one confirmed P3 status defect: a corrected successful `Begin` retained a prior invalid-parameter error. `recovered-begin.cpp` demonstrates that original defect as an intentional runtime RED. After notification, the author added only `error_=ERROR_SUCCESS` on successful Begin, its regression, a14th runtime mutant and matching documentation/counts. `fixed-diff.txt` records the entire before/after closure. The new freeze was copied to a separate `mirror-fixed`, hash-verified, and fully rerun: **1,231,296 production checks +14 runtime mutants PASS**. The expanded independent suite including the exact regression passes **543,340 checks each with /O2 and /Od**. No original production code was edited by this reviewer.

The primitive is suitable for continued integration review. It is not a publication mechanism. Its caller must create an exclusive new incomplete file, retain the file/data through worker use, serialize ordinary calls on the owning worker, join before status/destruction, validate GPU/material inputs separately, and perform the existing full mip/hash/index verification and atomic atlas publication later. `RandomFile` itself cannot prove that a supplied file is fresh. Source/material/capture stamps are enforced in this writer's memory and are not persisted as a resumable source receipt.

## Verification

`prepare.py` authenticated and copied only the eleven frozen source/dependency/manifest files to `mirror/`, preserving the relative header layout. No original generated artifact was copied, altered or removed. The original `_generated/one-tile.gst.partial` was not accessed. The original full `run_tests.py` ran in the mirror and passed **1,231,293 checks and13 runtime mutants**. Compile failures or crashes are not counted as mutant detection. The actual-file test used its own unique mirror path and removed that newly owned file.

`run_adversarial.py` compiled unchanged production code with both VC10 `/O2` and `/Od`: **543,337 independent checks each** passed. Added coverage includes:

- All514 cancellation callback observation points, both cancellation and C++ exceptions, including after the last row and before data flush.
- Both refusal and C++ exception at each of512 row writes; all Begin resize/header/header-flush failure and unwind boundaries.
- Zero source/material/capture generations; maximum unsigned coordinates/plane/pitch and maximum `size_t` length; null payload; ordinary correction after invalid input.
- Cancellation before Begin, from inside the first and last actual row writes, and during data flush. Cancellation arriving during a successful flush can allow that already valid data write to return, but remains permanently visible through `Poisoned`/`BaseComplete` and prevents later writes.
- Deterministic cross-thread `Cancel` while the owner is blocked inside its first row I/O; the worker is resumed and joined before inspecting state or destroying its resources. No second row, data flush or ledger advancement occurs.
- Independent missing-final-cancel and ignored-interlocked-cancel mutants both fail runtime assertions. The separate stale-Begin-error regression is an intentional RED against the reviewed source, not included in the passing totals.

## Code/lifecycle assessment

The512 alignment/domain checks bound all addressing before data I/O; arithmetic promotes row positions to64-bit before multiplication. Plane validation precedes indexing the three-plane completion state. Each accepted plane contains exactly512 tightly packed rows (at most2 MiB); no heap allocation, cache growth or filesystem traversal is introduced by this header. The completion bitmap is fixed and duplicate windows perform no I/O. Full64-bit source/material/capture equality prevents low32 aliases. Wrong-thread writes return before touching shared seek I/O or mutable status. Only `Cancel` is intended for concurrent access; no claim is made for concurrent status/destruction or caller violations of that lifetime contract.

Data is flushed before the in-memory window completion flag; all16 flushed windows must be present before the corresponding coarse-plane ledger write. Ledger-write/ledger-flush failures poison the writer and prevent retry or base completion. A ledger write that succeeds followed by a failed flush may leave the new ledger bytes visible on disk, but all corresponding data was already flushed and the file remains incomplete. There is no resume API or ready flag publication; the later validator must reject incomplete artifacts. Callback and `RandomFile` C++ exceptions become terminal failures. `/EHsc` does not promise recovery from access violations or arbitrary invalid caller pointers.

The writer borrows its `RandomFile` and plane data and owns no handle, worker or payload. It has no cleanup I/O in its destructor. Full-data and late cancellation tests keep `BaseComplete` false after cancellation even when all48 coarse bits were previously written. This is bounded source/lifecycle evidence, not a game memory/FPS measurement or proof against every runtime leak.

## Throughput boundary

One complete tile requires393,216 row writes +48 ledger writes +one header write = **393,265 `WriteAt` calls**, and one header flush +768 data flushes +48 ledger flushes = **817 flushes**. Across16 tiles this is6,292,240 writes and13,072 flushes. The Win32 adapter uses a seek and one or more `WriteFile` calls per row. This is bounded worker work, but more frequent I/O than the older2048 block writer; no end-to-end throughput claim is justified by these tests. Scheduling and measured storage latency remain caller integration concerns.

## Reproduction

`prepare.py`, `mirror/` and `run_adversarial.py` preserve the initial freeze and its intentional RED regression. `mirror-fixed/atlas-cache/window-writer512/run_tests.py` and `fixed-regression.cpp` preserve the final accepted freeze; `fixed-original-pins.json` and `fixed-mirror-pins.json` pin it. Both original/final immutable mirrors are retained to make the reported defect and correction independently reproducible. Review-only outputs live below this research directory; no game process, installation, INI, commit or source modification was performed.
