# Fixed global terrain geometry — isolated helper, 2026-11-03

This CPU helper is implemented and host-tested. It is not wired into Main, the
probe, the viewer or a game capture. User approval now permits changing the terrain
method and improving maximum zoom; the older prohibition on a renderer rewrite
in the historical overlap reports has been revoked. No full atlas is regenerated.

## Source and world contract

The supported source is the actual `fullmap.tif`, SHA256
`6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18`.
It is LE16, one uncompressed strip at43686,16385x16385 samples, orientation1.
Native14303 uses `(width & ~1)/294912`, so source sample(i,j) is exactly
`(-147456+18*i, float(uint16)*float(9800/65535), -147456+18*j)`.
Native14430 truncates source coordinates and returns0 outside the world. At
these exact integer source vertices no bilinear reconstruction is involved.

Final base output remains4x4 tiles of8192 pixels at9m/pixel. The source has18m
geometry;9m and later1–3m output rasterize the same real triangles and can reveal
material/model detail without inventing finer heights. A cell uses the native
row-strip diagonal, not a bilinear surface or the opposite diagonal.

## API, ownership, layout

`FixedTerrainGeometry.h`, namespace `FixedTerrain`:

- `Samples{minX,minZ,width,height,stride,data,sampleCount}` borrows an already
  validated uint16 source rectangle. Indices refer to the global0..16384 grid.
  The caller owns/pins the source through `Build`. No file, native API, pointer,
  job, mesh or texture owner is retained. The outer file adapter must validate
  identity, byte order and payload span before constructing this typed view.
- `Build(samples,x,z,out,count)` accepts global source origins divisible by64,
  and outputs4225 caller-owned40-byte vertices. Destination capacity and the
  declared source span are checked before any writes. It needs two source samples
  beyond each patch edge, limited only by the WORLD edge. Outside-world samples
  are zero as in native14430; internal chunk/capture edges must never clamp.
- One patch is1152m wide. Scratch is67x67 fine normals,53,868bytes on the stack;
  there is no heap, cache, filesystem, thread, queue, lock or global mutable state.
- `Indices(out,count)` emits8572 uint16 strip indices,8192 real triangles plus
  degenerate joins. One immutable17,144-byte index buffer can be reused by every
  patch using base-vertex/vertex-buffer binding offsets.

The literal raw packet is ten floats:
`X,Y,Z,fineNormal.xyz,coarseNormal.xyz,coarseTargetY`.
The pinned native GPU declaration is **POSITION float4 at0, NORMAL float3 at16,
BINORMAL float3 at28**, stride40. Upload1F76C copies the packet verbatim.
Leaf3960 passes the same inline Patch+C8 geometry; no repack was found or assumed.
The native-derived VS uses normal.zxy. Consequently at **morph0**, its input
world normal is `normalize(raw[6],raw[4],raw[5])`, not simply fineNormal.xyz.
This is an observed file contract, not a claim that the live game has a bug.
The helper preserves it rather than silently changing the material slope basis.

Use the existing `GeographyVertexSource` with the declaration above, **morph0**,
identity world matrix and absolute world vertices. POSITION.w is raw fineNormal.x
and is deliberately ignored by morph0. Do not add a local patch translation or
change POSITION.w to a guessed alternate representation. A different explicit
vertex layout would need a separately tested shader adapter.

The existing VS outputs stay TEXCOORD0 world normal,1 world position,2 horizontal/
vertical UV,3 map/overlay UV,4 cliff blend and5..8 fifteen fixed SID cliff-height
components. Keep the fifteen distortion slots even in the compact two-SID test.
Cosines run at actual GLOBAL18m mesh vertices and interpolate over these exact
triangles; moving cosine evaluation to fragments changes phase/interpolation.

Camera/view/projection belong to the raster caller. Two shifted test views must
use equal-world pixel centres on the same3m or9m grid. No live floating-origin
offset is borrowed. Any future origin rebase must preserve absolute coordinates
used by UV, selector, cliff phase and source keys explicitly.

## Native normal and index proof

The pinned47A0 builder has65x65 vertices and ten floats per vertex. Fine normals
sample the cyclic offsets `(0,-1),(1,-1),(1,0),(0,1),(-1,1),(-1,0)`, sum six
successive cross products, normalize with Ogre67070's float sqrt/reciprocal order
and negate. At a patch border the height callback samples outside that patch.
The5653..59FD pass computes target normals from the normalized sum of two fine
normals: odd/odd uses(-1,-1)/(+1,+1), odd/even uses horizontal neighbors,
even/odd uses vertical neighbors; even/even copies the fine normal. This is why
literal GPU normal compatibility requires TWO source samples of support.

Native5AA0 emits row pairs `(row*n+x,(row+1)*n+x)` with four degenerate indices
between rows. Native5E10 can rewrite boundary indices for unequal neighboring
LOD. This helper has one fixed global finest grid, so no adaptive border collapse
or camera-dependent morph/index change is retained.

