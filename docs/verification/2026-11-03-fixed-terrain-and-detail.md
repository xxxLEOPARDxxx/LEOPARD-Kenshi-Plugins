# Deterministic terrain and closer material controls, 2026-11-03

The user authorized a terrain-method rewrite and explicitly requested improved
maximum-zoom detail. The previous restriction no longer applies. The installed
atlas remains untouched while the new producer is verified.

## Verified geometry primitive

`GlobalSatelliteMap/atlas-terrain/fixed-geometry` builds native-sized65-square
patches on a fixed global18m source lattice. It preserves the observed raw40B
input layout, native strip diagonal and normal/coarse-normal evaluation, with a
two-source-sample halo. No camera selects refinement or collapses patch borders.
Source TIFF coordinates, native instruction windows, fixtures and ABI are pinned.

Fresh host checks:4507 each in optimized/debug builds; six compiled functional
mutants rejected. The independent instruction-port packet fixture is an offline
reference, not a claim of executing the native game builder at runtime.

The standalone WARP diagnostic in `tools/diagnostics/fixed-terrain-control` uses
its own device/context. Root and independent review each ran2,732,992 checks on
each of two real fixtures, plus six compiled/running raster mutations. One
fixture crosses the actual previously reported atlas edge at worldX=-18432.
The other is the compact native-material fixture centered at(-13824,-13824).
Shifted3m images and coincident3m/9m samples retain height/normal within the
declared tolerances. The diagnostic uses direct float32 height, not radial depth.

Independent review: `research/gsm-fixed-control-review-20261103/report.md`,
SHA256 `2c4ab26c3adde4ae58f1bf621041c276e791bd3f91651127664a6d43e4773fb8`.
Its extra odd/negative shifts and invalid-view recovery checks pass. Deliberately
moving a9m view beyond the fixed four-patch coverage exposes512 missing pixels;
that view is not accepted as covered. Unit/upward normal checks alone were found
insufficient, so a literal packet/interpolation oracle and constant-normal mutant
were added before acceptance.

After review, root removed only trailing text padding from generated native ASM
and made its generator trim the same padding. Native code-byte hashes, helper,
fixtures and numeric tests are unchanged. Freeze963266b94bbd46445df2d558598176ab7cfd6a1df3513d1120f069d2e7689489
records this narrow formatting maintenance. Byte-pinned files retain exact
line endings in git; normal whitespace checks remain enabled.

## Real closer material output

The separate compact material control uses the real selected two-biome case,
12 actual DDS textures, painted maps and the unchanged native-derived geographic
VS/PS.1m,3m,9m and shifted3m outputs exist. Geometry height remains18m; finer
rasterization resolves actual textures and shader variation, not invented terrain.
This is not a final game screenshot or a global cache implementation.

`preview_material.py` displays actual GBuffer material samples with the pinned
YCoCg decoder and standard sRGB transfer. Both panels cover the identical ground
rectangle; nearest enlargement adds no sharpness or lighting. Source/output
hashes and scope are saved beside the images. The current example previews are:

- `research/gsm-highzoom-budget-20261103/material-detail-3m-vs-9m.png`
- `research/gsm-highzoom-budget-20261103/material-detail-1m-vs-9m.png`

## Integration limits

These tests do not establish a complete seamless world atlas, native lighting,
full city coverage, FPS, or long-run game memory. The geometry and compact source
owners are not yet called by the installed map. A game-context-safe raster pass,
source/output ownership and complete budgets are required before deployment.
Packed material still has an independent neighbour/phase decode edge; a private
full-RGB terrain writer is being verified separately. The current atlas and
personal settings remain available as fallback. No public release or journal
entry is implied by this development checkpoint.

## Production primitive follow-up

The private actual-game adapter and its three completed four-view runs are
recorded separately in `2026-11-03-fixed-terrain-game-control.md`.

`rgb-raster/` now implements context-safe direct RGB16F output with exact GPU
payload accounting and withdrawal of interrupted frame publication. Root and
independent review passed the nine actual material controls/all three planes,
29 actual allocation/health failure cutpoints and the runtime unwind/negative
controls. Independent report SHA256:
`d098805d68d52894c2e422d24f34924c2aec6c3b54463e5ff8e313bbce31e057`.
After review, root removed only one redundant final CRLF from the header to
pass whitespace validation. A fresh full run passed the same9,437,694 main and
9,437,706 runtime checks,29 failure paths and six negative controls. All187
borrowed dependencies remained byte-identical. Packaging-amended freeze:
`82d321253aff2847acb723d0ce48f18d6feaf57e3998c390b7c3464457dac42c`.
No C++ tokens, shader bytes, ABI, math, resource ownership or output changed.

`world-source/` authenticates the real536,980,180B TIFF once per retained
read-only handle/epoch and streams only requested rows plus the world-clipped
two-sample normal halo. Maximum window2,117,682B. Root fresh /O2 and /Od each
passed3,478 checks and14 compiled negative controls. All256 world blocks used
541,863,200 exact partial-row bytes; no whole-world allocation. Independent
review additionally checked actual write/delete/rename exclusion on its OWN
sparse fixtures, cancellation at final boundaries, full64-bit identity and
callback-unwind cleanup. Report SHA256:
`1b619e4af260f017dc4ff8e24d3103040dd00bdccec3736a219a542766033ea7`.
Unchanged source freeze:
`bab2682335ca2343feccd0006710189bd0f03d12123a63eb5cc4f94a0e322bcf`.

These two reusable primitives are not yet production Main integration. The
material fixture is still two-SID and cannot silently represent other regions.
The general bank plan must preserve selector order and current source identity,
with explicit shared producer/viewer residency. Neither isolated component
changes the installed viewer or its maximum-zoom detail on its own.
