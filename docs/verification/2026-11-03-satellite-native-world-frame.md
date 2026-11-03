# Native deterministic-terrain frame checks

Private integration run in Kenshi1.0.65 on NVIDIA RTX4070TiSUPER, 03.11.2026
(Moscow). The ordinary viewer was disabled only for the controlled producer
budget. The installed working atlas/viewer and saves were not replaced.

## Actual game evidence

| Token | Process | Controlled view | Elapsed | Result |
| --- | --- | --- | --- | --- |
|1703|27004|512x512,1m,centre(-13824,-13824)|3.054s|accepted/exported|
|1704|27004|same view|0.880s|payload bit-identical to1703|
|1706|27004|map closed after the request was armed|0.165s|E_ABORT, no final file, no quarantine|
|1707|27004|reopened map, same view|0.901s|payload bit-identical to1703|
|1708|27448|centre(-13568,-13568), same resolution|0.903s|accepted/exported|

Token1705 was not armed before the map closed and is **not** cancellation
evidence. These times cover private frame processing, not world rendering or
FPS. Full exports and both game logs remain local in
`game/_codex_work/global-satellite-20261103/world-frame-native` and the private
probe output folders. No generated world atlas/ready manifest was published.

The first export SHA256 is
`337e7fddb8c3f8defb610c62a69b1b7aebe64ccdce26e8de144ad22cbaa5e290`;
repeat1704 is
`c2509bc0d9b726e865e45a872d034460ededc410860accf692cc62f40c8c9a1c`.
The file header includes process/request identity, so payload equality excludes
the176-byte header. Each complete export is4,194,480B.

All262,144 first-frame heights independently match the authenticated source
TIFF's18m anti-diagonal triangles within0.00005902267014334939m in the committed
double-precision checker (the earlier float-rounded check reported0.000030517578125m).
All FP16 material
and R32 height pixels are finite. Native normal alpha is reserved0, **not** GST2
coverage; a separately reviewed adapter converts accepted coverage to255.

The256x256 equal-world overlap has bit-identical heights. Material is not
bit-identical: maximum linear RGB difference0.000732421875, gloss0.001220703125;
gamma2.2 display differs by at most1/255 in57 pixels. Normal differs in18
components by at most1LSB. This is evidence against a height discontinuity in
these two windows, not a proof of all world/material boundaries or zero color
seams. Reproduce the value checks with
`tools/diagnostics/satellite-capture-probe/check_native_world_frames.py`.

## Installed code and cleanup

Case0 probe was built from the reviewed one-window caller. The second launch
uses commit2f6efa6's narrow case1 offset, private probe DLL SHA256
`2d1304749e06c365ab8b705e5c493fb2a6d9c35d6bbcbb5032b05438d8f48ba6`.
Frozen WorldFrameControl.h remains
`a237c23b6fe20a57641e8b6e28b9cf34f4930ed672bb5df4083abe0211526ce2`.
The previous DLL and original personal INIs are retained in
`_codex_work/global-satellite-20261103/before-world-frame-0758`.

After the second game was normally closed, both personal INIs were restored
byte-for-byte: probe
`f2e5bb6d9c626c8caed2216d52499fb66c45f0aa8eb6044aa7444fb878cefdc4`,
viewer`63d3ca9d5db3013724288fd70eacad35367df25e4e449a6012ff675ead6156f8`.
The controlled WorldFrameControl flag is absent again. The normal working
viewer DLL is unchanged.

## Remaining acceptance

The real original-source receipt, owned source uploads, fixed geometry,
accepted pixels, worker export, repeated capture and map-close cancellation
have now run together in the game. Remaining work includes cohort/native
scheduling, accepted-to-GST2 worker writing, whole-world completion/mips,
bounded1m/3m detail caching and viewer integration. Long-session memory/FPS,
world/save/device transitions and all biome edges are not established by this
small controlled run. Development is not a released update.
