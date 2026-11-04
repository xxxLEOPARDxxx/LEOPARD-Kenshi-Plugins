# Fair fine-tile illumination cursor

`GsmFineShadeCursor::State` is 12 bytes of Main-owned scheduling state. Call `Cursor(visibleCount)` once per frame before the two shading passes. `Index(scanned)` traverses that fixed rotated snapshot. After successful rendering, call `Published(index)` to advance the next frame. Keep the existing total of two renders/frame across both passes. First pass selects never-shaded images; second selects previously shaded images needing refresh. No allocation, resource ownership, clock or native calls are added.

The pure helper accepts at most `2*GsmCacheProfile::MaxVisible` indices (currently 1024), allowing a mixed 1m/3m fallback list, safely handles zero/count changes and returns sentinel 1024 for out-of-range scans. Publication does not change the current traversal; skipped/failed rendering without publication preserves scheduling position.

Run `python run.py`. The actual helper is tested under /Od and /O2 with two-pass scheduling. All 338, 512, 518 (mixed fallback) and 1024 initially shaded tiles receive a refresh within ceil(count/2) frames despite a new lighting epoch every 60 frames. Tests also cover initial unshaded priority, every-index-once traversal, wrap, changing counts, empty/oversized sets and unsuccessful publication. `python run.py --red` replays the observed fixed-index-zero scheduling fixture and must fail at the tail-starvation assertion. It does not mutate production source.

This verifies scheduling and boundedness, not live-game FPS or GPU rendering. Main integration remains the parent's scope.
