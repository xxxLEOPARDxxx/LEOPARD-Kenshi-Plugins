# XP Info parity implementation, 30 October 2026

Scope: extend the existing SkillLevelUpInfo module; preserve its DLL, INI,
MCM ID, Steam 3829848328 and Nexus 2257 identities. The historical level-up-only
request is superseded. No installation/publication is part of this change.

Primary reference: XP Info 0.1 by semper1dem (Steam: Alexander),
https://www.nexusmods.com/kenshi/mods/2160 . Thanks to the original author for
the feature design. Original source distribution: the release archive on that
page (the author's description explicitly identifies GPLv3 and bundled source).
No source repository was found or invented. Local original binary:
`mods/xp_info/xp_info.dll`; original flat INI is a user configuration, not proof
of factory defaults. Source archive is not present in the searched local caches.
Implementation is independent, using the published behavior and read-only
binary inspection; no original binary or assets are redistributed.
Original DLL SHA-256: `7d76575c0e167e96d219a02c4ee069c92abb69f690b08d96f38cf6f2207bd302`.

| Capability | Original evidence | Implementation target |
| --- | --- | --- |
| XP text | `+%.0f%% %s`, actual positive increaseStat delta times 100 | Accumulate real positive base skill deltas, including tiny gains; threshold and interval per skill |
| Level-up text | `%s %d`, positive integer-level crossings | Immediate final reached level; crossing suppresses fractional text for that event |
| Merging | Per-character maps, four lines then `\n+%d more` | Separate gain and level labels, four lines then localized original remainder summary |
| Appearance | Level gold `(1,.84,.35)` medium; XP blue `(.62,.86,1)` small; native RS_NORMAL | Same colors/sizes/rise/fade; preserve accepted current model-relative placement |
| XP accumulation | Original RVA 13b0/1de0, 120-second stale pending expiry | Hook real increaseStat; native baseline polling fallback; no XP formula replacement |
| Stat coverage | Original RVA 15f0 indexes enums 1..38 except generic Weapons, checks CharStats bounds and deduplicates native refs | All 37 candidate enums; 34 distinct valid fields on Steam 1.0.65, including hidden MassCombat. HiveMedic/Vet alias Medic; Survival is a global dummy and excluded |
| Panel | getGUIDataForMainInfo hook, native progress rows, divider | Recent skills under vanilla character stats, native progress widgets, divider, adaptive capacity |
| Panel order | Stable slots; replacement only after old slot idle >5 seconds | Stable slots with same hold period and newest eligible skill replacement |
| Factory defaults | Original DLL data RVA 422f0: all 3 on; rows=2; minPercent=1; emit=1s; recent=120s | Same defaults for added settings; existing Enabled/SelectedOnly/LogToGame retained |
| Settings limits | Original load clamps rows 1..5, emit >=.1s, recent >=1s; minimum >=0 | User explicitly limited MCM/runtime rows to1..2 after native HUD acceptance; minimum0..100%; interval.1..60s; recent1..3600s (bounded UI) |
| Settings UI | Original Mods-list Settings button, no original hotkey | Same functionality through MCM, all changes immediate; no invented default binding |
| Diagnostics | verboseLog default off | Existing Debug default off, last; errors always |
| Saves | No save writes | Session state cleared on load/new game/settings changes; no sidecar |

Plan: (1) add portable tested gain/expiry/slot rules; (2) integrate a synchronized
native reference index and main-thread-only rendering; (3) add native panel rows
and lifecycle cleanup; (4) expose all original controls in MCM and RU/EN/CN;
(5) build isolated SkillLevelUpInfoParityCheck, run module regressions, check imports.

Implementation details and remaining acceptance checks:
- Pending popups flush every GUI frame, independently of the 250ms native
  baseline/index refresh. Configured 100ms collection time is honored; level-ups
  bypass fractional threshold/collection time on the next GUI update. GUI gaps
  do not reset pending XP or recent training. Session/settings epochs still reset.
- The accepted model-relative torso/head anchor remains, rather than reverting
  to the original fixed (0,2,0) offset. Game-owned rise/fade/colors/sizes match.
- MCM replaces the original separate settings window and Mods-row UI scan.
  Existing SelectedOnly/LogToGame/master switch remain compatible extensions.
- Capacity respects both the outer panel and its actual rendering parent, so a
  75px nested viewport inside a 300px window cannot receive two rows extending
  beyond its bounds. Completely full panels show zero rows instead of the
  original forced minimum of one. This remaining visual difference needs actual
  native geometry acceptance; it is not presented as an approved adaptation.
- All native/UI work remains on the GUI thread. Worker callbacks only touch
  locked scalar state through indexed references. The captured epoch before
  original increaseStat must match both current and registration epochs after
  it returns, rejecting an old call across load even when the same address is
  already registered in a new world. A separate stable registration token plus
  accumulator owner/stat index is captured before the original call under lock
  and must still match afterward. Unchanged refreshes keep their tokens. Calls
  that began unregistered cannot adopt a registration created during that call;
  same-epoch owner/address reuse also rejects the old callback.
  Load invalidates before native deserialization;
  new game and settings changes also invalidate. Departed/reused refs are erased
  only when the target accumulator still matches their registration.
- Current native getStatRef was checked in the unpacked game at
  `RE_Kenshi/Kenshi_x64.exe`, RVA 0x8832c0, jump table 0x8833fc. Do not use
  the packed root executable for this inspection. Compiler asserts the SDK
  CharStats size is 0x230, matching the original's bounds check.

Reproducible checks (bundled Python):
`tests/test_xp_core.py` (18), `tests/test_head_anchor.py` (19),
`tests/test_head_anchor.py xp_boundary_fixture.cpp` (19), `tests/test_panel.py`
(10), `tests/test_update.py` (12); combined runner `tests/run_tests.py`. Arithmetic and slot tests were
observed red before implementation. An in-flight callback spanning load failed
before the epoch fix and passed afterward. Native boundary fixtures execute
extracted production functions with narrow UI/game fakes, not duplicate logic.
The independent review's timing, GUI gap, registration reuse and nested viewport
probes became tracked source-extracted regressions: eight assertions failed on
the prior source; all twelve timing/lifecycle/registration assertions now pass.
`tests/inspect_native_stats.py` supplies the read-only unpacked ABI proof.
`tests/inspect_original.py` supplies optional original DLL evidence using
pefile/capstone installed only into ignored tests/_tools.

RU/EN/CN source MO catalogs are compiled locally and every active message is
checked by Python gettext; no new MO is installed. A legacy compiler's accidental
default scan rewrote 21 installed old catalogs before source locale changes;
all 21 were verified byte-for-byte equal to the latest workshop content copies,
with no differing or missing references. The module locale helper now imports
only pure compiler functions and writes module-local files.

Final local evidence, 30 October 2026:
- VC10/v100 C++03 x64 isolated build succeeded, `SkillLevelUpInfoParityCheck.dll`,
  137728 bytes, SHA-256
  `26e73c0d9a8d97b32c6209486c15ebcf07513faf4ff8faad87fa38b9bacc63a3`.
- 78 behavioral checks passed (18 core + 19 anchor + 19 native XP + 10 panel +
  12 GUI timing/lifecycle/registration).
- Both exports exist: `?startPlugin@@YAXXZ`, `MCM_Describe`.
- All 37 KenshiLib imports exist in official 0.5.0; all 15 imported MyGUI
  symbols exist in the installed engine DLL. No Ogre imports are emitted.
- 62 entries in each EN/RU/CN MO, all active UI messages verified with gettext.
- `git diff --check -- SkillLevelUpInfo` reports no whitespace errors.
- No source changes outside this module; no DLL/INI installation or publication,
  no commit/push (the parent coordinates shared repository changes).

Verification limits: automatic tests cover arithmetic/state/selection and native
boundary integration. Visual geometry, non-overlap with UI overhauls/LiveXpBars,
save/load and real game XP still require in-game acceptance. Do not report those
as runtime-verified merely because compilation passed.

### Native accepted baseline and visual alignment follow-up, 30 October

The user accepted candidate1487 fonts/divider. The saved read-only capture
`tests/_generated/runtime-25036-173531.json` has12 successful samples: capacity2,
two24px XP rows, divider[9,134,307,1], names matching vanilla PaintedMedium25,
and actual Top-aligned percentage clients StandardMedium21 with21px text size.
The game was closed before the next change; that PID is not inspected again.

The requested2px visual drop preserves name coordinates, outer24px progress
rectangles, colors, divider and adaptive1/2-row layout. Actual vanilla and Dark UI
`Kenshi_ProgressBarFill` templates use Stretch Empty(10,2,131,15) and
TrackPlace(11,3,129,13) inside150x20. At the observed153x24 rectangle these are
19/17px tall; moving their tops to4/5 preserves both heights inside the row.
Shrinking the outer progress widget would shrink these visible elements and is
therefore avoided. Percentages use y+2/h22 only when public actual text metrics
fit with safe padding; otherwise the existing text rectangle stays intact.

Native `getChildCount/getChildAt` follows WidgetClient and enumerates content,
not skin widgets. The official protected SDK `getSkinWidgetsByName` is invoked
through a legal derived-access/base-member pointer, only for the owned bar's
skin and once per live widget. Weak WidgetRefs invalidate destroyed widgets;
stable frames neither search/allocate nor repeat geometry writes. Exact known
layout checks protect custom/unknown skins. No production private offsets are
introduced. The read-only probe separately captures at most8 own skin widgets.

ABI proof uses the actual installed MyGUI export bodies plus production VC10
helper/assembly, without calling game objects: getSkinWidgetsByName RVA1c32c0
has RCX=this/RDX=hidden result/R8=name, initializes result pointers0/8/16 and
iterates skin vector3f8/400; findWidgets uses allocator member24. VC10 verifies
VectorWidgetPtr size32 and empty allocator size1; the assembly-only accessor
passes the same registers. This container contract is independent of the
previously discovered EditText header/native layout difference. Reproducible
evidence: `tests/test_progress_skin.py`, `tests/native_font_profile.cpp`, ignored
`tests/_generated/progress_skin_proof.json` and `native_skin_abi.asm`.

TDD caught the first content-child assumption by modelling actual skin/content
separately (RED), then passed with the skin API. New checks cover both physical
heights, percentage fit/fallback, stable zero writes/searches/measurements and
five/six-row capacity transitions. The new visual drop still requires user
acceptance after the parent installs its final candidate; accepted1487 geometry
does not constitute runtime proof of the later change.

Review follow-up: percent movement now requires the actual rendered Widget
client to share the logical top/full height, and public text region top0/full
logical height. Insets retain the24px percentage box while the bar stays lowered.
The live geometry guard is evaluated independently of the cached text size:
origin-only changes cannot inherit stale permission. RED/GREEN cases cover a3px
inset with21px glyphs and2px bottom padding, including100 stable frames without
22/24px oscillation, repeated writes or measurements. The captured1487 percentage
region[0,0,153,24] confirms the ordinary full-height zero-inset layout.
