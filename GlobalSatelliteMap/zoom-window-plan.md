# Native-resolution map streaming, 2026-11-02

Observed live: zoom changes destroy every visible image; the viewer never reads
the 8192 or4096 source levels. This is a viewer limitation, not a new capture.

Use256-square windows from existing GST2 levels0..5. Keep16 coarse fallback
images plus48 detail LRU slots, under the existing96MiB GPU cap. Retain images
across pan/zoom; only evict an unneeded detail slot when admitting a replacement.
Choose the mip from projected pixel density, coarsening only when more than48
visible windows would be required. Read one bounded window in the file worker;
the render thread only uploads completed bytes and shades bounded windows.

Pin each window's three hashes with a roughly2MiB index, built once from the
existing complete files while verifying their original whole-plane hashes.
No terrain recapture, new image tile files, or additional large cache copy.
Keep source identity, cancellation, native projection, markers and light state.

Verify native-level selection, viewport seams, LRU protection and memory bounds,
reader corruption/cancellation, full build/imports, independent review, then
install only when the game releases the DLL. Live zoom/quality and timing remain
acceptance checks, not implied by static tests.