`native-evidence/` contains the actual file-profile hashes, instruction windows
and a rerunnable file-only audit. No DLL is loaded or native function executed.
The raw→GPU declaration and shader semantics above are file evidence; actual
runtime vertex fetch/raster/material parity remains a separate WARP/native-control
and game acceptance boundary.

## Self-contained fixtures and tests

Run `python run_geometry.py` here. Only the compact pinned fixtures and VC2010x64/
SDK7.1 compiler are needed. No TIFF, game DLL or process access occurs in this
standard test command. `fixture-pins.json` authenticates the fixture bytes.

- `height-window.u16` is a196x134 real source rectangle at7102,7358, with four
  LEuint32 header fields followed by row-major LEuint16 samples.
- `expected-patch.f32` holds4225 raw40B packets from an independent Python port
  of the pinned instruction order, not native execution or an analytic height
  substitute. The C++ helper matches every byte.
- `raster-window.u16` is **133x133**, global samples7358..7490 inclusive on both
  axes. Header16bytes plus payload35378bytes =35394bytes. Four patch origins are
  (7360,7360),(7424,7360),(7360,7424),(7424,7424). Their common corner is absolute
  (-13824,-13824), suitable for root's first256-square3m/768m controlled view.

Current test result:4507 checks PASS in both optimized and unoptimized builds; six separately compiled controls fail for
fineXYZ substitution, missing second halo, wrong global parity, wrong corner
diagonal, wrong strip columns and clamping the world edge. Shared40B horizontal/vertical edges and
four-way corners are exact. Bad destination, source count, alignment or support
refuses before writes. The pre-helper missing implementation was observed RED.

`geometry_contract.py` optionally regenerates fixtures by hashing the whole real
TIFF then reading only bounded rows. It is not needed for normal self-contained
testing. Its real-height negative controls show14.524m bilinear error,19.365m
opposite-diagonal error and13.709degrees chunk-clamped normal error in this sample.
These fixtures prove the port/ownership boundaries, not live image parity or FPS.

## Bounded next raster proof

Root owns `../fixed-raster`; no renderer edits are part of this helper. First
render actual256-square3m source-pinned geometry around(-13824,-13824), then
shift by an integral number of3m pixels and compare the identical world overlap.
Also compare one whole assembled mesh against four separately drawn65-square
chunks, including the corner. Check validity/coverage, world height, geometric
normal and depth before compact material or lighting. Preserve R16 strip winding
and native-compatible VS declaration. Do not infer success from patch counts.

Then connect the independently owned compact2SID material helper using the SAME
vertices/VS and global material inputs; verify RGB/gloss FP16, normal and height
in equal-world overlaps, including world cliff phase. Keep rawYCoCg decoding out
of this new RGB control. Full atlas regeneration follows only after actual proof;
cross-region palettes/paint and world-edge controls remain additional acceptance.

## Memory, disk and scope

`budget.json` is a conservative planned inventory, not a measured peak. The base
2048/9m block has256 patches,43,264,000B per CPU/GPU vertex copy and2,097,152 real
triangles, within the unchanged16M policy. Rendering a full8192/9m tile at once
would have33.55M triangles and is excluded. Full-world18m vertices alone would be
11.08GB raw CPU data; no full-world allocation is proposed.

The base inventory counts the existing503,315,252B DDS/paint/selector pools,
96MiB targets including RGBFP16/normal/radial/depth/owned output, CPU/GPU vertices,
shared indices,64MiB readback,64MiB copied writer output, one upload mirror,
1029-square source samples, scratch, one encoded DDS, metadata and128MiB explicit
unmeasured-driver headroom. The total remains below1GiB. Implementations must
admit against the actual complete ledger and refuse on exhaustion; no cap raise.
Old private native Terrain must not remain resident beside the replacement mesh.

Base GST2 remains16 files /22,900,916,224 logical bytes. Keep the old atlas while
building the new generation: both generations total45,801,832,448 logical bytes;
preflight at least24GB free for the new raw generation plus temporary metadata.
Existing compression is not assumed for future writes. New metadata must record
the changed global geometry/normal/shader basis and source profiles, never reuse
the old capture identity. No installed GST/mip/index is replaced by this helper.

Finer detail must be an on-demand visible-window layer with an explicit bounded
disk LRU, not a whole-world3m/1m atlas. Geometry stays18m. Actual screen world/pixel
density selects needed output; universal1m is not promised. Runtime192MiB budgeting,
compact material/SID batching, painted gradients, model residency and any explicit
window-count rebalance belong to the parallel material/cache design. Current144
windows reserve189,006,000B (180.25MiB), leaving only11.75MiB; the full503MB pools
cannot coexist with that reservation. Reducing declared residents is an explicit
architecture decision, not an automatic increase of the cap.

Water remains the actual stock horizontal native plane, not a heightmap box or
fake coastline. The pinned constructor uses a500000-square plane with100x100
segments, UNIT_Y normal and UNIT_Z up, under its separate absolute water height.
This geometry helper does not implement water shading/depth bias, models, lights,
streaming adapters, device lifecycle, Main wiring, install, commit or publication.
