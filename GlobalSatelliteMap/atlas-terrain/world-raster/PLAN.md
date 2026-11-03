# Reusable staged world raster, bounded first core

Only this new directory changes. Existing diagnostic90MiB helpers stay frozen.
Use the unchanged native VS and frozen ordered Partial/Tail shaders; new owners
support256 or512 targets under the shared192MiB ledger.

1. TDD public phase API and complete payload accounting. Init builds at most16
   immutable globally aligned geometry patches. No source/CPU view is retained.
2. BeginFrame copies900 typed material and60 distortion floats, full64 source,
   material and frame generations, exact viewport/coarse cell and current ready
   bank stage0 lease. All frame values are finite and single-cell/geometry bounded.
3. RenderStage requires strict0..3 order and fresh matching bank bindings on the
   same device/immediate context. Stage trailing empties reuse the last actual
   SID bank. Patch only that original row's roles; retain the original15-row ABI.
   Finish requires all four contributions and the unchanged last source lease,
   then executes the once-only native wetness/TBN/raw-RGB tail.
4. Every step rechecks the complete source/carry/raster/output/readback/current/
   pending/retired ledger. Refuse writable aliases/stream output before mutation;
   detach/restores owned read aliases, graphics/extra slots/predicate and counters.
   Any partial/failing/cancelled/source-changed phase withdraws completed output.
5. Nine actual256 controls compare all three planes byte-exact.512 controls use
   real source/geometry and a bounded two-SID monolithic oracle; actual four-SID
   source/step/finite/state smoke uses the bank when frozen. Do not allocate the
   >192MiB full four-SID monolithic oracle. Keep that proof qualification explicit.
6. Bound failure/state/lifetime tests, exact source/dependency freeze and review.
   No Main/native source-winner/current weather receipt or game/install claim.
