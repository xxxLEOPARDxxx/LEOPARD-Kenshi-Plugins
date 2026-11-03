# Fixed world-window placement

`WorldLayout.h` supplies integer placement for the new terrain pipeline. It
does not authenticate a source, choose a material, create a job, render, write
files or mark an atlas ready. The caller still establishes all current scope,
source/material and save/world authority.

`BaseCell(x,z,out[4])` covers one of32x32 native9216m material cells with four
distinct512x512@9m views. All4096 base windows align to the full world's origin
(-147456,-147456). Their centres are -145152+4608*k. Each window covers4x4
native64-cell patches; no geometry crosses its material cell. GST2 placement is
one of4x4 tiles, with exactly aligned512-pixel offsets inside each8192 tile.
There is no resampling, half-pixel shift, overlapping base window or uncovered
base pixel. The region catalog's actual world origin is checked rather than
silently accepting an assumed ordering. Inputs outside the32x32 grid refuse
before multiplication and clear all output placements.

`Fine(key,out)` accepts the existing bounded planner's valid256x256@1m/3m keys.
It preserves their exact centre/density and selects the smallest covering
rectangle of existing1152m mesh patches, at most2x2. Those footprints align
inside native material cells, including at world edges. Provenance labels must
be nonzero.9m and invalid keys refuse and clear output. Fine tile/texel offsets
belong to their separate density namespace: **do not send them to the512@9m
GST2 base writer**. This primitive neither implements nor allocates a fine cache.

The implementation uses fixed small outputs, integer bounds and catalog reads
only. It has no allocation, growing queue, thread, file, D3D or native API call.
Run `python run_tests.py` with the bundled Python/VC10x64/SDK7.1. A runtime RED
against a rejecting stub preceded implementation. /Od and /O2 each pass157,189
controls: exhaustive4096 base positions with unique/full coverage, independent
world-to-tile mapping, and fine coordinates across both axes and cell/world
boundaries. These checks do not prove material continuity, actual rendering,
writer IO, runtime performance or completion of the current working map.
