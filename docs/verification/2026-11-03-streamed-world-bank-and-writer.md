# Streamed world source bank and512-window output

The new staged source bank reuses83,885,956 GPU bytes across actual two- and
four-biome cells. It fits the shared192MiB budget only when all other live owners
are included. It does not silently raise the old90MiB diagnostic limits.
Independent root review/fresh tests are recorded in
`research/gsm-world-bank-review-20261103/report.md`.

The512-window GST2 writer preserves the existing16×8192 base tile format and48
coarse-plane completion bits, while allowing bounded small render results. It
writes only new incomplete files; successful base-plane completion is not atlas
publication. Data is flushed before its completion ledger. Cancellation and
partial IO invalidate the writer instead of creating a ready tile.

Independent review found a stale error after corrected `Begin` arguments; the
successful path now resets that status. Fresh final suite passed1,231,296checks
and14 compiled runtime mutants. Independent adversarial checks passed543,340each
optimized/unoptimized, including deterministic joined cross-thread cancellation.
Final writer manifest:`57b1de7c564fb567a11401ba04fa8de17a642818d94fa63d7852a150660eed38`.
Review:`research/gsm-window-writer-review-20261103/report.md`.

The writer is worker-only: a complete tile entails393,265WriteAt calls and817
flushes. Those operation counts are bounded but are not a throughput benchmark;
they must not run on the game's UI/render thread. A verified obsolete1,431,307,264
byte initial test file was removed after a path/reparse/header inventory and a
deletion manifest. Current actual-file tests use fresh unique names and cleanup.

These are implementation components, not a published mod release or completed
installed replacement. Current-source receipt validation, renderer scheduling,
fine-window residency and actual in-game image/performance checks remain required.
