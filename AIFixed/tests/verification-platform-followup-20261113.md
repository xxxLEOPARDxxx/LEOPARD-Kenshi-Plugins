# Actual Lagoon failure followup, 13 November 2026

This supersedes the proposed recovery contract in
verification-platform-20261113.md. The owner tested its 9DF2 candidate and
reported that none of the Lagoon NPCs escaped. That candidate is not claimed
to have fixed this scene.

Current local candidate, not installed or published by this subtask:
`D:/DEV/Kenshi/build/AIFixed-platform-recovery/AIFixed-platform-recovery.dll`

SHA256: `DE4614D5133067A9261CCB8CDD1A0087E12F79974230709AF15C4E26B42509F2`

Size: 216064 bytes, built 04:29:56 MSK. Release number unchanged. Native VC10 x64 build PASS.
Full AIFixed suite PASS: 682 counted checks plus native rest-clock audit.
Counts: policies 217, cycle 20, trace 58, beds 26, lifecycle 58, gate 85,
settings/locales 171, production-extracted landing 32, actual INI loader 15.

## The actual blockers

The latest log has 70 observing lines, 909 actor/intent-ineligible lines and two
height-limit lines across all platform actors in the captured aggregate.
The two height-limit lines belong to one different actor at
(18217.70,401.02,48202.56), ground 54.90, identity
1/60/2965088000:12/1539184512. They are not any of the five target identities.

None of the five target identities reached a landing query. Their maximum
logged observation ages were:

| Target | Maximum game-hour age |
| --- | --- |
| Original guard 15/6 |1.446|
| Трутень 61/3 |2.383|
| Стражница 24/8 |0.917|
| Стражник 24/6 |0.550|
| Патч 29/16 |0.367|

The engine retained current MOVE_ON_FREE_WILL action 1 and a remote native
IDLE/FAILED path while clearing the high-level GOAP goal to NULL. Our additional
non-null-goal requirement discarded that actual movement evidence. Separately,
the one-hour witness lifetime deleted the physical watch, so a stationary
actor could never accumulate its four-hour physical window through those task gaps.
The final candidate also includes the owner-requested configurable delay below.

`fixtures/platform_run_20261113.json` retains 67 actual rate-limited target
observations, log hash and candidate hash. It is a sparse real trace, not a
complete frame/time reconstruction. The first three focused regressions failed
on the prior candidate before the corresponding implementation changes.

## Final evidence rule

Physical observation and movement admission are separate:

- Same exact actor/support and continuously sampled position retain the
  physical watch across task/action gaps. Real physical progress, missing
  samples, world/identity/support changes and medical/combat exclusions still
  require a new complete physical window.
- Current native MOVE_ON_FREE_WILL with a current remote
  FAILED/BROKEN/INCOMPLETE path is current blocked intent. Its high-level goal
  may be NULL, and GOAP may recreate the native action. The failure state need
  not change for four hours. No failure-edge or uninterrupted-action latch
  remains to starve these real attempts.
- Without a current move action, a retained failed path cannot refresh
  movement evidence or admit relocation. WAITING may use a failure within the
  preceding game hour. Reached/at-feet/no-action phases retain only physical
  observation. Final current movement/support revalidation remains mandatory.

Root review identified the earlier invented recreated-action/failure-edge
contract as overly strict. The corrected continuous-current-MOVE regression
failed before simplification and now passes. The suite also covers long
no-action gaps without relocation, current retries after those gaps, combat
requiring a new full configured window, real movement, reached/at-feet states,
missing limbs, KO, player exclusion and rejected landing geometry.

Changed production functions: Observation::Sample/SampleGuard, ReadGuardCycle,
Inspect, FindLanding, LoadSettings and MCM_Describe. History's obsolete failure
latch was removed; sampling/query budgets were not changed.

## Live wait control requested during this followup

UnstuckDelayHours is an integer MCM control from 1 to 4 game hours, step 1,
default 1. RecoverStuckNpc remains OFF by default. The new delay applies live
without MCM_RESTART; the existing recovery master still requires its hook to
be installed on restart. Old personal INI files missing the new key use 1.

Physical maturity is since+current selected delay, checked separately from
landing retry deadlines. Decreasing the setting may use already-observed
physical time; increasing it postpones maturity. Neither change resets the
physical anchor or bypasses an existing probe cooldown. INI is loaded only
through the existing settings callback, with no frame/translation polling.

The actual descriptor test and actual runtime 1/2/4-hour tests failed before
implementation, then passed. Policy tests cover default 1, the three waits,
live increase/decrease and exhausted-probe backoff. An actual LoadSettings
fixture uses a temporary absolute INI to prove missing-key fallback, clamping
and unchanged existing user key/value bytes. Native IniPath/default preparation
is an explicit boundary fake; the installed personal INI was not modified.

All three MO catalogs were freshly compiled with 47 non-header messages:

- en_GB: D9A64430BEC253539BE3B9423F0BC0BD18BD31D6552E6D08A580590369B87CCC
- ru_RU: 19B06B6941B2DE3578A298D2132EF7D29977E8C10D405DB9971587CAE7BF73F8
- zh_CN: 1B3B86B7B28C5932CD49618C99A5E47951F9C603AD1E70A983D081CADD952985

Location: AIFixed/locale/<language>/LC_MESSAGES/ai_fixed.mo. The compiled RU
catalog was checked for U+2010-U+2015 and U+2212; none occur in game strings.
Source PO files stay in source control; install only MO catalogs.

## Upper-platform terrain and bounded descent

`inspect_platform_terrain.py` verifies the current fullmap.tif size and SHA256
6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18, then
samples the independently audited U16 anti-diagonal terrain triangles at the
actual saved actor coordinates. Results:

| Target | Base ground, world units | Descent, metres |
| --- | --- | --- |
| Original guard |140.2364|11.0242|
| Трутень |140.2670|12.0481|
| Стражница |139.9680|15.9106|
| Стражник |139.9680|17.5067|
| Патч |139.9680|16.5997|

The three higher actors exceed the former 14 m bound. Root explicitly approved
an 18 m maximum only for the BF_FLUFF platform branch after matching physical
support and steep slope confirmation. Gate recovery remains 14 m and natural
rock/off-mesh recovery 8 m. Horizontal distance remains 6 m; minimum descent 5 m.
The actual production FindLanding fixture failed eight assertions before this
change, then passed the three actual upper-coordinate cases, exact 18 m boundary,
over-limit rejection, unchanged gate 14 m boundary and rock 8 m rejection.

Authenticated base terrain is not live navmesh or collision proof. Physical
slope, native exterior walkability, dry landing, same loaded zone, gate codes,
wall/closed-gate boundaries, standing clearance and final state checks all
remain mandatory. Their actual game query results must still be observed in
the next run. Offline tests do not establish FPS, CPU or absence of leaks.

No gate automation, other mods, private settings, saved games or process
memory were changed. No UI/focus/input was used. The fresh log was read while
the game was open; work continued offline once root confirmed it was closed.
No installation, commit, push or publication was performed by this subtask.
