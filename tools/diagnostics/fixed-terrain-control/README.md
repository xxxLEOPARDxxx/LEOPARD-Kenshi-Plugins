# Fixed terrain raster control, 2026-11-03

This is a standalone WARP executable with its own D3D device/context. It never
accesses a game process or the game's immediate context. It is not a viewer,
game DLL, new cache format, or a completed satellite feature.

Run the bundled Python `run.py`. VC2010 x64/SDK7.1 and the installed Windows Kit
FXC compile the small diagnostic shaders and the actual fixed-geometry helper.
Input fixture bytes are authenticated before invocation; no TIFF-wide image or
native function call is needed. Generated objects/logs stay in `_generated`.

Two real-source fixtures are exercised, each with four65-square native patches:

- Interior: global source origins7360/7424 on both axes, center(-13824,-13824).
- Previously reported old atlas boundary: origins7104/7168 on X and7360/7424
  on Z, center(-18432,-13824). This crosses the old tile-local output x6144 edge.

Each uses a256-square3m image, a3m image shifted64 pixels in both axes, and a9m
image at the original center. World-coordinate matched samples are compared.
Native anti-diagonal height interpolation has an independent barycentric oracle.
Normal interpolation separately observes the literal raw40B input fields
`value6,value4,value5`, vertex normalization, barycentrics and pixel normalization.
It deliberately does not call the helper's ShaderNormal function.

Fresh result:2,732,992 assertions per fixture and six compiled/running negative
controls rejected: missing patch, wrong topology, wrong stride, wrong index
origin, constant-up normal, and wrong input-normal layout. Normal-oracle
strengthening followed independent review; the earlier unit/upward-only check
would incorrectly have accepted a constant-up shader. Those older counts are
not the final acceptance evidence.

The diagnostic stores float32 absolute height and normal, not the native radial
GBuffer. It proves these tested geometry samples/window invariants, not native
material derivative/colour equality, all world/region boundaries, FPS or long-run
game memory. The compact material helper separately renders native-derived
geographic shaders and actual texture resources. No active atlas is replaced.

`preview_material.py [--spacing 1|3]` visualizes that helper's actual frozen
control buffers over exactly the same ground footprint. It follows the pinned
SatelliteDecodeAlbedo neighbour/parity rule and standard sRGB display transfer,
then nearest enlargement. It adds no lighting, exposure, sharpening or buildings.
Source/output hashes and the diagnostic scope are saved alongside each preview
in `D:/DEV/Kenshi/research/gsm-highzoom-budget-20261103`.
