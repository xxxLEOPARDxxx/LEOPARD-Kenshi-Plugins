# Private single-biome material reference, 2026-11-01

Scope: one512 raster of primary cell1,1, one real Okran's Pride biome
(`5611-Newwworld.mod`), twelve installed2048 DDS inputs. No final atlas, biome
mixing, painted colour/overlay, water, buildings or shadows are accepted here.
Best Minimap is unchanged. No public package is published.

## Native upload and accounting

The native D3D11Texture array writer (vtable7CE08 slot2D8, implementation4CAE0)
uploads six Image layers and sends one TextureManager resource-loaded credit.
Texture::_loadImages instead clamps to cube faces and is unsuitable. Our exact
ABI/profile, twelve same-read file hashes, aggregate encoded/decoded128MiB caps,
64-byte Image and96-byte external-buffer MemoryDataStream are independently
reviewed. No native callback retains the CPU images/streams.

The first installed candidate (`f11004...`, PID13688) freed array GPU buffers but
added37,749,854 bytes to manager accounting per request. The writer does not set
Resource's Loaded state; the texture destructor consequently bypasses
Resource::unload's manager debit. This is an accounting mismatch, not evidence
that those same bytes remained allocated physically.

Only an owned successfully uploaded array now receives the exact guarded
Resource::setToLoaded call once. It changes state without another credit.
Resource::load/reload, shared resources and global cache counters are untouched.
Unknown constructor/upload states quarantine one bounded set and disable capture.

## Installed candidate and repeated runtime proof

DLL SHA256 `b73ac11bfa0627181dc406d4b8ce964ef27745a9a8589cb684a4bde64e8d23d9`,
214016 bytes, startup export verified,152 native imports resolved. Full build
and production/source/HLSL tests pass; independent review found no blocker.

Game PID12568, start UTC2026-11-01T12:50:47.6323432Z, user-loaded paused global
map, file requests without focus changes. Both requests use50333424 encoded
and50331888 decoded source bytes, then free those CPU inputs.

| Request | Controller ms | TextureManager before/after | Material before/after | RTV final refs |
|---|---:|---:|---:|---|
|1503|1993|2189512166 /2189512166|14474953 /14474953|six times0|
|1504|1142|2189512166 /2189512166|14474953 /14474953|six times0|

Three deliberately cached private shader programs remain after the first request;
subsequent program bytes/counts are unchanged. Scene count0, one main workspace,
one closed/joined private native worker per request. Texture/material accounting
is balanced, without eviction. Both images were inspected; no nonfinite pixels.

Whole-process private bytes:8264462336 before first,8282836992 at later idle;
8282836992 before second,8538902528 immediate after,8373190656 at idle about10min
later. The process continues running other plugins and native game work. These
non-equivalent idle samples do not establish universal absence of retention;
only the identified accounting drift and owned RTV debts are proven repaired.

Evidence: game `_codex_work/global-satellite-20261101/rtv-lifetime-20261101/`
`512-biome-loaded-state-{first,second}/` and `biome-loaded-state-proof.json`.
Each directory contains three BMPs, request log/INI, memory samples, idle sample
and a direct PNG preview. Existing capture.json scope text says distant terrain
because the generic controller predates this reference; this report corrects
that label, without rewriting historical raw evidence.

## Image limitation and next inputs

Real detailed layers render, but neutral white colour and empty overlay produce
strong clipping (168296 of262144 pixels in the second exposure preview). This
must not be presented as the requested completed satellite map.

Read-only native research proves actual ZoneMapOverlay ctor A136E0, not the old
SDK A12E00. Zone width4608m, origin−147456; painted tiles span8zones=36864m.
For x,z in0..7, bounds are `(origin+36864*x-18, origin+36864*z-18,
origin+36864*(x+1)+18, origin+36864*(z+1)+18)`. Overlay and colour share these
36900m UV bounds with no V flip. The installed64 overlay1024RGBA and64
colour2048RGBA PNGs are all present. Colour alpha is nonconstant and affects
native diffuse alpha as well as RGB; preserve it.

A73728m primary cell covers four painted tiles. One stretched pair is wrong.
Next reference needs four bounded pairs/private materials selected by absolute
patch bounds, including boundary cases and exact18m borders. Base RGBA inputs
total80MiB before mipmaps/transient decode. Do not create shared ZoneMapOverlay
objects, mutate global ZoneMaps, or silently substitute missing modded inputs.
