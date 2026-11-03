# Regional material and idle source-worker retirement

Reviewed by the parent implementer independently of the component author on
2026-11-03. This review covers the small native material adapter and the added
idle-only asynchronous shutdown API, not the unfinished production viewer.

## Material adapter

Reviewed `RegionMaterial.h` (`ccda3cbc02d6f85eb767363f1c5e16c3fbd71b35a557cc40d1f46a716447cde3`)
and `NativeAdapter.h` (`f8dae34b651853d9a9f97d846370b2bcfd4932eae0f9df16e8f19baf1aadd8e5`).
The extraction preserves the proven 900/60 float material packing. Capture
checks current Main scope, the profiled executable, ordered cell record slots,
tree bytes and canonical authored paths, then checks scope again before
publishing a copied result. Refusal clears the output. File contents are not
read here. The bounded, temporary metadata work belongs at region admission,
not in every visible frame. The caller remains responsible for same-pointer
load epochs and comparing both full material hashes with the validated atlas.

Fresh execution of the existing native-independent host controls passed under
both /Od and /O2: 200,880 checks per executable. These include all 64 regions,
off/on/off fade packing, the real captured region-2 parameter bytes and stale,
foreign-thread, malformed path, tree and nonfinite input refusal. The real SDK
adapter is compile/link evidence; these host runs do not execute native engine
record access. No additional large fixture or mirrored tree was created for
this review. No actionable issue found within this scope.

## Asynchronous idle stop

Reviewed the added `RequestShutdown`, `PollShutdown`, `Quarantined` and appended
`Stopping` state in `WorldCohort.h`
(`672f855dc845c5af25fa92dca861ba28bcd2b73aa4ae285c8ac389d792ab63ae`).
Admission requires Main, Idle, a live WIdle worker and zero retained/query GPU
ownership. Stop publication is nonblocking. Polling uses zero-time waits;
only WStopped plus known join releases the worker slot. Unknown wait status,
missing cleanup marker or the wrap-safe deadline retains the complete owner.
The worker terminal HRESULT is copied before deletion. Existing state IDs are
preserved; the existing final-teardown blocking API is unchanged.

Fresh `async-shutdown/run_tests.py` passed /Od and /O2. The normal path exercised
the actual WIC worker and repeated slot reuse (96 and 95 checks respectively;
pending-poll check count varies with scheduling). Each optimization also passed
8 initial-wait, 9 polling-wait, 10 deadline-wrap and 10 missing-marker checks.
Injected pending GPU/query and foreign calls refuse; this is not a D3D render
or driver-lifetime proof. No actionable issue found in the added idle API.

The production viewer still needs its own integration, resource-budget and
gameplay review. Neither component test proves final image quality, FPS or
absence of all long-running leaks.
