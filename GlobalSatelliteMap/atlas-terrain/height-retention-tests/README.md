# Completed-pair authenticated height retention controls

The normal fine pair flow joins its file consumers, acknowledges both native
views, drains all GPU allocations, and then reads/uploads prepared fine tiles.
Previously this ordinary drain used terminal `Close`, throwing away the locked
height file and repeating its full 536,980,180-byte authentication for every pair.

The scoped optimization uses `WorldCohort::DrainCompleted()` only at successful
Idle/WIdle completion. Its `Trim` command retains only the authenticated,
deny-write/delete `WorldHeight::Source` and its path. Input leases, RGBA, accepted
output, height window and raw pixels are cleared. The source is still used only
on its original worker; the existing height operation checks epoch/path before
reuse. Hard/context cancellation and worker Stop use full Close. Hard Cancel
superseding a dispatched Trim waits for Trim, then sends Close and waits again.
No source validation, GPU ledger or zero-producer read/upload gate was removed.
No new cache, growing queue, raw payload retention or per-frame allocation was
introduced. The CPU reservation equation and its existing maximum height/window
allowance remain unchanged; two Main-only booleans track cancellation mode.

Implementation/check sequence: extract current worker and cancellation methods;
demonstrate the old Close path fails reuse; add Trim/completed-drain admission and
hard upgrade; verify lifecycle/locks with tiny files and a real worker thread.

Run from repository root:

```powershell
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' GlobalSatelliteMap/atlas-terrain/height-retention-tests/run_tests.py
```

The runner extracts the current production `Work`, `Send`, `Cancel`,
`DrainCompleted` and cancellation Pump branch into a small VS2010 host control.
Native, D3D and 537MB source authentication are substituted; the worker thread,
events and tiny deny-write/delete file handles are real. It reads no game asset,
creates no D3D device and builds no plugin DLL. The controls cover:

- Same path/epoch reuse without a second authentication, while writes stay denied.
- Epoch/path changes close the previous identity; no window/pixels/RGBA survive.
- Terminal Close while cancelled, cancelled reopen refusal, Stop and proven join.
- Foreign/Complete/accepted/query/busy states reject completed-drain admission.
- Fence wait precedes worker trim and GPU clear.
- Hard/context cancellation before or after Trim dispatch forces full Close.
- Already empty Idle drain remains a successful no-op.

`--legacy-close` intentionally selects the old ordinary worker Close command and
must fail `ordinary drain reuses authenticated height handle`. The unmodified
production worker was observed to fail that assertion before implementation.
Generated executables, snippets/logs and tiny temporary fixtures stay in ignored
`_generated/`. Failed controls terminate their process, closing all handles;
successful controls remove both tiny files after worker join.

These are lifecycle/ordering controls, not a test of production TIFF parsing,
native source freshness, actual GPU work or measured in-game speed. Native cold
latency and authentication counts still require a game measurement.
