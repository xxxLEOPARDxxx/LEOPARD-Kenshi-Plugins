# Local light integration, 2026-11-02

Keep the existing terrain path unchanged when disabled. Add a separate MCM switch
(RU/EN/ZH, default off) for lights already registered in the current native scene.
No world/zone activation and no invented lights for unloaded towns.

Acquire copied point/spot values at most once per second while the map is open.
Use the independently audited native snapshot and HDR layer. Before admission,
require a paired native town-position/node/origin/camera read proving the raw-scene
to GST translation; reject changed generations/origins and any inconsistent pair.
No retained Town/Entity pointer. Failed native reads latch until scene/generation
change or explicit option reset, retaining terrain/water.

Compose terrain HDR, optional local contribution, optional native water, then one
exposure. Share the existing four-window/200ms scheduling budget. Detect exact
unchanged light snapshots before invalidating windows; do not relight on a paused
unchanged clock. Do not shade blocks outside all light ranges. Fixed local private
payload526416B counts in the common192MiB texture/buffer budget; no per-render
resource allocation, source enumeration, disk reads or new atlas captures.

Verification: production mapping admission tests and functional mutants; extracted
caller routing/lifecycle/budget checks; existing native HDR/WARP suites; independent
integration review; live proof with nonzero scene origin; map on/off, panning and
load cancellation; timed memory/CPU observation. Short tests are not a guarantee
against all leaks or an FPS measurement. Buildings and local-light shadows remain
separate implementation work.
