# DE461 actual game failure and narrow followup

The owner tested DE461 and reported that the Lagoon guards did not escape.
The latest process was already closed when this followup inspected it; only
the fresh log, saves and game assets were read. No UI/focus/input was used.

Prepared candidate, not installed or published by this subtask:
`D:/DEV/Kenshi/build/AIFixed-platform-recovery/AIFixed-platform-recovery.dll`

- SHA256: `944F0C5D84F5D586938C05F2F998489F30A1A66D85485539F3E68D65FF3D5A3C`.
- Size: 216064 bytes; native VC10 x64 build PASS at 05:28:57 MSK.
- Full suite: 693 counted checks PASS, plus native rest-clock audit.
  Counts: policies 217, cycle 20, runtime 59, beds 26, lifetime 58, gate 85,
  settings/locales 171, landing 42, actual INI loader 15.
- The three compiled catalogs still contain 47 messages each. No player-facing
  strings or MCM/default settings changed in this followup.

## Exact observed original-guard failure

Actor 1/15/98510744:6/3684570368 stayed at
(18773.75,254.39,47727.40) in all five logged observations. Its current goal
and native body action were both 36, which is PATROL_TOWN in the actual SDK.
Its native state was IDLE with remote FAILED/WAITING navigation.

ReadGuardCycle admitted only generic MOVE_ON_FREE_WILL action 1, so this actual
native patrol action entered ordinary travel recovery instead. That branch
had no fortification support and used the 8 m rock/off-mesh limit. Four actual
probes failed terrain-height-limit at ground 140.12: the guard's descent is
about 11.43 m, not an excessive platform descent. The four probe ages were 1.100,
2.017, 2.933 and 3.850 game hours; the configured 1 h wait was therefore working.

ReadGuardCycle now admits this specific native PATROL_TOWN movement action
alongside generic MOVE. The remaining current blocked native path, exact
support, medical, timer, collision/face-proof, landing and final revalidation
requirements are unchanged. The actual-action/position regression failed on
DE461 production before the fix and passes now. Its native landing result is
explicitly faked; this is not a claim of actual in-game release.

## Observed Drone failure and independent proof path

Actor 1/61/2046386688:3/2518066432 stayed at
(18780.78,260.48,47723.61) in all seven logged observations. Three mature probes
entered platform recovery correctly but returned support-not-steep. Native
ground was 140.17, so the required descent is about 12.03 m. The old diagnostic
did not retain the rejected footing normal and never queried a current nearby
walkable face; neither result is known for this actor.

The slope threshold is not relaxed. Following root's approved design, a
non-steep platform may instead qualify only if the native closest-walkable-face
query at the actor explicitly returns 0 (miss). Success 1 and unavailable -1
reject it. The old cached navigation face alone cannot establish this proof.

This fallback retains the ordinary 14 m supporting-structure limit; 18 m remains
exclusive to physically confirmed steep support. Minimum 5 m descent, 6 m horizontal
limit, dry and native-walkable landing, same loaded zone, gate side, barriers,
full standing clearance and final current movement/support checks remain.
It does not admit a no-action/reached/at-feet NPC merely because it is idle.

The source slope is now captured before rejection, and sourceFace is logged:
-2 means not queried,0 means explicit miss,1 success,-1 unavailable. The extra
nearest-face query runs only inside an already budgeted mature landing probe;
no new character/world scan or polling frequency was added.

The production landing-flow fixture first failed five assertions before this
fallback, then passed explicit-miss acceptance, valid-face and unavailable
rejection, retained 14 m cap, and mandatory landing clearance. Other old target
identities moved or were absent in this scene, so their previous locations are
not presented as current stationary failures.

## Evidence and limits

`fixtures/platform_de461_run.json` freezes 16 actual sparse target observations
with the full source log hash and DE461 candidate hash. Sparse diagnostics do
not reconstruct every frame. The source player collision XML and static base
placement were also inspected, but were not used to invent a current transformed
footing normal. No external messages, memory writes, settings edits, gate
automation edits, installation, commit, push or publication were performed.

Current production changes are limited to UnstuckRuntime.h: ReadGuardCycle,
FindLanding and the LandingTrace/TracePlatform diagnostics. Root source review
was clear before final build; independent review is requested on this exact
frozen candidate. The actual original-guard footing and Drone source-face
result still require native runtime validation. No FPS or leak-free claim is
made from offline tests.
