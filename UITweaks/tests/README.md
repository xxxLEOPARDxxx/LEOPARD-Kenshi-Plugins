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

HUD correction tests (same unreleased v12): `save_preview_gpu_test.cpp` uses an
actual WARP D3D11 device to verify scene pixel and OM binding restoration, including
scope unwind and unsupported descriptors. Alpha tests compile the current hook
and real production transaction; normal/skin alpha opt-outs, deleted widgets,
1024-entry capacity rollback and guard unwind are exercised. Selection adapters
cover no default preview and a click on the already-highlighted first row.
Capture-order fixtures extract DrawGui and its modal/root helpers; they check the
two-pass HUD path, post-GUI single-pass path, readback failure, backup refusal,
load cancellation and nested RTT passthrough. Driver calls are represented by
a scoped boundary in this sequencing fixture; the actual GPU class has its own
independent WARP test. These checks do not replace the new in-game acceptance.

The refinement suite also covers filtered 1600x900 lossless PNGs, legacy JPEG
sidecars, unchanged-good-file behavior on encode failure, empty-state hiding,
both Save/Load selection, and stable opening geometry. `test_save_preview_height.py`
extracts the real bounded per-menu height adapter. `test_save_preview_dialog_height.py`
loads the installed MyGUI DLL and verifies actual native child viewport growth
against all four installed vanilla/Dark Save/Load XML layouts. The height setting
is applied on menu opening and never compounds between Save and Load roots.
