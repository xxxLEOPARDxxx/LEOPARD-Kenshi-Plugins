# AI Fixed: Basusu GateFix/SleepFix integration, 2026-11-02

Current unpublished work; historical version 1 remains in README. No installation,
personal INI edit, publication, push or commit was performed by this work. The
game was initially absent; a later read-only process check found PID14656 using
`RE_Kenshi/Kenshi_x64.exe`. No UI/focus/process actions were taken.

## Provenance and actual evidence

Read the development/user instructions, Kenshi context and historical AI Fixed
rules. Historical commands/publication permissions were not current work.

Basusu upstream was cloned outside the mods and detached at
`edcd54f89002b69c702733f256bf8d1677f51f39` in
`D:/DEV/Kenshi/research/Basusu-kenshi-plugins-upstream-20261101`.
GPLv3 LICENSE was copied verbatim before adaptation. UPSTREAM.md records file
hashes, thanks and original/source links. No CorpseLoot/FurnaceSlots integration.

Sources read:

- [Pinned source](https://github.com/Basusu0/Basusu-kenshi-plugins/tree/edcd54f89002b69c702733f256bf8d1677f51f39).
- [GitHub issue #1](https://github.com/Basusu0/Basusu-kenshi-plugins/issues/1):
  healthy-looking workers sleeping, including starvation reports.
- [SleepFix](https://steamcommunity.com/sharedfiles/filedetails/?id=3787375792):
  all 47 public comments returned by the public comment endpoint.
- [GateFix](https://steamcommunity.com/sharedfiles/filedetails/?id=3787399736):
  all 11 public comments returned by the public comment endpoint.

Steam owner76561198023028341 comment render endpoints were read with count100;
response totals were47/11, no omitted page. Raw snapshots are ignored under
`tests/_generated/upstream`, not distributed. README links were placeholders;
actual Workshop IDs were independently resolved. Pinned source and the public
Steam DLL are not assumed identical: `ownBedsOnly` is absent in pinned SleepFix,
although comments report that setting. No public Steam binary was audited.

Confirmed source limitations: shared raw repair-bed cache of eight entries,
raw actor/bed registries, repair choice without a distance limit, and broad order
clearing in some transitions. Comments also describe contention while en route,
repeated cancellation, and difficulty completing a normal close. These support
the design questions, not a claim that every player saw the same root cause.

The existing AI Fixed SelfRepairOne from commit
`86642928f57ca18f0fb4a85b6809362652288adf` was substituted into the new extracted
fixture with the same native boundary fakes. It failed manual interruption,
repeated injured travel and live-disable cleanup scenarios. Current source
passes. Baseline mode is `test_lifetime_debug.py --baseline-repair`; failure is
expected. This is a production decision regression, not an in-game reproduction.

## Implemented boundary

One GUI-owned scheduler handles flesh sleep, robotic health and wear. It does not
add a second competing SleepFix loop, permanent job or medical-update hook.
New AutoSleep, RepairHealth and GatePassage modules default off. Existing
SelfRepair remains on. Thirteen new settings,18 total, are live; only Enabled
and complete old Debug diagnostics retain real MCM_RESTART requirements. Debug
is last/off; all42 MCM messages exist in compiled EN/RU/ZH catalogs.

Medical parts use SDK fields, robotic race/limb classification and missing-part
checks. Biological wounds wait for treatment unless self-healing. Sleep completes
from flesh; repair completes from robotic health plus wear. Thresholds are
separate, and repair transition respects the configured flesh wake threshold.
Repair completion uses the existing absolute criterion: raw flesh minus stun
within0.5HP of max*age*HPMult and wear<=0.5; derived percentage only triggers care.
High-HP/prosthetic/stun regressions protect against early ratio-based completion.
Healthy flesh alone does not trigger sleep. No hunger scale is guessed.
The typed native getHungerSpeedModifier<0.999 gives feeding priority for biological
actors: no new rest, owned rest yields, and the same bounded episode may resume
when the native penalty clears, provided intent and attempt budget permit.
Skeletons skip food checks. isFed() checks a different buffer and hunger KO is
too late; neither is substituted for the preventive native penalty.

Bed eligibility uses actual BF_BED/BF_SKELETON_BED, furniture and usable/free
slot, native and own reservations, same town/camp, ownership, floor and radius.
OwnBedsOnly=1 is enforced; optional allies exclude neutral/enemy owners. Repair
rate must be positive. The same eligibility helper rechecks the approach target,
including changed allied/neutral ownership, floor and operation permission.
Bed names never identify a bed. Geography/ownership are
rejected before native slot/blackboard/power work. No raw cached bed is retained.

Own orders require the unchanged input sequence, exact target/full hand, and
sole order. Manual travel, Stop/clearOrders and queued commands take precedence.
Cancellation suppresses this injury episode until its trigger clears. Only an
owned order is replaced; permanent jobs remain. Approach timeout60s, three
attempts, retry20s; no-progress5min ends our rest and suppresses the episode.
These deadlines now use native simulation seconds, excluding pauses; continuing
healing has no total-duration cutoff. GUI scan cadence remains wall-based3s. Native hunger penalty pauses our rest; these limits and offline tests do not
prove that food jobs always succeed or that an arbitrary AI mod will cooperate.

Gate assistance records only explicit ground moves, waits400ms input quiet plus
250ms stationary sampling, and checks a path crossing the actual door sides.
Only own gates are eligible. Actual DoorStuff leaf is both native destination
and subject, clear=false/shift=false. Once open, the original destination resumes.
Close/relock use normal native transitions after crossing/delay/clear8m area.
Manual order/lock/open-button changes cancel assistance. It may safely leave a
gate open/unlocked on cancellation, timeout, object loss or a failed close.
No teleport, forced-close fallback or own movement/pathfinding engine.

## Native API and hook compatibility

Independent read-only native audit:
`D:/DEV/Kenshi/research/AIFixed-Basusu-20261102/contract.md`, rerunnable with its
`audit_native.py`; `audit_nutrition.py` separately verifies the typed hunger getter.
Its Steam6442A0 body computes t=clamp(hunger-1,0,1), then thunk15B59 to9B0070
returns0.5+0.5*t. The tolerance0.999 is our policy, not a new raw hunger scale.
Actual installed KenshiLib export stubs and Steam/GOG tables
were used, not source symbols.asm slot numbering. Actual Steam executable SHA256:
`504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`.
GOG table bindings were checked; no GOG executable/gameplay claim is made.

The audit verifies Character move vtable318 through RTTI, `_NV_` hook alias,
Character versus OrdersReceiver bool ordering, leaf/full-hand subject semantics,
separate clearOrders path, inside/outside reference getters and normal door
open/close. No new guessed code addresses/class offsets appear in the module.
Direct GUI lockButton/openButton mutate lock fields without lock/unlock calls;
both buttons are observed too. Observational detours call originals once and
preserve signatures. Own command TLS restores through `__finally`, including
native faults; C++ exceptions retain normal unwind before native SEH containment.
Bed lookup native faults are caught inside reservation-copy lifetime and suppress
the injury episode, avoiding repeat attempts and leaked reservation nodes.

Gate Push Guard has a separate anti-push feature. Its door update/state,
movement update/teleport and shared lifecycle/world hooks are not replaced.
AI Fixed adds player-move/order/clear/button/lock observers. Shared world/load
chains still call their originals once. Runtime load-order/cooperation remains
a game acceptance check. Exact-name SleepFix.dll/GateFix.dll detection suppresses
the corresponding competing module; renamed originals are not discoverable by
this check and should not be combined.

## Performance and lifecycle

Rest scans every3s, max256 concurrent rest identities; existing episodes reserve
capacity even if the player list is reordered. Reservations are serial-bearing.
Input keys max1024, pruned against current player identities; overflow fails
closed. Gate queue/states max128, watched door keys only while referenced; eight
actors per250ms round robin. At most two local spatial queries per250ms globally,
and at most one search per actor per1s of simulation time. No input means no gate search.

**Native maxNumber is ignored by Kenshi.** It forwards INT_MAX internally.
Radius5..100m/default30m and frequency limit the scope; they are not hard bounds
on native scanning/allocation. Returned building lists>=128 are declined after
native work, and cached machine lists>4096 are declined after native collection.
Native/TLS lektor buffers retain their high-water allocation, as in existing
AI Fixed; no invented deallocator is used. A very dense custom base still needs
live CPU/memory measurement. No broad claim of zero leaks/FPS cost is made.

World load/new/reset/clear invalidates before original callbacks. GUI owns and
clears transient maps; external observers only write identity sequences under a
critical section. Slow heartbeat pauses access without forgetting order ownership.
No saved raw Character*/Building*/DoorStuff* registry, timers, background threads
or preview/readback work was added.

## Explicit player-report regression matrix

| Report | Verified offline boundary | Gameplay acceptance still required |
|---|---|---|
| Humans with prosthetics stop repair repeatedly | Flesh/robot health/wear separate; ordinary sleep ignores prosthetic completion; powered repair waits both | Actual mixed-race medical update, restored jobs |
| Bed search across continent | Configurable radius, same town, geography rejected before slot/power | Actual player's settlement layout/navigation |
| Fully healed workers return to sleep while travelling | Healthy flesh gives no sleep trigger; observed manual/Stop cancels episode; old production decision fails regression | Sustained travel with current mod stack |
| Turret operator remains in bed / remote enemy behind walls prevents rest | Own completed rest emits get-up; manual order wins | Remote-threat combat classifier is deliberately unchanged; no claim this report is solved |
| Standing/production workers need rest, then resume jobs | No manual order permits one assignment; permanent jobs never cleared | Native GOAP priority, food/job resumption |
| Barrel used as bed / floor / ownership error | Actual selector rejects wrong function/non-furniture/usable/floor and neutral/enemy ownership | The specific Smuggler's Bar asset, custom models/animations |
| ownBedsOnly=1 violated | Actual selector test excludes allies while enabled and excludes neutral/enemy always | Actual modded faction/ownership flags |
| Same-town/modded beds / NeedSleep interaction | Actual function plus usable checks, reservations, exact external-order preservation | NeedSleep may independently issue rest orders; actual mod combinations |
| Healthy workers starve sleeping | Healthy trigger rejected; native hunger penalty pauses owned rest; simulation-time no-progress yield guard | Actual food availability/path/AI cooperation and long fast-forwarded play; no universal starvation-fix claim |
| Gate opens/passes but never closes | Extracted runtime normal close, wait closed, original lock restoration, occupancy and manual cancellation | Actual path/door animation, many travellers, Gate Push Guard and both load orders |

## Verification snapshot

Commands use bundled Python and VC10/Windows SDK7.1; generated output is ignored.
They do not load/install the mod.

- `test_policies.py`: health/threshold/bed/intent/gate geometry/time boundaries.
- `test_bed_selection.py`: exact production selector and native-fault wrapper.
- `test_lifetime_debug.py`: exact production ownership/rest/state/lifecycle/MCM
  wrappers; native medical/world data are boundary fakes.
- `test_gate_runtime.py`: complete production GateRuntime and input observers;
  native order execution/spatial world are explicit fakes, not native GOAP proof.
- `test_settings_locales.py`: real MCM/defaults/restart flags and compiled3locales.
- Build: `D:/DEV/Kenshi/build.ps1 -SrcDir D:/DEV/Kenshi/ours/AIFixed -Name AIFixed`.
- Import audit: generic Container-Highlight `check_candidate_imports.py` against
  installed dependency exports, read-only.

Initial independent source/offline review (superseded candidate below): **CLEAR**, conditional on this offline
scope. The reviewer reran all five suites against this source snapshot:

| Check | Result |
|---|---|
| Health/bed/intent/gate policies |46PASS|
| Actual production bed selector/shared approach eligibility/native-fault wrapper |26PASS|
| Production lifecycle/debug/owned rest/nutrition pause |50PASS|
| Production gate runtime/input observers |27PASS|
| Actual MCM/defaults/three compiled catalogs |153PASS|
| Total |302PASS|
| Installed dependency import audit |205named imports,5modules,0missing|
| git diff --check |PASS|

Initial candidate (superseded by the clock correction): **174080bytes**, SHA256
`041628cbff293744ac8ff72181c7205f362df0240f0ec770f8a2d8063fdde230`.
Source AIFixed.cpp SHA256
`7029d50be287fea340e74a5da87e93a19d7d6d0dce22b474c59620b40c81781f`;
RestPolicy.h SHA256
`1774373e1477a06dc69fe169869b66377f92177305725a3877dda3ca544ec841`.

Review found two gaps in the first draft and verified their fixes: a percentage
could finish high-HP robotic repairs early, and approach validation omitted some
initial bed conditions. Absolute raw-health/stun completion and the shared full
eligibility helper now have targeted regressions. Native nutrition was separately
verified before using its getter; no guessed raw UI units were introduced.

At that review no source changed after the matching build; subsequent edits only
added selector tests and documentation. No rebuild/installation was done by the reviewer.
The additional root-review finding and revised candidate follow below.
Actual native GOAP, successful feeding, many travellers, custom furniture/races,
NeedSleep, both Gate Push Guard load orders, CPU/FPS and memory remain live
acceptance checks. These offline checks do not prove those outcomes.


## Rest deadline correction after root review

Root review found that the fixed ten-wall-minute cutoff could terminate healthy
progress even after the first offline CLEAR. The GUI also continued checking
GetTickCount deadlines during pause. New production regressions first failed
four cases: >10min progressing repair, twenty-minute steady healing, one-hour
paused GUI and the first resume. The fixed total cutoff is removed. A subsequent
regression exposed the old high-water comparison: healing after new damage was
ignored until the former peak. The comparison baseline now decreases on new
damage; only actual subsequent improvement refreshes the watchdog.

Read-only `audit_rest_clock.py` verifies the actual504b Steam image and installed
export/table bindings. Its ignored JSON records native bytes/disassembly:

- GameWorld::getTimeStamp slot3669 Steam25B4B0 is a CPerfTimer wall clock and is
  deliberately NOT used for simulation deadlines.
- getTimeStamp_inGameHours slot3673 Steam66C180 returns total native world hours
  from the engine's day/hour update. getTotalHours slot6611 SteamCCFB0 is the
  typed value getter; getLengthOfHourInRealSeconds slot3674 Steam66C960 returns
  the native conversion109.090911865 s/hour. It matches the reciprocal native
  hour advance rate0.0091666663 at normal speed.
- userPause Steam787470 zeros the same global speed that native world-hour
  update uses; the hour update writes the total hours returned by the getter.
  isPaused slot3628 SteamDEDC0 reads the native pause state.
- TimeOfDay::getRealLifeSeconds has an additional0.5 scaling in this executable,
  so the explicit native world-hour length is used instead. No guessed raw
  offsets or clock conversion constants are placed in production.

One GUI-owned clock samples these typed APIs only at the normal3s rest cadence.
It accumulates simulation milliseconds, skips all paused scans/commands, ignores
sampled paused intervals and the first resume gap, and reanchors on invalid or
rewound clocks without spending unknown time. ResetSessionState resets it with
world ownership. The first resume sample can conservatively omit one scan
interval; it cannot consume the paused interval. No extra hook, thread, actor
scan, allocation or per-frame native clock polling was added. Gate intent timers
are unchanged by this rest-only correction.

The 60s approach timeout,20s retry,5s get-up grace and5min no-progress watchdog
remain bounded in simulation time. A progressing recovery may continue beyond
ten/twenty minutes. Explicit tests retain stalled recovery and failed-approach
retirement after actual simulation advances. Fresh final results and candidate
identity are recorded after the new build below.

Rest-only follow-up verification (superseded by the gate correction below):

| Check | Result |
|---|---|
| Policies, including paused/native rewind/invalid/overflow/wrap clocks |57PASS|
| Actual bed selector |26PASS|
| Production lifecycle/rest/nutrition/long recovery/pause/stall/approach |56PASS|
| Gate runtime, unchanged |27PASS|
| Settings and compiled EN/RU/ZH |153PASS|
| Total |319PASS|
| Actual Steam native clock audit |PASS; GOG bindings table-only|
| Dependency imports |210named,5modules,0missing|
| git diff --check |PASS|

Rest-only candidate, now superseded: **175104bytes**, SHA256
`1087f79ccaa187cbd379e06a90c350ad21826ec8be65f198f3dba3af7df7f6e5`.
AIFixed.cpp SHA256
`769bf299af31f3c7d40fedeead0111b429d14036a06c1529b8216be7a57e0850`;
RestPolicy.h SHA256
`bc366effef287adf6b8420b7913b2caec92251bfcb25630ed4d3a0621619c8a7`.
This follow-up's independent review is **CLEAR under the rest-only offline scope**.
The reviewer independently reran all319 checks, the actual504b native clock audit,
210-import audit and diff check; the175104-byte/hash identity matched. No source
edit, rebuild or installation was performed by the reviewer. The unchanged gate
wall-clock/pause behavior was separately surfaced to root for a scope decision;
this CLEAR does not claim a gate-pause change or live gameplay acceptance.
No install/commit/push was performed. Live compatibility and CPU/memory acceptance
remain as listed above.


## Gate pause correction requested before release

The rest-only review explicitly surfaced the remaining gate wall timers. Root
requested a separate gate-only correction. The complete production GateRuntime,
actual GUI dispatcher and actual metadata-reset function are now extracted in
one native-boundary fixture. Before the fix it failed13 assertions across pending
input, owned OPEN/resume, post-crossing close/relock, invalid clock and world
reset. These are offline production-call-order regressions, not native GOAP proof.

Gate assistance now uses a separate GUI-owned SimulationClock with the same
already-audited native world-hour/getTotalHours/hour-length/isPaused APIs and
pause/invalid/rewind policy as rest. Input hooks queue serial identity, destination
and sample position only; they do not mix GetTickCount timestamps with active
scanner time. The scanner assigns creation/input/sample timestamps when consuming
an intent on a valid, unpaused clock. Every gate episode deadline, quiet interval,
search interval and close delay uses that same active simulation-time domain.

A paused/unavailable clock prevents GateScan, including automatic query, unlock,
open, movement resume, close and relock. Queued moves and existing bounded episodes
remain. First valid resume reanchors without spending the paused/unknown interval.
An existing OPEN can resume once after unpause; a failed OPEN still retires after
30 active simulation seconds. User/engine originals in observational hooks still
run once, including explicit door-button actions; the mod does not block those.

The250ms wall poll cadence and two-query budget per pass remain, so speed-up does
not increase the global8queries/wall-second ceiling. Quiet/search/delay values are
simulation durations and scale with simulation speed. With no queued move and no
episode, GateUpdate skips native clock reads entirely and drops its idle anchor.
No new feature, hook, world scan, thread or unbounded registry was introduced.
Clock state and all pending/owned metadata reset with world generation before
using a replacement world. Original GUI updates continue even during paused,
invalid-clock and world-not-ready cases.

Current targeted tests cover long paused input before episode creation, an owned
open order and one-time resume, retained crossing/close budget and deferred relock,
native clock fault and numeric NaN recovery, bounded failed opening after active
time, actual load/reset of clock/queue/episodes, stale-world readiness, unchanged
original GUI calls and idle zero-query/zero-clock polling. Fresh counts, matching
candidate and independent review follow below.

Final current verification:

| Check | Result |
|---|---|
| Policies/native clock math |57PASS|
| Actual bed selector |26PASS|
| Production lifecycle/rest/actual native-load gate-clock reset |57PASS|
| Complete gate runtime plus actual GUI/clock/reset integration |49PASS|
| Settings and compiled EN/RU/ZH |153PASS|
| Total |342PASS|
| Actual504b native clock audit |PASS; GOG bindings table-only|
| Dependency imports |210named,5modules,0missing|
| git diff --check |PASS|

Gate-pause candidate before the text-only correction:175616bytes, SHA256
`3d0b58002aa920f09e33ad9088ef5dfab4b64f960637219cc63988e60c4ed4b8`.
AIFixed.cpp SHA256
`abc18872cba7396191d713c665690441d322fe6c3a278ced743e328dedc4f1f1`;
GateRuntime.h SHA256
`3fa07c3c194a00cc42e6eeafc939c54ffdb578ae51e8c03a0b5bcc77b79502d3`.

Final independent source/offline review **CLEAR**. The reviewer reran all342
checks, native clock audit, import audit and diff check, and confirmed the exact
candidate/source hashes. The reviewer did not edit, rebuild, install or commit.
Native GOAP, successful feeding, mixed travellers, custom beds/races, NeedSleep,
both Gate Push Guard load orders and live CPU/FPS/memory remain acceptance limits;
no offline test claims those measured gameplay outcomes. No installation,
publication, package, commit or push was performed by this implementation work.

## MCM percentage explanation and shorter labels, same prepared v2

User MCM feedback found that RepairHealth did not explicitly explain its percentage
condition and the Russian threshold label clipped. Only three CPP Tr literals
and their corresponding PO entries in EN/RU/ZH were changed: toggle help, slider
label and slider help. The toggle now explains that any robotic part below the
health percentage set below starts repair, while wear is checked separately.
Slider help explicitly names the percentage of maximum part health.

Labels: EN `Robotic health threshold (%)`, RU `Порог здоровья робочастей (%)`,
ZH `机械生命值阈值（%）`. The RU label is29 characters instead of38; this is a
text-length check, not a new game-font pixel-width/UI screenshot claim. Actual
visual smoke belongs to the parent-controlled game check.

Reversing exactly those three literal substitutions reproduces the accepted
pre-edit CPP byte SHA256
`abc18872cba7396191d713c665690441d322fe6c3a278ced743e328dedc4f1f1`.
No behavior, default, hook, API, hot-path frequency or allocation ownership changed.
Fresh342 checks, the native clock audit, full build and210imports/5modules/0missing
all pass. Release glyph validation caught one new RU em dash before replacing
the archive; it was changed to ASCII hyphen. The153locale/settings checks and
canonical EN/RU PO+MO game-font glyph validation then passed with the final catalogs.

**Current candidate:** `D:/DEV/Kenshi/build/AIFixed/AIFixed.dll`,176128bytes,
SHA256 `686e5a2073045bc038924cf11371424ca48626c543f6e864055184ea04c01c23`.
CPP SHA256 `88ddb16c37f3f0caa8f54f0d2def1e429b17b13566c967de58c05ceb5ed3835b`.

| Locale | Final PO SHA256 | Final MO SHA256 |
|---|---|---|
|en_GB|`64221617a6a37cbc986e42db30820cfd469bc079f345fb69d0122079253cee30`|`79948f0792156bc24a9ce2e8ccbf0028540bae3be6c82162a7ef80bbe0cbb128`|
|ru_RU|`a63f03cb470c0c562b8c37e52541c8a0f9e71dd657294bfd284594c1fb5fcfeb`|`731848996004d117f345a7bbf08c61ca318ae96888e0ea361e44df141f65cf24`|
|zh_CN|`9d18f216a49cd4395f03240334aa8d4564e41de157d27d09d03ab047f8f9a59d`|`107b0a90782ea7651aa8bbc3306e7d281263318add6fded9cfe25f9f43e0c74f`|

The release owner was notified to refresh the same AI Fixed2 and combined package
proof. This text correction did not install, package, publish or commit anything.

## Parent installations and startup/UI smoke

After the review, root independently reran57 lifecycle checks,49 gate checks,
the native clock audit and inspected the final source/native-export boundary.
The reviewed behaviour candidate3d0b5800/175616B was installed after confirming Kenshi was closed,
with three compiled catalogs, GPL/provenance files and new factory defaults.
The personal INI was byte-identical at installation; its existing wear threshold
20 and Detailed log1 were preserved. New modules remained opt-in. The game later
expanded missing settings through MCM; that is separate from installer preservation.

Fresh process15696, startUTC2026-11-01T22:18:40 (see exact evidence timestamp),
loaded candidate3d0b5800. Startup logged diagnostics installed, then42 Russian
catalog strings, MCM description registration and the AIFixed page. Root checked
the loaded module path/SHA, responding process and absence of AI Fixed startup,
hook or conflict errors. This proves loading and MCM initialization only; it does
not upgrade the gameplay/feeding/gate/load-order/FPS/memory limitations above.

Historical evidence is archived in the release verification's content-addressed
startup record. The original installation backup includes the previous
DLL, catalogs, README, factory defaults and personal INI. Final README was copied
after release-note preparation.

The subsequent percentage-text correction installed candidate686e5a20/176128B
and three matching catalogs while Kenshi was closed, preserving the personal INI.
Its backup is game/_codex_work/aifixed-20261102/before-percent-clarification/.
Process29328 loaded this exact candidate and logged installed hooks without
AI Fixed errors. The user explicitly confirmed the percentage wording in MCM.
Root inspected the installed Russian catalog:42 messages, hash73184899 and the
correct new tooltip/short threshold label. The current log does not contain the
older MCM-page/42-string trace; user acceptance is recorded separately from logs.

Current evidence is the root `before-basusu-integration/startup-smoke.json` plus
`before-percent-clarification/installed-catalog.json`. The release proof archives
this record by hash and keeps the historical3d0b5800 record separate. Neither
startup/UI check establishes new automatic-module gameplay or FPS/memory proof.
