# Native geometry regression

Run `python test_geometry.py` on the configured Windows VC10/SDK7.1 stand.
The test extracts the current production helpers and calls the installed MyGUI
DLL with a small native Widget/Skin/LayerNode fixture. Generated binaries and
logs stay in `_generated/`.

Coverage includes pixel conversion, repeated unchanged relative sizes, changed
sizes, explicit refresh callbacks, derived controls, parent clipping, nested
helpers, callback reentrancy, toggle-off and C++/SEH cleanup. The fixture pins
the local MyGUI ABI; this is not a game scene or FPS benchmark.
