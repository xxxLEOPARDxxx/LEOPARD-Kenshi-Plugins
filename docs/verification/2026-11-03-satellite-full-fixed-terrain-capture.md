# Complete fixed-grid terrain capture, 2026-11-03

This records a completed private game capture and offline atlas validation.
It is not a claim of completed fine-viewer integration or final visual QA.

## Native capture

The installed private probe DLL SHA-256 was
`972d37b5357b51ce137e60a24ef01af2c7aaee82707fdc21459170ff152afff0`.
The paused `SAT-render-proof-20261101` scene ran request 1906 with the normal
satellite viewer disabled during generation. Autosave was temporarily disabled
after request 1905 was safely cancelled by a save. Request 1906 completed:

```
world build cohorts=1024/1024 windows=4096
world build token=1906 result=2 hr=0x0 quarantined=0 windows=4096 expected=4096
```

All 64 native regional parameter/distortion hash pairs were recorded. All 221
frozen physical source identities were checked unchanged before receipt
construction and again at finalization. The completed capture metadata is
61,447 bytes, SHA-256
`a516c376cc960ce36e9e3835c4df673b5a746a774f3ac69cca2ae46255aada08`.
Linear HDR material/gloss, normal/coverage and world-height planes remain in
their native atlas formats; no packed-colour conversion was applied.

## Offline finalization

The reviewed `finalize_world_build.py` completed in 364.36 seconds, in place,
after the actual successful terminal and known writer/producer join. Sixteen
8192-square tiles were checked for full base coverage and finite material and
height data; their mip levels were built with the aligned filter. All 288
plane hashes were verified. The window index covers 21,840 records.

- `ready.json`: `4ce3c66eac164bc25d5fa7e26c61e85a4bde0c2b7be089d791e25c63ea8a2179`
- `windows.gsi`: `3c93d9e420bece50e008fad82c66e42ab3019b94bcdedc6e406ecdf373863d79`
- Sixteen tile files: 22,900,916,224 bytes total. Index: 2,097,664 bytes.

The new directory was moved into the installed viewer's `captures` path while
Kenshi was closed. One previously working atlas remains under
`captures-before-world1906` pending visual acceptance. Moves did not make a
second copy of the new dataset. The probe's loader JSON was set to an empty
Plugins array without changing `mods.cfg` or its captured source identity.
The user's viewer settings were restored byte-for-byte; autosave was restored
to enabled with its existing 10-minute interval.

## Resource observation and cleanup

100 process samples spanned 13:15:49–13:32:22 Moscow during and after capture.
The first sample had 8,527,740,928 private bytes and 14,830 handles; the last
had 8,287,170,560 private bytes and 14,796 handles. This is a short observation,
not evidence that every plugin is leak-free or an FPS comparison.

Manifested cleanup removed eight closed failed/cancelled output files and 161
obsolete October test-build object files: 11,984,689,897 logical bytes total.
The measured remaining game `_codex_work` was 874,601,135 bytes. Active outputs,
saves, source, personal settings, compact evidence and required rollback files
were retained. Deletion inventories and the actual native log remain locally
under `_codex_work/global-satellite-20261103`.

Remaining acceptance work: inspect seams in the installed viewer, exercise
fine refinement at maximum zoom, verify layers and close/reopen/load resource
lifetimes, and measure the resulting game behaviour.
