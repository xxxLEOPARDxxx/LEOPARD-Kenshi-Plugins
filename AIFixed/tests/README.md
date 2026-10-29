# AIFixed lifecycle and diagnostic regressions

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
