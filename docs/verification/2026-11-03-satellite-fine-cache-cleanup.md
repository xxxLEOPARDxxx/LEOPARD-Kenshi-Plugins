# Fine-cache healthy-stop cleanup — 2026-11-03

Narrow implementation completed in `D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-cache/fine-store/` and `fine-worker/`. No game, INI, UI, coordinator or production integration files changed. No commit made by this agent.

## Behavior and ownership

`Session::DiscardKnownClosed()` runs on the owning worker. It preflights all valid records in the fixed private index (at most 128), then removes only those exact owned files using the existing HANDLE-relative identity-checked deletion path. It does not enumerate directories or perform recursive/pattern deletion. Wrong physical identity, unknown close, deletion errors and poisoned sessions preserve remaining forensic data. Failed live closes retain the whole owner through `NeedsRetention()`.

FineWorker calls this API before Close only on healthy normal/graceful-cancel shutdown, after borrowed/read data is acknowledged. A live held read remains valid and its file remains present until ReleaseRead and known join. Soft viewport changes keep the worker/cache alive. Main performs no disk I/O. The 134,348,800-byte disk cap and CPU reservation remain unchanged.

Empty owned session directories deliberately remain. Unknown, poisoned, corrupt and unrelated files are not swept. This change prevents healthy session payload accumulation; it is not a claim that all forensic directories or historical files disappear.

## Verification

Observed RED: `fine-worker/_generated/cleanup-red.log` contains `FAIL healthy worker stop discards closed owned fine payloads` before the shutdown fix.

Final full runs completed with /Od and /O2:

- FineStore: 429 existing controls, 54 syscall-fault controls, 64 new known-owned cleanup controls per configuration. Log: `fine-store/_generated/cleanup-store-final.log`.
- FineWorker: 29 normal controls and 150 fault controls across eight modes per configuration. Log: `fine-worker/_generated/cleanup-worker-final.log`.
- Cleanup tests cover physical same-size foreign replacement before any deletion, live CloseHandle FALSE retention/recovery, delete refusal, poison refusal, Main ownership refusal, held read cancellation and cancellation before Get opens a file.
- Fixture cleanup is manifest checked by the runners. PE imports checked by the runners exclude game/Ogre/D3D/UI dependencies.
- Final `freeze.py --verify`: 33 FineStore inputs and 40 FineWorker inputs. `git diff --check` clean for both folders.
- Parent independently reviewed bounded record preflight, relative identity deletion and live-close retention; no actionable issue found.

These are CPU/disk lifecycle tests, not a game FPS or long-running leak measurement.

## Frozen SHA-256

| Artifact | SHA-256 |
| --- | --- |
| fine-store/FineStore.h | 3559a2ef54febd597b0419d03a31237fdabd5b5d6117bf33533bb03bee3e7cb9 |
| fine-store/verification.json | f2f1ea911259641305a4e73ae03ab1a4f408e38088625e6f6d3fab0a11ba183e |
| fine-worker/FineWorker.h | f37408a32c3a654b5fb65b0992673dace4a41aecd5e24df1dfe10f0df2f94b21 |
| fine-worker/verification.json | 3d85e85309df6eaf79025548f881758394a8150bc0e8f8ddff8f2742a3589a39 |

## Exact source scope

FineStore: `FineStore.h`, `README.md`, new `cleanup.cpp`, `run_tests.py`, `verification.json`.

FineWorker: `FineWorker.h`, `README.md`, `tests.cpp`, `faults.cpp`, `run_tests.py`, `verification.json`.

Verification reports depend on current transitive production headers; regenerate their manifests if those inputs change later.
