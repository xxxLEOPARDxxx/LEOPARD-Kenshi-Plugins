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
python tests/test_guard_trace.py
python tests/test_landing_runtime.py
python tests/test_delay_settings.py
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
The guard trace fixture extracts production Inspect, CanTravel, ReadGuardCycle,
SameIntent and uses the real Observation policy. Its read-only 2026-11-12 capture
contains all nine distinct snapshots and their 316-observation histogram. The
original collector retained first-seen snapshots and aggregate counts, not the
full temporal order or game clock. The repeated order and >4-hour progression in
the regression are therefore explicitly synthetic; all nine initial phases are
tested. Native collision/navmesh/landing calls remain explicit boundary fakes.
Physical observation and one-hour failure freshness are separate: a long
no-action/no-goal gap can retain a continually sampled same-support position,
but cannot relocate without a current active movement attempt and a recent
confirmed failure.
The tests preserve recent real failure evidence across native goal resets while
rejecting relocation during reached/at-feet/no-action phases, including a reached
state arriving between the probe and final revalidation. Physical arrival,
mobility, another support and native medical/combat/player exclusions retire
the old physical watch. Stale movement evidence and rejected landing geometry
block relocation without deleting a continuously sampled same-support watch.
The 2026-11-13 followup additionally covers four separate synthetic mixed-goal
actors. Bed/shopping/other goals retain a physical episode only on the same
support; current MOVE_ON_FREE_WILL and distant blocked native navigation are
required to start an attempt. The actual native movement action remains
valid after the high-level GOAP goal is cleared, as observed in the latest log.
A current native MOVE with a current remote FAILED/BROKEN/INCOMPLETE path is
current blocked intent even when GOAP recreates the action or its state remains
unchanged for four hours. A retained native failure without that movement
action cannot refresh evidence or admit recovery. WAITING requires a failure
within the preceding game hour; inactive observations only retain physical
history. Combat/medical exclusions still require a new physical window.
The landing-flow fixture extracts actual FindLanding and checks geometry and
rejection diagnostics with explicit native query fakes. If terrain were 140,
the saved upper actor heights 299-315 exceed the former 14 m platform limit.
`inspect_platform_terrain.py` independently authenticates fullmap.tif and
samples base-terrain triangles at their exact saved coordinates: the actual
base-ground descents are 15.91, 17.51 and 16.60 m. Confirmed steep-platform
recovery now permits at most 18 m; the actual landing-flow fixture covers
those coordinates, the 18 m boundary, over-limit rejection and unchanged
gate/rock limits. Base terrain is not native walkability or collision proof;
all original dry/same-zone/clearance/barrier checks remain mandatory. The lower captured guard
height is accepted only when every fake physical query confirms a safe exit.
The runtime replay's fake landing now also respects the production distance
and minimum descent limits; actual native landing queries remain faked.
`inspect_saved_platform.py <save...>` reads v15 platoon instances/state records
without opening a process or modifying saves. It requires the local read-only
modread Reader and writes only ignored `_generated/saved_platform_actors.json`.
Platform diagnostics are gated by Detailed log, limited to one observation
per actor per 15 seconds and five ordinary lines/second globally. Probe-result
lines use the existing probe budget (at most two/second), with no extra native
collision or spatial queries. No CPU/FPS/memory claim follows from these tests.
`python tests/test_guard_trace.py --baseline-reset` runs the current replay
against the pre-fix `3bc09133341a2a87a32bfaca800be086a2bfae3e` runtime and is
expected to fail all nine cycle phases;
it does not modify production source or inspect a live process.
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

Latest actual Lagoon failure analysis and candidate receipt:
[verification-platform-followup-20261113.md](verification-platform-followup-20261113.md).

The live UnstuckDelayHours control defaults to 1 and accepts integers 1..4.
Actual MCM descriptor tests verify its range/default and lack of restart flag.
The runtime and policy fixtures cover 1/2/4-hour waits, live raising/lowering,
retained physical history and independent retry backoff. Legacy captured
replays explicitly select 4 hours. test_delay_settings.py compiles the actual
LoadSettings and its globals against a temporary absolute INI; it checks
missing-key fallback, clamping and preservation of existing personal values.
The native INI path/default preparation boundary is explicitly faked; the
installed INI is never opened or modified by that fixture.
