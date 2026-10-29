# MCM input regressions

Run from the repository root after production changes:

```powershell
python .\ModConfigMenu\tests\wheel_probe.py
python .\ModConfigMenu\tests\capture_probe.py
```

Each runner compiles the **current** `ModConfigMenu.cpp` helper bodies with
native/UI boundary fixtures, then executes the assertions. It exits nonzero
on a compiler error or failed assertion. Fixtures are embedded in the two
Python files; no other test framework or Python packages are required.

Dependencies match the project: Windows, Python 3, VC10 x64 and Windows SDK
7.1 at the paths used by `ours/build.ps1`. Capture additionally reads the
current `shared/ModConfigMenu.h` and sibling `../KenshiLib-src/Include` OIS
header. All paths to project sources are relative to this tests directory.

- `wheel_probe.py`: controls scroll the native page exactly once without
  changing their values; nested/custom scroll areas and hidden/deleted hosts
  remain independent; adjusted WidgetInput base pointer, thumb/clamping and
  native redispatch are covered.
- `capture_probe.py`: initial selecting click, mouse opt-in and modifiers,
  matching press/release, keyboard repeats, NONE/Escape, legacy click-cancel
  and ordinary text input are covered.

Generated C++, object files, executables and link outputs stay in `generated/`
and are ignored. Neither runner accesses a live game, installs DLLs, writes
player INIs or replaces the final in-game UI check.
