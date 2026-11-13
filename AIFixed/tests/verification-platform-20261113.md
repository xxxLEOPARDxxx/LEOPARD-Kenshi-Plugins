# Platform recovery followup, 13 November 2026

Status at handoff: reviewed local diagnostic candidate, not installed or
published. Release number unchanged. The game remained closed throughout.

Candidate:
`D:/DEV/Kenshi/build/AIFixed-platform-diagnostic/AIFixed-platform-diagnostic.dll`

- VC10 x64 build: PASS, 215552 bytes, 13 November 2026 03:32 MSK.
- SHA256: `9DF2E06A71695BAE3200226FD09FEEFEB9399EFD88A9459AD2C4593F9EBBA6BA`.
- Installed DLL before handoff remained
  `F571E038F01A947176B97F941E450FF3EBDA74667185ADB57725BAEC7BC82C6E`.
- Full AIFixed offline suite: 599 counted checks PASS, plus native rest-clock
  audit PASS. Counts: policies 176, cycle 20, trace 48, beds 26, lifecycle 57,
  gate 85, settings/locales 164, actual landing flow 23.
- `git diff --check -- AIFixed`: no whitespace errors; existing CRLF conversion
  notice for tests/README.md only.

## Findings and change

The old BF_FLUFF watch rejected bed/shopping/other goals and immediately lost
the four-hour physical episode. It also counted one unchanged native failure
as fresh on every sample. Current code ties the episode to the exact actor and
support identity across goal changes. Only a current native MOVE_ON_FREE_WILL
action under a non-null/non-idle goal, with remote blocked navigation, may
start an attempt or admit relocation. Once a failure has been acknowledged,
GOAP cancellation/action recreation cannot acknowledge it again; a native
non-failure path phase must first intervene.

The production-extracted mixed-goal tests first failed six checks. A separate
independent review then found the cancellation freshness loophole; its exact
constant-native-FAILED regression failed before the latch correction and now
passes. Root and Minimap-agent core reviews found no remaining blocker.

The four-hour window, 0.5 m physical anchor tolerance, one-game-hour recent-failure
requirement and medical/combat/player exclusions remain. Geometry predicates
remain unchanged: steep physical support, dry same-zone landing, 6 m horizontal
limit, 5-14 m descent, clearance and wall/closed-gate checks. Final action/support
revalidation remains mandatory.

## Actual saved observations and remaining uncertainty

`inspect_saved_platform.py` fully consumed v15 platoon records from four saves
without opening a process or changing a save: autosave0 / 48 files,
autosave2 / 47 files, quicksave / 47 files, autosave1 / 38 files.
`fixtures/platform_saved_20261113.json` freezes ten actual observations from
the two latest saves with source hashes, timestamps and full actor handles.

The original guard and four nearby elevated candidates have exactly equal
saved positions between 01:44:38 and 01:54:39 MSK. The four candidates are:

| Saved name | Container/index/serial | Saved height |
| --- | --- | --- |
| Трутень |61/3/2518066432|260.7485|
| Стражница |24/8/3292846592|299.0742|
| Стражник |24/6/3422119168|315.0350|
| Патч |29/16/2482788096|305.9645|

These are inferred candidates from position/proximity, not independently
confirmed screenshot identities. Saved medical flags show no death/coma/KO,
carrying or furniture occupation for these candidates. Endpoint equality does
not prove uninterrupted four-game-hour immobility. The saves do not provide
current goals/actions/native paths, support handles, terrain or collision data.

If terrain at the higher candidates were 140 world units, the three heights 299-315
would exceed the existing 14 m descent limit. Actual FindLanding extraction
verifies that conditional rejection before exterior search; it does not claim
terrain at those positions was measured. Its lower-guard positive fixture
requires every explicit native physical query fake to confirm a safe exit.

The latest log contains only one recovery, actor 41/48/3319702272 near 26469/49034.
Saved identity identifies it as Дикий бык. Its latest saved position is 26344.44,
140.19, 49019.31, about 12.9 m from that stuck point after the logged 3 m nudge.
That is evidence of later movement, not confirmed immediate return into the
same obstruction. It is not the Lagoon guard.

## Focused next run

Detailed log now records every sampled BF_FLUFF actor, including ineligible
ones: full actor/support identity, current position and anchor, observation and
failure ages, goal/action, native state/path and raw native goal delta, attempt,
fresh-failure witness, probe site and exact rejection reason. Probe results add
terrain, radius, slope and candidate landing. Ordinary observation lines are
limited to one per actor per 15 seconds and five/second globally; forced probe
lines share the existing at-most-two-probes/second budget. Debug off returns
before native diagnostic queries. No new collision or spatial queries were
added for logging; history/diagnostics remain bounded and main-thread owned.

One user run with the joint candidate installation should distinguish missing
native witnesses, physical-anchor resets, rejected support slopes, excessive
height, missing exterior points, clearance, water, gate-code and barrier
rejections. Do not enlarge the landing limits without that physical evidence.
No FPS, CPU, leak-free or actual Lagoon-release claim follows from offline
tests. The source replay's order and game clock remain explicitly synthetic.

No gate automation code, other mods, settings, publication files or personal
changelog was changed by this subtask. Game/_codex_work size measured at
handoff: 4558 files / 1025436119 bytes; no new files were written there or deleted.
