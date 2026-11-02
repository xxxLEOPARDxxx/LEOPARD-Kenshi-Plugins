# Cached town file worker (2026-11-02, integration pending)

`GsmCachedTownJob.h` is a value-only file reader for the existing single `GsmAtlas::Job` worker. It does not import/render a mesh, query a town, access a scene, load an Ogre resource or retain native/COM pointers.

API:

```cpp
GsmCachedTown::Request request; // unsigned townId; float storedXYZ[3]
GsmCachedTown::Result result;   // state, owned bytes, sha256[32], BlobInfo, winError, error[192]
State Read(const std::string& gameRoot, const Request&, Result&,
           CancelFn, void* context, ChunkFn afterChunk = NULL);
```

Only the trusted canonical local game root and unsigned ID influence the path: `data/meshes/distant/distant_<ID>.mesh`. There is no filename/path parameter for a caller-selected asset. Positions are checked finite and otherwise left untouched. Limits alias the existing private mesh parser's **512 KiB** cap; reads are **at most 64 KiB**, directly into one bounded vector. No directory/world enumeration or second file-sized buffer is used. The existing Job's manifest/window metadata allocation is unchanged; this is the additional mesh-payload bound, not a whole-process memory claim.

Every ancestor must be a directory without a reparse attribute. The file is opened for read with **FILE_SHARE_READ only** and OPEN_REPARSE_POINT; disk-file type, attributes, normalized final DOS path, size and identity are checked. SHA256 covers exactly the locked bytes. Identity/size/last-write time and final path are rechecked after reading. Only then does the existing strict `PrivateTownLod::Parser` supply counts, signature and six local bounds. Partial parser metadata, bytes and digest are wiped on refusal/cancellation. The file/crypto handles retire before acceptance. A failed close refuses the result. A blocked synchronous OS read can still take time; cancellation is checked between bounded chunks, not claimed instantaneous.

`Job` adds initialized `cachedTown`, `cachedTownRequest`, `cachedTownResult`. `Run` refuses any mixture with terrain/water/model validation. Cached jobs never run ReadWindow/WindowMinimum or replace the terrain manifest. Existing `Cancelled(job)` / generation observer is checked before and through chunks; the progress callback records bytes. Main must still perform its existing generation/current-job check on completion, because a world change can occur after any worker completes.

States / caller contract:

- `Ready`: `job.ok=true`, immutable owned bytes + SHA + parser metadata. Native profile/shader verification remains **separate**, through existing model validation before any import.
- `Unavailable`: missing file/folder, `job.ok=true`, empty payload/error text, Win32 missing code retained. This is an optional unavailable asset, not a terrain error.
- `Refused`: bad path/type/reparse/sharing/size/hash/read/coherence/format or contained exception, `job.ok=false`; empty payload and bounded technical error.
- `Cancelled`: empty discarded payload, `job.ok=false`; completion must be ignored by the current-generation guard.

The root's Main Poll must handle this control job **before** its generic terrain failure branch and release the job on every outcome. Main owns nearest-town selection, retry/negative-cache bounds, native import/cached packet ownership and scheduling priority; this helper adds no queue/thread or automatic retry. No hardcoded Hub digest or source-mesh installation is present.

Verification (VC10 x64 `/O2`, actual helper + actual full Job dispatcher, no native game calls):

- Observed initial RED: new worker header/API absent. Then **61 checks PASS** against actual Hub ID2 and a second actual cached mesh ID10, using owned temporary edge fixtures.
- Hub source read only: `distant_2.mesh`, 108,057 bytes, SHA256 `fe7c73fdb428d86b6b5cf0cb25ede23372080f1342b389eaefc360fa9d0b3b28`. This hash is evidence, never an acceptance constant.
- Actual file SHA/parser metadata, valid changed byte payload with fresh SHA, max chunk size, missing unsigned ID, malformed/empty/oversized source, nonfinite copied position, invalid root/alias, real directory junction, existing-writer refusal, writer denial during read, pre/post final-path checks, short reads, metadata changes, close/callback failures, cancellation and all four mode combinations are exercised. Win32 failure shims are limited to owned fixture calls; normal reads/sharing/junction behavior use the real filesystem.
- **Nine compiled functional mutants rejected:** expanded cap, allowed writer, omitted path checks, omitted parser, delayed cancellation, retained refused payload, skipped cached dispatch, mixed worker modes, ignored generation.
- Existing model worker **28 checks + four mutants PASS** and water worker/window-minimum **45 checks + four mutants PASS** after the fourth mode was added. Only the old three-flag source-contract/mutant needle was extended; assertions were retained.

Commands: bundled Python `tests/cached_town_job_tests.py` and `tests/cached_town_job_mutations.py`; optional `--town-id 10` for the positive runner. Logs/compiled fixture outputs are in `build/cached-town-job-tests`. No plugin build, game/UI/settings/install, DLL candidate or commit was performed by the worker author. Root Main integration, full native build and independent review remain separate acceptance steps.

Frozen production hashes: helper `37c5db90ee5356d7990915fe52f6f735127b863187966067631c6965699882db`; Job header `8cf1836122bf8137ea54b8aee68c9539c9ef88e252e4b25d1d04c4c72c89a903`.
