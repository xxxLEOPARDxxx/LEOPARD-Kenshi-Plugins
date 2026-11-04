# Large detail blocks and persistent frame-time cost

User direction confirmed 2026-11-04: actual larger render surfaces, aiming at
1024x1024 at unchanged3m spacing; retain accepted terrain detail, dynamic light,
water and city layers. Grouping existing256 views is not this deliverable.
No further design approval is required under the user's standing workflow rule.

Current evidence: fine producer completes, RAM/GPU retain prepared terrain, but
fine lighting/model composition continues. Coarse coverage suppression works in
the live log (coarseHidden1, coarseShades fixed) yet user still reports low FPS
and once-per-second stutters. Therefore do not repeat the same coarse fix.

1. Add bounded debug-only CPU stage timing around lighting/water/local/model
   snapshots, fine source tick, model preparation, lighting submission and GUI
   presentation. One aggregate report per5s; no blocking waits or GPU flush.
   Distinguish CPU submission time from GPU duration. Measure a stationary map.
2. Fix the measured periodic CPU work: cache immutable validated source/profile
   results, prepare copied values/files on workers, retain Main-only world/GPU
   ownership. Verify source invalidation and save/world/device changes.
3. Reuse static geometry/material composition separately from dynamic lighting.
   Charge retained surfaces to actual RAM/VRAM budgets; preserve original terrain
   for layer toggles. Never reuse the one shared scratch cell across owners.
4. Introduce actual1024 parent raster jobs. Existing512/256 bounds and independent
   camera radial validation must be extended coherently. Validate the full parent
   first, then create sealed aligned child crop receipts for existing256 cache
   keys. Never validate parent depth under a child's different camera centre.
   Bound parent CPU/GPU scratch, counts, cancellation, worker borrows and fences.
5. Incrementally draw/prepare large surfaces within a measured frame budget;
   do not replace many small stalls with one large uninterruptible draw. Preserve
   centre-first coverage and warm-cache reuse. Compare cold latency, stationary
   and panning frame times, memory and pixel/height agreement in the game.

One private development stream; no public version/release or personal changelog
entry. Keep the existing atlas untouched. Review substantial lifecycle changes
independently and commit/push concrete tested increments.
