# Idempotent fine tile presentation

Measured debug evidence showed about1.54ms in FinePresent every stationary frame,
even with no new lighting renders. The old function already guarded most UI
setters and stable visibility, but still called native worldToMapCoords twice
per shaded visible tile and repeatedly checked the same owner context per tile.

FinePresent now retains copied projected rectangles in the existing bounded
1024-widget pool plus a small geometry token. Current map/main/image/scroll/layer
identity, demand revision, canvas rectangle and viewport rectangle determine
when projections need refreshing. Stable frames perform zero native projections
and zero UI setters. Actual widget visibility, texture, depth and coordinates are
still checked cheaply; externally changed coordinates are repaired from the
copied native rectangle. Newly created/rebound/shown tiles project once. Lighting
renders into an already bound texture do not alter presentation metadata.

The fixed16KiB rectangle array and token are charged in FineCpuLedger. No CPU cap
increase, heap cache, queue or native owner reference was added. Identity tokens
are compared only; they are never dereferenced later. Existing WidgetRef remains
the live widget authority. FineCancel invalidates the token. Pan/resize/context,
retirement/reuse, new world/layer and missing widgets retain safe refresh paths.
A scoped reentrancy flag unwinds even on a projection exception.

Creation no longer sets depth twice; the correct fine depth is set only when it
differs. Detach clears texture/hides only when those values change. No marker
widget or parent layer visibility/depth is modified. Actual icon flicker recovery
and game FPS improvement still require native measurement; these controls prove
presentation behavior and avoided work, not a universal UI flicker fix.

Run from repository root:

```powershell
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' GlobalSatelliteMap/tests/fine-present/run_tests.py
```

The runner extracts the real cache helper, FineDetachWidget, FineVisibility and
FinePresent into a tiny host. It uses actual fine index/widget-slot classes and
controlled widget/map interfaces with projection/setter/allocation counters.
Twenty-seven checks cover stable100frames, unchanged lighting, first shade,
depth, canvas/viewport changes, reentrancy/exception, external coordinate repair,
context/map/layer refusal, pan, retirement/reuse, destroyed WidgetRef, world/layer
replacement and marker visibility. Original production code was observed RED at
`unchanged100 frames perform zero native projection and UI writes` before changes.
No game assets, actual UI/D3D calls or plugin build are involved. Generated host
artifacts/logs stay in ignored `_generated/`.
