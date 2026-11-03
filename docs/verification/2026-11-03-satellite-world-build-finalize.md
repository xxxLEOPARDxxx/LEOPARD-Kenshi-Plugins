# Closed WorldBuild candidate finalization

2026-11-03. Implementation only in `tools/diagnostics/satellite-capture-probe/finalize_world_build.py` and its focused test. No game input, INI, installation, production atlas processing, activation or publication performed by this task.

## API

```
python -B finalize_world_build.py inspect ABS_CANDIDATE
python -B finalize_world_build.py finalize ABS_CANDIDATE --allowed-parent ABS_PARENT --capture-receipt ABS_FROZEN_GSCAP2_JSON --completion-receipt ABS_CLOSED_JSON
```

`inspect` reads present `.gst.partial` headers only, marks `ready:false`, permits an incomplete subset, and creates nothing. `finalize` requires a candidate that is an immediate child of the explicitly named existing parent; every path/ancestor is checked for reparse points through the existing Windows index helpers. Exactly sixteen canonical partials must be the entire candidate contents. Any extra files, mixed/previous finalization state, ready/index or control marker refuse before mutation.

The external completion receipt must explicitly contain typed values: `format=GSM_WORLD_BUILD_COMPLETION`, `version=1`, `directory=ABS_CANDIDATE`, `expectedWindows=4096`, `windows=4096`, `error=0`, `quarantined=false`, `writerJoined=true`, `producerJoined=true`, and `captureSha256=SHA256(exact frozen GSCAP2 bytes)`. Missing values, bool-as-int and coercions refuse. The caller must derive that receipt from actual observed producer/writer completion; this script does not prove game state merely by reading supplied JSON.

Capture metadata has the canonical GSCAP2 grid/side/origin/span and exact source rows; top-level engine/mods/height pins must agree with those rows, and the region catalogue digest must equal the compiled header. `capture_identity.verify` rehashes the actual exact current enabled source set twice, before and after input preflight. Receipts are bounded to64KiB with duplicate JSON keys rejected and reread before mutation.

## Validation and ownership

All sixteen files are sharing-locked through preflight: canonical names/coordinates, exact file length/layout/formats, flags0 only, complete48-bit base block ledger, stable physical file identity/header. The base scan reads32-row stripes and independently rejects every nonfinite material/gloss or world height and every coverage byte other than255. Signed/HDR material is preserved, not clamped to LDR. It also records each base plane digest for forensic provenance.

After all checks, a new-only `.world-build-finalizing.json` freezes receipts and preflight rows. Locks close for same-directory no-replace renames; each path/file identity/header is rechecked immediately before its rename. Frozen exact capture bytes are written new-only as `capture.json`. Existing semantic `build_mips.build_atlas` and `build_window_index.build_index` process linear flags0 data, never packed/YCoCg. Final ready capture/index pins and the actual2,097,664B GSI digest are rechecked. A new-only `.world-build-finalized.json` records success with `activated:false`.

Partial failures preserve forensic candidate/marker state and refuse automatic rerun. The script neither deletes nor overwrites existing atlas directories. It never moves a candidate into the viewer's `captures` directory. Existing mip/index internals may replace their own newly created readiness metadata as designed.

This is a **trusted local, closed-candidate offline workflow**. Reparse/physical-path checks and sharing locks are enforced, but closing locks for path-based rename and the existing path-based mip builder do not establish immunity to a hostile simultaneous namespace mutation. Do not claim atomic live publication. Keep the viewer disabled or game closed for final activation; the root owns that separate verified cutover.

## Evidence

- Initial supplied stub failed completion control as expected (`completion receipt not implemented`). Expanded missing-helper controls were RED before implementation; compact complete-pipeline control was RED before adding the tested production phase helper.
- Final `python -B test_finalize_world_build.py`: **8 tests pass**,0.734s. Log: `tools/diagnostics/satellite-capture-probe/_generated/finalize-tests.log`.
- Actual compact64x64/six-level/16-file production-helper pass: generated mip chain, all288 whole-plane index hashes,21,840 indexed windows, ready capture/index pin checks and refusal of a second run. Compact payload16-file total~1.4MiB. Fixture source verification checks real temporary source bytes; it does not invoke actual game/mod source discovery.
- Fault controls: bad flag in last of sixteen files leaves all partials unchanged; nonfinite material/worldY and coverage hole reject read-only; incomplete header inspection never advertises completion; malformed/missing completion fields, duplicate JSON fields, metadata cap, stale/foreign/mixed candidate files refuse.
- Temporary fixtures are removed by `TemporaryDirectory`; production data untouched. `git diff --check` on owned paths is clean.
- Tests establish the helpers and compact real finalization path, not production throughput, an actual4096-window render, or arbitrary crash/race immunity.

Frozen code SHA256: `8499ff932db04632e0c62972ca8092030a35ae20168d90dc0101abdbef3c38a9`.

Frozen tests SHA256: `acc56243de2fa8ac1ebc7eb49674173e884b171db7155fc7a70b010dc934b1a5`.

Production costs remain:16GiB base semantic reads before mutation,~58.64GiB clean mip reads,~5.33GiB mip writes,~21.33GiB index verification reads; no extra atlas copy for same-directory partial renames. The striping is bounded, but no process RSS/performance claim was measured here.
