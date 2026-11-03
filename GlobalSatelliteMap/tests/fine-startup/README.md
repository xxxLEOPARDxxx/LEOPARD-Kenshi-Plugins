# Focused startup budget regression, 2026-11-04

`run.py` compiles the actual `FineCoordinator::StartupFits` header and a narrow
extraction of the actual `MakeBudget`/`Owner`/`Extra` and native static reservation
bodies with VC10 `/Od` and `/O2`. Actual NativeStage/StageAdapter/NativeSession
types determine sizes. Only the static reservation accessor is substituted in
the extracted MakeBudget owner; no engine method, worker, D3D device, disk cache
or game is started. The executable imports only KERNEL32.

Runtime RED was captured before adding StartupFits: the old startup expression
charged retained fine images against the viewer-only allowance and rejected the
observed 34,143,518-byte town viewer plus six retained windows. Final24 controls
per optimization pass, covering0/6/256 resident windows,40MiB boundary/+1,
pending/retired debt attribution, physical-slot/byte bounds, integer overflow,
actual K2 reservation and the observed7,880,725-byte external CPU ledger.

`run.py --legacy32` separately compiles the old32MiB startup comparison as an
explicit negative control, without editing production headers. Both `/Od` and
`/O2` reject the observed town viewer with zero retained fine windows; expected
RED logs are `_generated/legacy32-Od.log` and `legacy32-O2.log`. The ordinary
runner regenerates the actual-current helper and passes afterward.

Actual native producer reservation:45,077,658B. Actual MakeBudget other CPU,
including the observed external ledger:9,971,365B. Combined55,049,023B remains
below67,108,864B. Full256-window physical cache +40MiB viewer + actual K2 uses
515,058,740B, below the unchanged536,870,912B aggregate GPU cap. Pending/retired
debt is still charged; no fake zero peak or new memory budget is introduced.

This uses the fixed current development profile. A scalable budget for weaker
GPUs remains separate future work; this repair does not claim automatic hardware
adaptation or measured game FPS. Actual game x6/x8 rendering and image acceptance
remain caller/integration checks. Generated code/logs are ignored and small.
