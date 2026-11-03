# Independent accepted-window adapter review

Verdict: **CLEAR; no P1/P2 finding.** No source correction requested.

Author manifest SHA256:
`28da66770cf159d3f8f55445f8f0dac6ab41d298daddbff31dcdb5f94ef0512d`.
AcceptedWindow.h SHA256:
`71dd16bba54fd1e69a5fbf7872bac14edfbf5dec2932b4f28ae2d109a4bd4899`.
The frozen own/dependency/fixture/log pins and35 mirrored files remained
unchanged. Reviewer writes/execution stayed in this research directory. No
production, game/INI/install/UI or publication change was made.

## Contract and lifetime

Build first withdraws old output and obtains an actual fresh Accepted.Acquire.
It owns only one normalRGBA8 buffer, at most1MiB. Normal RGB stays byte-identical
and the native reserved alpha0 becomes coverage255 only after whole-window
acceptance. FP16 full RGB/gloss and CPU world height are borrowed unchanged;
material gloss is never interpreted as coverage, and radial is never used as
the height plane.

Acquire independently checks the supplied owner address, complete scope/frame,
fresh Accepted acquisition, height identity, budget and cancellation. It then
compares every source normal RGB/reserved alpha against the prepared copy. It
does not publish cached material/height addresses. Same-label rebuilds with
changed normals refuse, including when the allocator reuses exactly the same
address. Matching normals permit fresh accepted material/height because that
copy then has identical normal content for the new snapshot. Labels remain
the caller's authority rather than cryptographic proof of arbitrary image origin.

Every refused getter zeroes the View and clears owned data; failed/cancelled
Build likewise frees partial output. No input is owned or retained as a plane
pointer. Accepted, Adapter and acquisition context must remain alive, unchanged
and serialized through all downstream consumption. Old Views are invalid after
owner rebuild/Clear/destruction. This is a worker-only usage contract, not an
engine Main-thread detector or an asynchronous ownership facility.

## Fresh execution

Bundled Python ran the unchanged full suite from the isolated mirror. VC10/O2
and/Od each passed23,452 controls across14 actual accepted256/512 windows and
1,900,544 pixels. All material/height bytes and converted normal pixels were
checked. All five512 cases fed the unchanged WindowWriter512 through an exact
offset memory sink with flags0 and existing formats; all rows/planes matched.
The256 cases refused without resizing or writer IO. One written window did not
publish a coarse readiness bit. Existing semantic_box retained signed HDR/gloss,
coverage and height semantics; native normal alpha0 would erase that coverage.

Twenty actual-event cancellation boundaries, two allocation faults, eleven
functional mutants and two cancellation mutants passed intended assertions.
Compilation failures/crashes were not counted as kills.

An additional48 independent controls each/O2 and/Od used the real adapter and
actual Accepted/source/fixtures, instrumenting only the borrowed Accepted
allocator/release into two explicitly charged test arenas:

* Forced same-slab replacement changed finite material/gloss while keeping
  normals and labels: fresh acquisition returned current unchanged bits.
* Forced relocation changed the accepted slab address with matching normals:
  acquisition returned the fresh material/height addresses, not cached pointers.
* Forced exact same slab address with changed normal RGB refused and released
  the old normal copy, proving content checks rather than allocator luck.
* Destroying/reconstructing a valid Accepted at the same object address was
  freshly revalidated after old Views were discarded; no dangling owner was read.
* Getter admission succeeded at the inclusive64MiB ledger boundary, refused one
  extra raw byte and maximum-U64 raw charge, and withdrew output on refusal.
* Corrupting the adapter-owned coverage byte prevented it reaching a sink.

## Cost and limits

The adapter performs one bounded row/pixel scan in Build and Acquire and one
allocation per successful Build. Old/new normal copies never coexist. Admission
automatically charges Accepted, the authenticated height window/object and
Adapter; retained raw capacity and all other owners remain explicit caller
charges. Subtraction-based accounting avoids overflow. All test arenas and
additional source vectors were disclosed in the independent test ledger.
There is no file/native/D3D call, thread creation, queue or growing cache; only
nonblocking cancellation polling. GPU accounting is unchanged.

The existing Writer512 sink proves format/value compatibility only. Placement,
source/material/capture stamps, caller lifetimes, successful IO, complete coarse
windows/mips/index readiness and atomic atlas publication remain separate. This
review establishes no actual native winner, global continuity/seam, current
world cohort, game FPS/RSS or universal leak claim. It supports the parent's
next separately reviewed cohort integration.
