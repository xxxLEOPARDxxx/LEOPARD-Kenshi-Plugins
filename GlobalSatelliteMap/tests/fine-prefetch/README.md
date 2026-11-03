# Bounded directional fine cache-ahead

Production policy/caller: `GsmFinePrefetch.h` and the cache-ahead/event path in `GsmFineViewer.h`. At most two same-cell native K2 pairs are considered beyond the leading viewport edge. Direction comes from the last observed world-center pan; a new world/base resets it. No prediction starts before600ms of stable demand or before all visible source pairs finish. A warm completed view can also predict after its stability delay if its measured budget fits.

Speculation uses the existing one-pending FineCoordinator, native Main capture/actual material pins, file worker Put acknowledgement and asynchronous source-bank drain. Preflight Get skips already cached pairs. Speculative reads never upload a GPU image. Optional capture/material/admission failure skips prediction and resumes final visible publication. Visible GPU tiles are never evicted for prediction. All old192MiB GPU/64MiB CPU/96GPU-window/128disk-file caps remain. Additional plan memory is charged to the external CPU ledger.

This is latency hiding for a small future strip, not CPU-parallel native rendering. Existing cold live evidence was55visible windows in28pairs, approximately40seconds; source GPU work runs on the audited engine Main workspace and file/height work already uses workers. WorldCohort supports K2/K4, not K1. Healthy mapclose currently stops/cleans the file session; this change does not add cross-open trust or persistence.

`run.py`:37 controls each VC10/Od and/O2 with actual pure policy and extracted production caller/event bodies; three functional mutants killed. Fake owner/native/UI boundaries are explicit. Covers stability, visible-first order, two-pair bound, all directions, exact full64-bit demand, actual55tile budget refusal without eviction, asynchronous preflight, no speculative publication, acknowledged drain, warm idle and optional source refusal.

`run_native.py`:26 controls each/Od and/O2 through actual WIC/WARP/WorldCohort/FineStore/FineWorker/FineCoordinator. Actual accepted adjacent planes reach the bounded file store, the next predicted pan reads a hash-validated record, stale64-bit demand is refused, viewport change drains the speculative producer and healthy cancellation joins all owners. Source metadata provider is the existing synthetic fixture, not the real engine. Generated small files are inventoried and deleted.

No game/UI control, new atlas, render restart, source ABI relaxation, FPS or long-run leak claim.
