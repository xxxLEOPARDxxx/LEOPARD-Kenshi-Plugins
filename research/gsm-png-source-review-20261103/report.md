# Independent pinned PNG source review

**CLEAR at final freeze `931297e052082cc0c604a4829f6190cb18b6cd1b54bc7ab395e4ec9e43c1edd4`; no remaining finding.** This reviews the worker-only PNG primitive, not Main integration, active source-winner qualification or installation readiness. No production file, game process, installed setting or UI was changed by this reviewer.

The initial freeze `307648a3d95f4efc4d6a717bc3f34e0cb6f5600d28aa1c2f677490922f5f4e6b` had two confirmed issues, reported before the author changed production:

- **P2, canonical pins across translation units:** `Side` compared the pin's address with a TU-local `static const RgbaPins` array. A genuine byte-identical canonical pin returned by a separately compiled bank TU failed with Side0/`E_INVALIDARG`. The independent two-TU test demonstrated this under both `/O2` and `/Od`. Final code compares the full canonical byte counts, decoded SHA, encoded SHA and logical path instead. Both independent cross-TU builds now return Side1024/`S_OK`; copied metadata with separately allocated equal strings also succeeds. Changed canonical fields refuse.
- **P3, cancellation during the final output SHA:** there was no cancellation observation after the potentially16MiB final hash. A thin wrapper around the actual hash signalled a real event after computing the final SHA; the old function returned `S_OK`. Final code checks that event before success and returns `ERROR_CANCELLED`. The original RED and corrected GREEN logs remain separate.

`fixed-diff.txt` retains the exact source/test/manifest changes; they match those fixes, the cross-TU test, fifth mutant and associated counts. The original and final sources are retained in separate immutable mirrors. Final19 original and19 mirrored closure pins, initial18 mirror pins and all final original frozen logs were rechecked unchanged after testing.

## Fresh tests

The final unmodified `run_tests.py` ran inside `mirror-fixed/GlobalSatelliteMap/atlas-terrain/png-source`: **660 checks across all129 real PNG images and five compiled runtime mutants PASS**. Every real image's WIC straight RGBA output matches the separately generated Pillow SHA pin. No native game DLL is called or loaded by these tests. Mutants must fail runtime assertions; compile failures and crashes are not accepted as detection.

Additional tests use actual production code, real WIC and real test-owned COM/event lifetimes:

- **86 alias/overflow/COM checks each under /O2 and /Od.** Exact adjacent input/output buffers work in both address orders; one-byte partial overlaps refuse without mutation. Pointer extent overflow rejects before dereference. STA initialization conflict leaves a reusable empty decoder; balanced MTA ownership permits a later STA; an external MTA count survives decoder initialization/clear. Duplicate Init, idempotent Clear and64 repeated factory lifetimes are exercised.
- **76 Init/WIC/hash fault cutpoints,388 assertions each under /O2 and /Od.** Thin HRESULT/hash wrappers inject failure before COM init, factory creation, stream initialization, decoder/frame/dimension/converter operations, each16-row copy and both hashes. Every failure surfaces, partial output remains unpublished, the decoder succeeds on retry, and clearing balances COM sufficiently to establish a fresh STA. These are actual API calls around the injected refusal, not a separate implementation.
- **200 cancellation observation points,812 assertions each under /O2 and /Od.** Actual1024/2048 image decodes use a real event signalled at each pre-hash, post-input-hash,16-row and final acceptance observation. Every case returns cancellation, preserves buffer guards and allows clean reuse. Partial destination pixels after copying starts are intentionally discarded rather than treated as valid output.
- **11 canonical-field checks each under /O2 and /Od.** Copied full metadata is accepted, while modified decoded size/SHA, encoded size/SHA, logical path, trailing SHA text or null metadata strings refuse without changing output.
- Independent logical-path omission and missing-final-hash-cancel mutants both fail runtime assertions. The final separate-bank-TU and post-hash event regressions also pass.

The first final two-TU harness attempt moved generated test sources below the review-build directory without adjusting their relative includes. That test harness path error was corrected to absolute mirrored header paths before the passing builds; it is not a production result.

## Code and ownership assessment

Only a complete canonical profile can select1024 or2048 dimensions. Both encoded and decoded byte counts are exact and bounded at16MiB. Range checks reject pointer wrap and any overlap before header access, hashing or output mutation. The authenticated encoded SHA fixes the exact PNG—including header/chunk metadata—before WIC sees it. WIC frame count and dimensions are checked again; explicit32bpp RGBA requests straight alpha without a color-management transformation. Final decoded SHA prevents acceptance of a different decoder result. No unpinned file or fabricated caller hash can qualify merely by assertion.

The decoder owns one WIC factory and one successful COM initialization count. Local stream/decoder/frame/converter RAII owners are released on failed HRESULTs, cancellation returns and normal completion. It opens no paths, owns neither input nor output blob, and retains neither buffer after return. Its WIC memory stream only exists during the call. Foreign Decode/Clear return before touching COM state; valid lifetime still requires creation, initialization, use, Clear/destruction on the same worker. Destroying the C++ object on a foreign thread would violate that contract: the helper refuses foreign release rather than providing apartment marshaling or a cleanup thread. Join and ownership transfer are integration responsibilities.

Cancellation is cooperative, not a bound on WIC/hash call latency. Input must remain immutable, and both buffers and the cancellation event must remain alive through the call. Failed decode/cancel after copying can leave partial bytes; the caller must require successful HRESULT plus current job/source admission before publication. The decoder's final cancellation check does not replace a Main-side epoch/job check when a worker result is eventually consumed.

## CPU and integration boundary

The header allocates neither blob. Caller-owned encoded and RGBA payloads can each reach16MiB; the caller must also account for retained geometry/height, staging, pending results and other owned CPU buffers. WIC/COM internal working allocations and latency are opaque. The README correctly avoids claiming a hard64MiB bound on every process/OS allocation. Keep initialization, hashing and decode on the bounded worker, outside UI/render callbacks. The full canonical Side comparison is fixed at129 profiles; no world scan, growing cache or per-frame INI/file polling is introduced.

The129 pins are immutable base-snapshot profiles, not evidence of current native resource winners. Source receipts, full current epoch/job validation, successful bank upload, complete residency accounting and final GPU/map acceptance remain separate requirements. These tests do not establish runtime FPS, long-run memory behavior or absence of every possible WIC/internal leak.

## Reproduction

`prepare.py` preserves the initial freeze and original RED regressions. `prepare_fixed.py` authenticates/copies the final freeze. Run the final mirrored `run_tests.py`, then set `PNG_REVIEW_FIXED=1` and run `run_cross.py`, `test_final_cancel.py`, `run_controls.py`, `test_faults.py`, `test_cancel_points.py`, and `test_canonical_fields.py`. Their final logs are under `review-build-fixed`; original regression logs remain under `review-build`.
