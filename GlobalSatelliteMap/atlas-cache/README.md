# GST2 relightable terrain cache

This directory owns only the file format, bounded C++03 Win32 base writer and offline
NumPy mip/finalization tool. It performs no game/process/UI work. It does not implement
models, lights, shadows, viewer widgets, captures or a new stored tile grid.
The final base atlas is exactly16 logical `.gst` files, one per4x4 cell,8192 square with
six levels8192,4096,2048,1024,512,256. No temporary microtile image files are created.

## Fixed binary ABI

All integers and IEEE754 values are little endian. Row0 is minimum world Z;
column0 is minimum world X. Tile coordinates are0..3. The geographic world origin,
73728m cell span and source/mod identity belong to the capture/integration manifest;
this writer does not infer them from live game objects.

| Offset | Bytes | Meaning |
|---|---:|---|
|0|4|ASCII `GST2`|
|4|4|version2|
|8|4|header bytes1024|
|12|4|flags:0 decoded incomplete;2 RAW_PACKED incomplete;6 RAW_PACKED plus in-place DECODING;1 complete decoded only|
|16,20|4each|tile X,Z|
|24,28,32,36|4each|grid4, base8192, levels6, planes3|
|40|4|capture block side2048|
|44|4|reserved zero|
|48|8|base-written mask: bit `plane*16 + blockZ*4 + blockX`; full `0x0000ffffffffffff`|
|56,60,64|4each|actual DXGI IDs10,28,41|
|68..127|60|reserved zero|
|128..991|864|18 descriptors, level-major then plane-major,48 bytes each|
|992..1023|32|reserved zero|

Descriptor fields: offset uint64 at+0, length uint64 at+8, raw SHA256 bytes at+16.
Every level stores three whole planar images consecutively, no padding/gaps/overlap.
The first plane starts at1024. Descriptor ranges must equal the canonical computed
range, not merely fall inside the file. COMPLETE headers require a full base mask and
all18 hashes. File length must be exactly1,431,307,264 bytes; all16 total22,900,916,224
bytes (about21.328GiB). Header and each plane have their own digest in the ready contract.

| Plane | Native format ID | Bytes/texel | Semantics |
|---|---:|---:|---|
|0|10 `R16G16B16A16_FLOAT`|8|decoded **linear** base RGB and gloss A, little-endian binary16|
|1|28 `R8G8B8A8_UNORM`|4|world normal XYZ encoded `n*0.5+0.5`, explicit coverage A|
|2|41 `R32_FLOAT`|4|absolute world height Y in metres, little-endian float32|

Material is not sRGB and must not be clipped to0..1: actual native decoded albedo exceeds1.
No tone mapping, exposure or gamma is baked. No new alpha meaning is assigned to gloss.
Normal coverage is independent of material alpha. Invalid/nonfinite material or height,
or coverage0, canonicalizes to material0, encoded up `(128,255,128)`, coverage0, height0.
The caller must supply world-transformed normal/height inputs. The writer copies plane
bytes exactly; packed material conversion is the offline stage described below.

`Begin(...,true)` is the native capture mode. Its material slot remains8 bytes per texel,
but contains the original native BGRA8 packed32 in its first4 bytes and zero in the last4.
No independent2048-block decode is performed, because that would clamp real neighbor
samples at every capture-block border. `Begin(...,false)` retains the already decoded
binary16 path for direct inputs/fixtures. Readers accept flags EXACTLY1, never2/6.

## Capture integration

Include `Gst2Writer.h`; no new linked library beyond Win32 is needed. One `Win32File`
and `Writer` per actively written file; the file object must outlive its writer.

```cpp
Gst2::Win32File file;
Gst2::Writer writer;
if (!file.OpenNew(path) || !writer.Begin(file, tileX, tileZ, true /*rawPacked*/)) FailCapture();
// Repeat16 aligned2048-square blocks for each plane. Existing files are never overwritten.
if (!writer.WriteBlock(plane, pixelX, pixelZ, packedPlane,
                       Gst2::BlockBytes(plane), Gst2::BlockPitch(plane))) FailCapture();
if (!writer.BaseComplete() || !file.Close()) FailCapture();
```

`pixelX/Z` are0,2048,4096,6144. Plane0 block is32MiB/pitch16384; planes1/2 each16MiB/
pitch8192. Exactly one complete tightly packed plane per call; no caller buffer is copied
or retained. Validation rejects overlap/duplicate slots, invalid coordinates/plane,
wrong length/pitch, and arithmetic out-of-range before I/O. Writer stack header is1024B;
the48-bit ledger is constant storage. No full tile allocation/mapping or worker/thread
is created. The writer is single-threaded and owns no borrowed game pointer.

Each block is2048 random-access row writes. It flushes data, writes its8-byte ledger only
after all rows succeed, then flushes ledger. Any I/O failure poisons that writer and
refuses retry; a partially written block never earns a success bit. The file remains
incomplete. Input validation errors do not poison because they perform no I/O.
Root should abort that capture, preserve diagnostic evidence, and choose a new output
directory rather than reuse uncertain native output. `Close` is explicit and checked;
RAII is the final fallback. Open is CREATE_NEW with no sharing; builder cannot consume
a file while native capture still owns it. Preallocation sets file length, it does not
guarantee subsequent disk writes will succeed. Reserve disk space before capture.

Source identity is a separate capture manifest, as agreed with the root integration.
No secrets or personal INI are part of these cache files.

## Offline builder and publication

Run the bundled Python (NumPy installed) after all16 base files are closed:

