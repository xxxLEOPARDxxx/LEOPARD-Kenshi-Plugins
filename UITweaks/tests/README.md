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
