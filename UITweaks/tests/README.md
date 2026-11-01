# Native geometry regression

Run `python test_geometry.py` on the configured Windows VC10/SDK7.1 stand.
The test extracts the current production helpers and calls the installed MyGUI
DLL with a small native Widget/Skin/LayerNode fixture. Generated binaries and
logs stay in `_generated/`.

Coverage includes pixel conversion, repeated unchanged relative sizes, changed
sizes, explicit refresh callbacks, derived controls, parent clipping, nested
helpers, callback reentrancy, toggle-off and C++/SEH cleanup. The fixture pins
the local MyGUI ABI; this is not a game scene or FPS benchmark.


Run `python test_right_click.py` for the RMB regression. It extracts the current
production input policy and all five order hooks into an isolated VC10 fixture.
Coverage includes portrait/context-menu Trade, its original auto-approach path,
map commands, inventory frames/icons, detached backpacks, replaced roots,
null/cyclic/bounded ancestry, held buttons, release/close latch preservation and
swapped mouse buttons. A 1000-call steady-state check detects repeat parent
queries and allocations in these extracted paths. This does not measure in-game
FPS or prove freedom from every possible leak.

Run `run_save_preview_tests.ps1` for the save-preview production boundaries.
The runner compiles VC10 fixtures for size/layout/state policy, the real Windows
WIC codec, and extracted current production writer and save/load adapters.
Generated screenshots are codec test patterns only, never represented as game
captures. Fixture files stay under `_generated/owned-save-fixture`.

Writer coverage uses real kernel threads, files, COM/WIC and atomic replacement:
success, early cancellation, cancellation before commit, changed save revision,
missing directory, normal join, bounded timeout/quarantine and RAII lock unwind.
The save adapter fixture models native sync clearing the error before return,
so checking errors too late fails. It also covers same-name slots in different
roots, refused saves, quick/auto saves, disabled/wrong-thread/title-menu guards,
and load/new-game invalidation. Existing RMB and native geometry suites are still
required and must pass alongside these tests.

`audit_save_previews.py` is explicitly read-only and requires an already running
Kenshi process. It resolves native methods using the installed KenshiLib export
stubs and Steam RVA table, records their native code, and reads actual RTTI for
the MyGUI backend and render window. It never launches the game, injects calls,
writes process memory or controls windows. Its output is ignored in `_generated`.
Runtime scene/skin/save acceptance and actual FPS/retention measurements are
separate from these isolated boundary checks.