```powershell
& 'C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe' `
  'D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-cache/build_mips.py' 'ABSOLUTE/OUTPUT/DIRECTORY'
```

The builder validates all16 canonical names, exact sizes/header/ranges/tile IDs and full
base ledgers before modifying any file. A small exclusive `.gst2-build.lock` prevents
concurrent builders. All16 raw flags must agree. For RAW_PACKED inputs it first caches
all original4 edge strips of all16 files (2MiB at production dimensions), checking the
zero padding while streaming32-row stripes. It then flushes flags6 (RAW2|DECODING4) to
ALL16 headers before any overwrite. Conversion uses native four axial neighbors,
float32 Y difference threshold30/255, right-neighbor fallback, global checker parity,
and true neighboring primary-file edges. A carried original previous row plus the next
original row avoids sampling half-floats already written by the preceding stripe.
Only the true outer atlas edge clamps. All16 decoded planes are flushed before all16
flags clear to0. Any interrupted in-place decode leaves flags6 and requires full
recapture: original pixels cannot safely be reconstructed, and no resume is claimed.
Mixed raw/decoded inputs or nonzero raw padding refuse before conversion.

The decoder follows actual `shaders/GBufferDecode.hlsl`, SHA256
`5f6804f56ba7fd77514ed9464d16cc57e233a621e860c037baad4a70d93d7401`.
Its independent scalar float32 reference tests are CPU evidence, not new GPU/native
render parity. The root owns source/native capture parity and live acceptance.

It canonicalizes invalid base pixels in bounded32-row stripes,
then generates each next level in16-output-row NumPy stripes; no Python per-pixel loops
or whole-plane NumPy maps. Every output uses the global aligned2x2 box. Coverage is the
arithmetic coverage mean (UNORM8 nearest half up). RGB/gloss and height use valid-coverage
weighted means; material returns binary16 without gamma/clamp; height sums in float64
to avoid overflow of finite float32 extremes. World normals decode, weight, normalize;
zero/cancelled result uses up. Coverage rounding to0 invalidates the output height/material.
Canonical invalid coverage is written/flushed before replacing NaN/Inf material/height,
so an interruption cannot erase the only invalid marker and resurrect coverage on resume.

All tile dimensions/origins remain powers-of-two aligned, so no2x2 generation footprint
straddles an internal cell edge at any of the six levels. Tests assemble the actual16-cell
source into one atlas and prove every generated tile exactly equals the cropped global
semantic reference at each level. There is no independently clamped edge approximation.
**Runtime bilinear filtering across file/tile boundaries is a separate neighbor/gutter
contract; these generation tests do not establish runtime seam correctness.**

After generation, each plane is streamed once through SHA256 in1MiB chunks. The digest-
bearing header is written/flushed with COMPLETE0; only then is the4-byte COMPLETE flag
written/flushed and its header read back/validated. Already completed files in a partially
finalized directory must pass all plane hashes before resumption. A corrupted complete
file is refused, never silently rebuilt. An existing `ready.json` is immutable/refused.

After every file is complete, a <=64KiB `ready.json` is flushed and atomically renamed
within the same Windows directory, refusing replacement. Only this manifest publishes
the atlas; complete individual headers without it are not a ready atlas. Each tile entry
has `x,z,file,bytes,headerSha256`; all plane digests are in the hashed header. No new file
or screenshot is published on failure. A crash can leave a lock/temp control file; inspect
that unfinished directory before explicit recovery. No old ready atlas is overwritten.

Approximate I/O for a clean build: base canonical scan16GiB, changed invalid base regions
only rewritten, mip reads about21.31GiB and writes5.328GiB, final hashes21.328GiB. The base
writer itself writes16GiB. RAW conversion additionally reads8GiB for the edge cache,
reads8GiB to decode and rewrites8GiB in the existing material slots. No raw microtile
files or full-image arrays are created. This is explicit offline disk work, not a per-frame
task. The shape-specific traced NumPy peak for a full-width8192 semantic source stripe
is12,225,136 bytes; raw decode plus the complete edge cache peaks26,348,036 bytes;
this measures algorithm allocations, not whole Python/runtime/game RSS or GPU memory.
Native capture buffers are separate (root admission target128MiB after half-float revision).

## Verification

`run_writer_tests.ps1`: real VC10 x64 `/MD /O2 /W4`, production Writer against a recording
I/O boundary for exact offsets/mask/all48 blocks/invalid bounds/overlap/partial write/flush
failure, plus an actual tiny Win32 CREATE_NEW/seek/write/read/flush/close/no-overwrite smoke.
`test_cache.py`: strict binary header and range attacks, semantic alpha/HDR/gamma/NaN/
normal cancellation/extreme height, full16-cell global cropped reference at all fixture
mips, missing/duplicate regions, corrupt SHA, interrupted finalize/publication and resume,
raw primary/capture-block/rolling stripe border correctness, explicit decode-recapture
refusal, invalid-marker persistence, actual8192-wide bounded stripes. `test_mutations.py`
rejects five in-memory production mutations: primary clamp, local parity, converted
previous row, erased invalidity and clipped native HDR. Reduced fixture dimensions are explicit library-test
arguments only; the CLI/native writer always enforce production8192/6/2048.

Observed RED: missing writer header/Python module before implementation; old material
UNORM8 path failed HDR retention before binary16 correction. All current tests pass.
This is offline/source verification; no full22.9GB real capture/build or live UI acceptance
has been performed by this worker. No source outside `atlas-cache` was edited, installed
or committed here.
