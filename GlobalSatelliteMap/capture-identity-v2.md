# GST2 capture identity

`capture.json` is immutable metadata beside all16 GST2 files. The producer writes
it before full capture and re-verifies every declared source after capture.
The mip builder pins its exact bytes in `ready.captureMetadataSha256` and refuses
changes during finalization. Runtime validates that pin, then all local inputs in
one cancellable file worker. No per-frame file/INI/world scan.

Required JSON fields: `format:"GSCAP2"`, integer `version:2`, `grid:4`,
`baseSide:8192`, `origin:[-147456,-147456]`, `span:294912`,
`enginePath`, `engineSha256`, `modsCfgPath`, `modsCfgSha256`, `heightPath`,
`heightSha256`, `sourceManifestSha256`, and `sources` containing at most512
objects with absolute canonical local-drive `path` and lowercase `sha256`.
Each dedicated path/hash must occur identically in sources. No UNC, reparse
points, aliases, missing sources or changed bytes. Metadata <=65536 bytes;
aggregate hashing <=8GiB,64KiB cancellation intervals. Engine is the actual
RE_Kenshi/Kenshi_x64.exe, not the different launcher executable in game root.

Source manifest hash equals the compiled region catalog manifest. That catalog
pins219 terrain DDS and128 painted PNG records; runtime separately verifies those
actual local assets under game root, plus global blendmap. Active FCS mod names
resolve to local mods and Workshop candidates; retain all same-name candidates
rather than silently guess an ambiguous native override. This is conservative:
updating an unused duplicate can require regeneration.

This identity describes the terrain/mod profile, not a specific saved-game hash.
The terrain base can be reused on save reload under the same profile. World/load
generation still retires owned widgets/resources and pending work. Models,
player buildings, local lights and their unsaved state are separate layers;
this identity must not claim they were captured by the terrain-only generator.
