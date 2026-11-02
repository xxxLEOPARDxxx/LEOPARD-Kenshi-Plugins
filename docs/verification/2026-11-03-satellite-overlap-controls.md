# Actual satellite overlap controls, 2026-11-03

This is evidence of an unfinished producer boundary issue, not a released fix.
No full-world recapture, installed GST/mip/index replacement or global LOD-camera
modification was performed. Controls run through the existing reviewed private
Terrain with one new diagnostic directory each, separate from the active atlas.

## Actual captures

Both captures are2048square,9m lattice, altitude16000, primary1,1/region3,3,
detail32/32/0 and fade0. Copied-only file workers write48MiB+32bytes.

- Token1520, sourceStart5120,6144, centre(-18432,-9216), PID15108.
  control.bin SHA25669313f4ee447392007e7adf59ea64b31e751c7e32c5eb995b9cf55727c5fb337.
  Native peak144646656bytes. Worker and Terrain destroyed; all four owned RTV
  creation debts released. Three intentionally cached private programs remain.
- Token1521, sourceStart6128,6128, centre(-9360,-9360), PID19852.
  control.bin SHA256692023ea6768d5ec01b7fc9eabf5286d9b736577a2b9fbc6ca2b24ae5c411c52.
  Native peak127497856bytes. Worker and Terrain destroyed; four RTV debts released.
  Resource telemetry ends scene0/privateNames3/workspaces1. These short counters
  do not prove whole-game memory stability or unchanged FPS.

## Same-world result and limit

Authenticated GST windows and the actual production packed-material decoder are
compared with matching global checker parity. The examined old boundary has
four columns of changed geometry/normal values, expanded to six material columns
by the axial chroma decoder. First control differs from the old right-hand frame
by up to12.2285m and27.572degrees; over a million other examined interior samples
match material/normal bytes exactly, with millimetric radial-height roundoff.

The16px inset control does NOT fix the old right frame: its right-hand normals
match that old frame, while its left guard differs from the first control.
Its extended guard comparison reaches35.33m. Thus a fixed six-pixel blending or
sixteen-pixel crop is not an established correction. Differences follow native
world patch/neighbor refinement context, not merely a missing viewer texture.
The exact runtime patch IDs/stitching branch were not captured; do not present
the offline native neighbor-index audit as direct runtime branch tracing.

Native code evidence shows1152m minimum patches with65x65 vertices; coarser
2304/4608m patches have36/72m vertex spacing. Error scoring includes camera XYZ
and a height adjustment toward each patch. A finite universal halo is not
proved by the controls. A shared LOD-camera API requires valid owned viewports,
raw-pointer lifetime through worker join and a bounded global quality policy.
Uniform level7 raw vertices alone would be2768896000bytes, exceeding the
unchanged1GiB private native limit; no such implementation is approved.

Root reports/scripts: D:/DEV/Kenshi/research/gsm-interior-seams-20261102,
gsm-overlap-edge-20261103 and gsm-halo-native-20261103. Independent controlled
case1 review: gsm-overlap-case1-review-20261103. Current public map remains a
development feature; focus stays on actual detailed model/lighting coverage
without a fundamental terrain-render rewrite, as requested by the user.
