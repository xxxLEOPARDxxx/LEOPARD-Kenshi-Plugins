# Bounded fine-detail disk store

Private C++03/VC10 component. No game calls, D3D, threads, UI or atlas
publication. `Session` is synchronous and belongs exclusively to one caller-owned
file worker. Its `Begin` refuses the native Main thread recorded in `Scope`;
every subsequent operation checks the owning worker thread. Caller must join
that worker before mutating its accepted producer/window. Before destruction,
worker `Close` must have run and `NeedsRetention()` must be false.

## Contract

- Pure `Configure(Config&, allowedParent, newDirectory)` copies bounded paths.
  The immediate child directory must not exist. Original world-build directory
  guards hold every nonreparse ancestor; new files and reads/deletions use
  HANDLE-relative operations with `FILE_OPEN_REPARSE_POINT`. No path redirection,
  existing-directory adoption, cross-process cache reuse or overwrite is allowed.
- `Identity` carries full64bit source/global-material generations plus full32byte
  SHA256 pins for the **verified complete base**, complete source catalog, and
  pixel-affecting material catalog. Caller verifies those artifacts; providing
  nonzero bytes alone does not prove they are the game's current sources.
- `Begin(enabled, nativeScope, identity, config, ledger)` creates the owned cache
  on the worker. `Put(enabled, key, identity, actualAccepted, currentScope,
  exactFrame, authenticatedHeightWindow, cancel, ledger, receipt)` accepts only
  real production `WorldPixels::Accepted` data, at exact world-origin anchored
  **256x256 at1m or3m**. It checks the full captured scope, frame and source
  identities through production `AcceptedWindow::Adapter`. Frame's regional
  material generation deliberately differs from the global material stamp.
- Material RGB/gloss FP16 bits and authoritative world-heightR32 are preserved.
  The normal RGB stays exact; alpha0 is converted to full accepted coverage255.
  No synthetic height, image upscale or9m/GST2 key reinterpretation is present.
- A sealed current-session `Receipt` is returned only after the complete header
  and all three planes were written, flushed, checked for exact EOF/physical
  file identity, and **known closed successfully**. `Check(receipt)` validates
  the current session's private index, fullkey/identity/serial/hash. It is not an
  independent cross-process base-publication receipt.
- `Get(enabled, key, identity, cancel, ledger, Blob&)` first clears the output,
  then opens the independently indexed physical file. Exact header bytes, full
  file hash, per-plane hashes and EOF must match the retained private index.
  Data is published only after successful close. Blob owns one1MiB CPU payload;
  `Acquire(enabled,currentKey,currentIdentity,View&)` exposes const plane views.
  The coordinator must also check current world/load/plan before GPU publication.
- I/O failure after file mutation, corruption, identity mismatch of an existing
  record or close failure poisons the Session. Further I/O is refused; `Close`
  preserves the latched failure. No best-effort error swallowing or partial
  record publication. Forensic files remain for explicit scoped cleanup.
- If a file close fails, its actual HANDLE remains owned by Session, including
  Get and delete-pending eviction handles. A failed directory close also retains
  its HANDLE. `NeedsRetention()` tells the outer worker/controller to quarantine
  the whole Session, all parent guards and its CPU budget rather than destruct
  it. `Close` retries pending handles; only later **known successful close**
  clears retention. A closed-but-error outcome is deliberately conservative:
  an invalid-handle retry cannot prove that ownership was released, so the owner
  remains quarantined. The destructor cannot implement this outer ownership
  policy; calling it while retention is required violates the caller contract.

## Closed-session payload cleanup

`DiscardKnownClosed()` is a terminal **owning-worker-only** operation. The fixed
private index is its bounded ownership inventory: it checks at most512 recorded
physical file IDs, exact sizes and nonreparse directory/file guards before
deleting anything. It then rechecks each indexed file through the existing
HANDLE-relative Remove path. It never enumerates a directory, follows a filename
pattern, recursively deletes a folder or touches a source/unindexed file.
An old receipt no longer passes Check after successful deletion.

The operation requires an active healthy Session with no uncertain close.
Foreign/replaced files, failed preflight/close or failed deletion poison the
Session and preserve remaining files for inspection. A failed live handle close
still requires retaining the complete owner. Close alone does not discard data;
the FineWorker invokes this operation only during normal/gracefully cancelled
terminal shutdown, after every producer borrow and Main-held read is released.
The empty session directory is left in place; historical/forensic directories
are not swept. No file I/O is added to Main or a normal rendering frame.

## Disk and CPU limits

There are512 fixed slot names, each exactly1,049,600B (1024B header plus
1,048,576B payload), hence **537,395,200B maximum**. The worker removes a
known-closed LRU record **before** creating its replacement; it never creates
a513th file. An evicted receipt's serial no longer passes `Check`. If an
unexpected file replaces a slot, its physical identity prevents deleting it.

All operations are serialized; there is no internal request queue. A production
coordinator must retain **exactly one pending read Blob**, clear/join that work
before replacing it, and include every unrelated retained Blob in `ledger.other`.
This component cannot discover allocations owned by unrelated caller objects.

The single64MiB CPU budget charges `sizeof(Session)` (including512 index records,
all hashes, header copies and path guards), one normal-coverage adapter, borrowed
Accepted/window owners, one1KiB scratch header, one Blob/payload, and explicit
`Ledger.retainedRaw`/`Ledger.other`. Automatic Accepted/window charges must not be
hidden from the ledger or double counted by the coordinator. Opaque Windows file
system/crypto allocations are not algorithm payload estimates. No GPU allocation
is performed here.

## On-disk private format

All files `fine_000.gsf` through `fine_127.gsf` are immutable while indexed.
Header1024 starts `GSFINE01`, version1, header/file sizes, slot, current-session
nonce and write serial, exact spacing/x/z, full64bit global identities, three
SHA32 pins, canonical regional Frame fields and canonical authenticated height
source identity. Formats10/28/41 and side256 are explicit. Unused bytes are zero;
compiler struct padding is never serialized. The payload is consecutive material
RGBA16F524288B, normal/coverageRGBA8262144B, worldY R32F262144B.
The trusted index separately keeps exact header/full-file/perplane hashes and
physical file ID. A self-consistent disk header is never sufficient admission.

## Verification and limits

Run bundled Python `run_tests.py`. VC10/Od and/O2 tests execute the actual header,
canonical TIFF source authentication, production Accepted radial/triangle checks,
origin-anchored1/3m keys, exact HDR/height/normal disk roundtrips,129 writes/LRU,
full64bit staleness, changed material hash, inclusive64MiB/+1 budgets, actual
file corruption, and Windows syscall faults (zero/short writes, read EOF,
flush failure and live-handle close failure in Put/Get/eviction/directory
teardown, followed by known-close recovery). Every fixture is inventoried and gets
a cleanup manifest before deletion. Generated binaries/data stay ignored.
Focused cleanup controls additionally cover exact known-record removal, stale
receipt invalidation, all-record preflight before mutation, a foreign last file,
live failed preflight close retention and deletion refusal without a sweep.

Raw fixture color and normal values are deliberately synthetic finite/HDR data;
their radial values are calculated against the actual authenticated canonical
height source and then accepted through the unmodified production validator.
These are disk/acceptance tests, **not native game rendering, GPU relighting,
coordinator cancellation/join proof, FPS or long-running memory measurements**.
