# One-frame seam control

This diagnostic is not a new atlas and never writes to GlobalSatelliteMap/captures.
Set `OverlapControl=1`, `AtlasCapture=0`, `AtlasBlocks=1`, `DiagnosticSide=2048`,
`IsolatedTerrain=1`, and use one new file-request token. Keep automatic requests
off. The module must already be loaded, the same world valid and the map open.

The fixed view is primary(1,1), material region(3,3), centre(-18432,-9216),
span18432, altitude16000, native detail32/32/0, fade0. Its 2048 square samples
at9m match old source columns5120..7167, rows6144..8191 exactly, with the old
x6144 capture edge now INTERNAL to one render. No simulation zone is activated.

Output goes into a new `overlap-<token>` folder beside the diagnostic module;
an existing folder refuses the request. It contains only `control.bin`:

| Offset | Type | Meaning |
| --- | --- | --- |
|0|8 bytes|`GSOV1\r\n\0`|
|8|LE uint32|2048|
|12,16|LE float32|world minimumX=-27648,Z=-18432|
|20|LE float32|sample step9|
|24|8 bytes|reserved zero|
|32|2048² LE uint32|native encoded material BGRA, before YCoCg decode|
|16777248|2048² LE uint32|normal/coverage, same channel conversion as GST|
|33554464|2048² LE float32|finite world height|

Total50331680bytes (48MiB+32). The existing worker validates ALL radial samples
before opening the file. No GST writer, sparse8192 primary, mips, GSI index or
ready marker is created. Host source buffers remain3×16MiB, with no extra32MiB
expanded material allocation. Normal worker join, cancellation, bounded warmup,
RTV cleanup and quarantine rules are shared with atlas capture. Disk admission
requires128MiB free including reserve, rather than the whole-world23GiB gate.

Interpretation: compare equal-world material/normal/height against authenticated
old windows. A changed tessellation pattern only in the common frame supports
the native per-pass LOD/morph hypothesis. Do not rerender the world or blur seams
on that hypothesis alone. This control does not prove runtime viewer correctness
at every mip boundary and is not a performance/leak acceptance test.
