# GST2 window reader and GSI1 index verification — 2026-11-02

Scope: the new `AtlasWindowReader.h`, offline `atlas-cache/build_window_index.py`, and their dedicated tests. The viewer planner, renderer and integration are owned and verified separately. No recapture, image conversion, game calls, installation or publication is performed by these helpers/tests.

## Frozen interface

`AtlasView::ReadWindow(ReadRequest&, const WindowKey&, const unsigned char[3][32])` reads one source window. `WindowKey.x/z` are 256-texel window-grid indices, with tile 0–15 and source level 0–5. Success sets the request's upload level to 5 (256 square) and primary tile coordinates. The source level remains in the key. The three buffers total exactly 1,048,576 bytes (RGBA16F material, RGBA8 normal/coverage, R32F height).

`AtlasView::WindowIndex::Hashes(key, hashes)` exposes only a successfully verified index. `LoadWindowIndex(path, captureSha, headerSha[16], indexSha, control, index)` validates the complete 2,097,664-byte GSI1 file: fixed 1,024-byte header, reserved bytes, capture pin, sixteen GST2 header pins, whole index SHA256 and 21,840 nonzero three-plane hash records. Record order is tile, source level, window Z, window X. Header and record layout match the root-owned `AtlasWindowPolicy.h`.

Both readers reject noncanonical/local-drive-invalid paths, ADS, control/NUL characters, reparse leaves/ancestors and physical-path aliases. Handles permit readers only, denying write/delete while open. Cancellation is checked before work and at each ≤64 KiB chunk/row; partial output is discarded. A window revalidates the pinned complete GST2 header, exact file size and each selected plane's window SHA256.

## Reproduction

Run in PowerShell:

```powershell
& D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-view/run_window_reader_tests.ps1
& D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-cache/run_window_index_tests.ps1
```

Fresh results: **92 C++ checks and eight Python fixture suites PASS; eight functional mutants produce the expected assertion failures**. The C++ runner uses VC10/Windows SDK 7.1 and C++03 production code. Its actual sparse GST2 file has the production 1,431,307,264-byte layout, eight distinct windows across all six levels, real SHA256, Win32 sharing, a real junction, cancellation, malformed headers/indexes and source corruption. The sparse source's unused whole-plane digest fields are fixture pins; whole-plane verification belongs to the Python builder tests.

Python compact fixtures retain all 16 tiles, six levels and 21,840 records. An independent crop/hash oracle checks every record. Other tests cover whole-plane corruption, capture pins, missing tiles, stale control files, preservation of a different existing index, actual denied input writes, cancellation, atomic ready-publication failure and safe idempotent retry.

Compiled reader mutants: wrong source level, omitted window SHA, omitted index SHA, wrong record mapping and retained partial buffers. Builder mutants: omitted whole-plane SHA, transposed window records and omitted capture pin. Each must fail its named functional assertion; compile/import errors do not count as successful negative controls. Logs are under the respective ignored `_generated/window-reader` and `_generated/window-index-tests` directories.

## Offline production command and transaction

```powershell
& C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-cache/build_window_index.py <complete-atlas-directory>
```

The CLI accepts production geometry only. It holds the complete ready/capture files and all sixteen GST2 sources against writes/deletion. One streaming pass verifies all 288 whole-plane SHA256 digests while constructing window digests. It writes/fsyncs/verifies a new index, then atomically adds `windowIndexSha256` to `ready.json`. GST2 bytes are never modified. A different existing index is refused. Failure before ready publication preserves the old ready file; a complete unadvertised identical index can be verified/adopted on retry. Only control files created by this invocation are removed.

Root separately reported the real 22,900,916,224-byte cache pass: **116.89 seconds, all 288 planes verified**, index 2,097,664 bytes, SHA256 `3e935e8a26de0fba2e1e1f934fbee20256c69adb4f55530dedccf5f2d393b357`. Root's actual C++ index load and eight real-cache windows across levels 0/1/2/4/5 passed cross-language SHA checks. These are integration-owner results, distinct from this subtask's fresh fixture runs.

## Performance and lifetime limits

There are no game, Ogre or D3D calls, world scans or frame callbacks here. The builder keeps an approximately 2 MiB index, one source row (at most 64 KiB), bounded hash states and 16 source handles; it does not expand the atlas. The runtime index is approximately 2 MiB; one window uses 1 MiB of output plus row/header/hash state. Handles and hash providers are scoped, and failed reads release partial vectors. The owning viewer must use one cancellable background job, generation checks and bounded GPU slots.

Current window reads issue 768 strided row reads, which can be slow on compressed NTFS. Root measured about 250–343 ms at source level 0, about 250 ms at levels 1/2, 172 ms at level 4 and 78 ms at level 5 on the actual cache. These are worker-read measurements, not frame-time/FPS or universal latency guarantees. No stripe-read expansion is included. No gameplay, live zoom visual quality, FPS or long-session leak claim follows from these checks.

Frozen SHA256:

- `AtlasWindowReader.h`: `fbc72c909af7125380a7c6e09a400bfc5f456a1562d0dd06a69d3f5fdc5ed6ca`
- `build_window_index.py`: `15efdf0ae568c9d27ed4e3d1493d9b7ef8f36444d02b6bfb1c3a9218a360e839`
