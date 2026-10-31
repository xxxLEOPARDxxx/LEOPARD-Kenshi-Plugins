# Global Satellite Map: development prototype, 2026-10-31

The requested feature is an optional satellite view on the GLOBAL map. BestMinimap keeps its existing map source. The target atlas is exactly 4 x 4 tiles, each 8192 x 8192 (32768 squared logical pixels).

## First coherent stage

1. Extract C++03 policy into `SatellitePolicy.h`: strict bounded manifest parsing, half-open tile edges, visible-tile selection, mip choice and aggregate memory budget. Test this production header with VC10 before integrating UI.
2. Add a MapScreen::update child overlay, below native/custom markers, with no mouse/key focus. Never change mapImage's texture, coordinates, events or child markers. Use static WidgetRef references for all owned widgets.
3. Accept only a complete genuine capture manifest (16 tiles, six independently loadable BC1 DDS levels 8192..256, SHA-256 per resource). Do not provide fabricated images or an example manifest that can accidentally be enabled. Validate headers before any overlay, hashes before upload. Load at most one visible level per 200 ms; strict compressed-data cache budget 96 MiB, maximum 16 resident textures, no full-atlas RGBA expansion.
4. Keep capture input identity and save identity separate. Manifest sources enumerate exact hashed capture dependencies including mods.cfg and base data. Require every enabled mod filename to occur among verified sources; missing/unresolvable sources fail closed. Save identity hashes native quick.save and all zone/platoon files, with hard file/byte limits. Path, live world pointer and load/new-game generation invalidate resident data. No changes to save/world files.
5. MCM switch defaults off; three locales; Debug=0 is last. Missing/invalid assets leave overlay absent and show localized status once per attempt. Native load/new-game hooks only invalidate plugin state before calling originals. No install, release archive or capture invocation.
6. Build VC10 /GL + /LTCG; run extracted-policy and lifecycle/source-contract tests. Report source/DLL hashes and limitations. Full UI/game/FPS/leak acceptance remains pending.

## Explicit later stages (not substituted by this prototype)

Prove orthographic/global projection and fixed exposure on a small terrain/hill/building overlap. Audit DistantTown ownership, render-only geometry, native world lighting, player/modded building changes and unloaded regions. Then implement a native dynamic layer and efficient staged capture with globally consistent mips/edges. A static captured preview is not the requested continuously lit world; two photographs cannot replace native sun/shadows/lamps. No force-activation of all zones.

## Constraints / review focus

Existing APIs: MapScreen::update and worldToMapCoords; SaveFileSystem::loadGame/newGame/getActiveSave; Ogre's native BC1 image/texture loader. Important risks: map window destroyed/recreated, same-slot reload, interrupted loads, marker stacking, zoom/scroll boundaries, corrupt DDS, memory exhaustion and synchronous disk/GPU stalls. First-stage source validation is bounded but can be expensive: it runs only on explicit enabling/reload with a genuine manifest, never every frame. No runtime performance claims before measurement.
