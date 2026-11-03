# Independent world-native / logical-name integration review

Verdict: **CLEAR within the reviewed contract. No P1/P2 finding.** Reviewed
2026-11-03; no production, game, INI or installed files changed by this review.
No native contract DLL was loaded and no native game function was executed.

Final world-native manifest SHA256:
`41c3f923a000b3ead9d17e2ddd61010caaa9c67a45e5ddc9064c1adb517a1840`.
The independently reviewed production pins are:

| File | SHA256 |
| --- | --- |
| SourceFactory.h | b2d9fd7a185f590b2ad1a705f4588e31e564e16f47084beb6cea6353a37f3e7c |
| StageAdapter.h | 0554fd314cf6c4acb9c5942cfa062d66250d8a5bfcb4b8de8dbafb148a2d5937 |
| NativeProvider.h | 4c7268431302518edfa39292168ecf282ecf5b146cd988e84655872fc45ce144 |
| WorldStream.h | 7593fea4e012446173b0cd7ee20499905795b26b8f12c059c573d59e9abe6ff0 |
| flow_tests.cpp | 989fef7f1d78306ed33064d34ec1ba8ef26e81e4116c2223fef13c5644a10a1c |

The machine-readable `verification.json` contains unabridged exact pins. A final
metadata-only amendment added dependency scanning/session records to the runner
and a README paragraph. Its diff was reviewed; all production/test bodies and
borrowed files in the initial45-file mirror remained identical. The final native
freeze's10 owned files,935 dependencies,31 evidence entries, artifact hashes and
PE imports passed its verifier and independent direct hash checks. Independent
fresh binary import sets match the final manifest. Host binaries import no
Ogre/Kenshi module; the native DLL links the real SDK/import library only.

## Contract inspected

SourceFactory reconstructs the expected catalog plan once, admits at most four
selected SIDs and15 distinct roles per SID, and keeps encoded asset identity
separate from the original logical resource/group. PNG pins are region-specific
overlay/colour plus shared blend; DDS sources use actual upload-role assets,
including padded layers, rather than substituting deduplicated hash identities.
The exact `blendMap.png` spelling and explicit Overlaymaps/Landscape groups are
the new owned-input contract. No physical worker path is guessed by the factory.

Provider routing requires its owning thread, captured scope and exact factory
role/pin/name match before selecting a SID. Every stage binds all roles before
capture. Capture is terminal on failure and clears all60 token outputs, including
previously valid outputs. BeginBank requires exactly15 unique roles from one
selected SID; nested, mixed, incomplete and combined64-role transactions refuse.
Each transaction routes to one NativeMetadata owner, with balanced EndBank.

All four NativeStage members explicitly choose OwnedRawIndexedFile. This permits
a stable nonzero loading-listener identity as a new raw indexed-file source;
it does not claim the bytes returned by that listener or historical native GPU
contents. Refresh checks complete identity, original spelling, listener64-bit
identity and semantics. The borrowed NativeMetadata retains its profile/import/
vtable gates, exact-then-native-insensitive indexed winner, FileSystem-only
policy, SDK manager/group recursive locks and current source-receipt validation.

WorldStream retains a SourceFactory per Submit, clears it before admission and
on token refusal, and independently checks expected slot/input/scope before any
worker Open. A valid request for some other role is not thereby valid for the
current controller input. Source hashing and acceptance still use original
encoded pins; fresh logical names do not weaken the source-byte barrier.

## Fresh execution evidence

All execution used the isolated `mirror` under this research directory. Commands
were bundled Python `run_tests.py` in mirrored world-native and world-stream,
then `additional_tests.py`, `flow_refusals.py`, `finalize_review.py` here. Final
author metadata was read and verified without regenerating it.

* Native adapter suite:122,105 checks each under VC10/O2 and/Od; full64×16 catalog
  sweep,1,999 selected SID stages and29,985 requests. All four compiled mutants
  failed as expected. Actual native contract compile/link passed; never executed.
* Controller suite:12 lifecycle checks each O2/Od;393 actual Win32/WIC/WARP flow
  checks, three completed frames, exact frozen256 three-plane comparisons,
  persistent authenticated TIFF reuse, OFF drain and world-change cancellation.
* Independent extension:10,845 checks each O2/Od. Recomputed encoded hashes from
  original catalog hex bytes; exercised malformed roles/IDs/hashes/sizes/scopes/
  logical pairs across all60 roles, all four SID BeginBank paths, listener high
  bits/archive/path/semantics changes, and124 distinct Init/Bind/Capture refusal
  cutpoints. Every cutpoint reseeded60 previously valid tokens and confirmed all
  were withdrawn and the failed owner could not be retried.
* Independent actual controller extension:1,352 checks, including14 malformed
  token submissions. All refused while Idle with zero height authentications
  and zero GPU owners, then the unchanged real three-frame flow completed.

Check counts in polling controllers can vary with completion timing. No passing
count is interpreted as a native API or performance measurement.

## Limits and integration boundary

This approves the bounded adapter and logical-name handoff for the parent's next
controlled integration step. It is not blanket installed-game acceptance. Native
game Main wiring was not exercised by this review. The parent-reported live PNG
metadata capture is separate evidence and does not prove all selected DDS/PNG
sources for every cell. Missing/ZIP/manual/unknown indexed sources must continue
to refuse; no fallback to catalog physical paths is justified.

Keep provider/tokens on the creating Main thread through destruction, retain
provider memory outside small callback stacks and charge it to the CPU ledger.
NativeMetadata validation repeats bounded code/profile and index work during
explicit capture/accept/commit; its Main latency needs real measurements.
No per-frame world scan, new queue or growing cache was found. Long-run FPS,
all-world throughput, final finite/world-height acceptance and caller scheduling
remain outside this review. Existing streaming raw radial/three-plane semantics
and shared192MiB accounting are unchanged by the reviewed integration diff.
