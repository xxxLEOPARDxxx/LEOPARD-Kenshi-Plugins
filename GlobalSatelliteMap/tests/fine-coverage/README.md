# Opaque fine coverage guard

`GsmFineCoverage::Covered` is a pure scheduling guard. Main supplies actual
current context/map-widget identity, a freshly computed current FineGeometry
demand, center/phase state and a visitor proving each real fine entry exists,
has a shaded output and is opaque. The visitor is called once per plan key and
must not mutate entries. Use a namespace-level functor with `operator() const`
for the project's C++03 compiler.

The policy verifies Fresh identity/geometry, phase4/noncenter, the exact clipped
viewport lattice, count, unique keys and stamps. Thus phase4 or a center-only
plan cannot independently hide coarse fallback. It uses O(N) work and a fixed
512-byte scratch array; it retains nothing and allocates no heap memory.

When true, root may skip coarse shading behind complete opaque fine imagery.
Fine illumination/lamps/water continue normally. Any hole, pan, resize, map or
world change, unshaded/translucent entry or context/phase loss immediately makes
the next check false. Existing coarse imagery remains a fallback until its next
normal update (at most the existing200ms Draw cadence); the guard changes no
texture ownership, cache format, source validation or GPU admission.

Run from repository root:

```powershell
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' GlobalSatelliteMap/tests/fine-coverage/run_tests.py
```

The32 policy controls use actual FineJob/coverage policy with a bounded fake
entry visitor. They exercise full1m/3m coverage, holes/transparency/unshaded
entries, native world/capture/source/material identity changes, geometry/map
changes, center-only/incomplete/malformed plans, duplicate keys, clipping and
overflow. A further17 controls extract the actual `FineOpaqueReady` and
`FineCovers` code from FineViewer and substitute only map/widget/index interfaces;
they exercise hidden/missing/unbound widgets, wrong texture, alpha/rectangle,
context/map/geometry failures and restored presentation. No game assets, native
rendering, GPU device or plugin build.

Opacity is a scheduling assumption for supported finite shader output, based on
accepted full input coverage and actual widget alpha1. It is not GPU readback
proof that every output alpha equals255: nonfinite shader fallbacks can produce
transparent pixels. Existing coarse textures/widgets remain present; only their
refresh is suspended. Such a pixel can reveal previously lit same-world fallback
until the guard fails and normal refresh resumes. This guard does not remove
fallback geometry or stop fine lighting, lamps or water updates.

`--legacy` models old unguarded coarse rendering and must fail the first complete
coverage assertion. That RED was observed before implementation. Generated
executables, snippets and logs stay in ignored `_generated/`.
