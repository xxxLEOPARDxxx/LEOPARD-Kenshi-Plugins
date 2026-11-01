# AIFixed offline regressions

Current Basusu integration report and explicit player-report matrix:
[verification-basusu-integration-20261102.md](verification-basusu-integration-20261102.md).
Run with bundled Python; all outputs stay in ignored `_generated`:

```powershell
python tests/test_policies.py
python tests/test_bed_selection.py
python tests/test_lifetime_debug.py
python tests/test_gate_runtime.py
python tests/test_settings_locales.py
python tests/audit_rest_clock.py
```

The selector and gate/intent fixtures extract current production code. Native
medical data, query results and actual GOAP execution remain explicit boundary
fakes. They cover health/wear separation, radius/function/floor/ownership,
reservations, manual/Stop/queued intent, close/relock/occupancy, native faults,
TLS restoration and identity/queue/query budgets. The locale check compiles only
this mod's three catalogs and checks actual MCM keys/defaults/restart flags.
The read-only clock audit additionally verifies the installed Steam executable's
native pause/speed/hour path and actual Steam/GOG SDK table bindings. Rest
regressions cover continuing recovery beyond ten/twenty minutes, healing after
new damage, long pause/resume, native clock rewind/invalid input and simulation
time approach/stall bounds. The audit writes only ignored JSON; no process API.
The gate fixture also extracts the real GUI dispatcher/clock wrapper/reset, with
readiness and rest boundaries faked. It covers queued input and OPEN/close/relock
across long pauses, invalid-clock recovery, active-time expiry, world replacement,
continued original GUI updates and no native polling when gate work is absent.

Historical existing-rest reproduction:
`python tests/test_lifetime_debug.py --baseline-repair` substitutes the exact
SelfRepairOne from86642928f57ca18f0fb4a85b6809362652288adf into the current fixture.
It is **expected to fail** cancellation/reissue/live-disable/new timeout cases.
It does not replace native gameplay/FPS/memory/mod-compatibility acceptance.

## Original lifecycle/debug regression scope

Run `python tests/test_lifetime_debug.py` from AIFixed, or use its absolute path.
The runner requires the local VC10 x64 compiler and Windows SDK 7.1. It writes
ignored build files to `tests/_generated`; it does not load or install the mod.

The harness compiles the current production wrappers, session reset/readiness
helpers, state traversal, live-bed identity check, SelfRepairOne and MCM
description. Narrow boundary fakes provide unavailable native world/medical data.
It checks:

- Turning Debug off skips all four diagnostic bodies while preserving original
  calls, including diagnostics installed earlier; MCM exposes the restart flag.
- A missing heartbeat prevents access to a stale non-null world pointer.
- Character and building reuse cannot inherit an earlier object/container
  serial's attempts or reservation. The fixture deliberately supplies coarse
  native hand operators; the production five-field comparator must not call them.
- Load/new game/reset/destruction invalidate readiness before native callbacks;
  reentrant world updates cannot reopen it during those callbacks.
- The GUI owner clears repair metadata, reservations and diagnostic caches on
  generation changes or confirmed no-world data, including same-address loads.
- A 301 ms heartbeat gap merely suspends scans and preserves sent ownership,
  reservations, attempts and the normal scan interval. Once a fresh heartbeat
  resumes, production SelfRepairOne still issues GET_OUT_OF_BED after repair.
- A native load exception closes readiness and restores the loading counter.
- Missing player data or failed lifecycle registration prevents automatic scans.

The pre-fix source failed four behavioral checks: disabled diagnostics still ran,
medical diagnostics survived live disabling, no-world scans ran, and a reused
character address inherited attempts. These tests do not establish gameplay or
FPS results, and do not replace a game test of repair-bed selection and orders.
