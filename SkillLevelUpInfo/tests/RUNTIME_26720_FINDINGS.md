# Native observation after candidate872b, 30 October2026

No gameplay code/DLL/INI changed for this investigation. Exact authorized
PID26720 only; OpenProcess rights QUERY_INFORMATION|VM_READ. No other process
search, game/native function call, injection or write occurred.

Header verification was corrected only after a1024-byte comparison proved the
ImageBase field contained the actual module base. Installed and candidate disk
SHA both872b5526f1e3fd074a21431414dc7b814d0afbbead05efb3dcbf553078db3c1b;
timestamp0x6ae4814c matches the linker map; SizeOfImage0x27000 matches inventory.
Four changed bytes0x14a..0x14d are all within ImageBase at0x148. All other1024
header bytes match. Nine offline probe tests pass, including rejecting a wrong
ImageBase, size and timestamp. Raw evidence is retained in `_generated`.

The subsequent10-second read produced40 samples,39 successful,258834 bytes;
one transient invalid read range occurred while native row addresses changed.
The process was not frozen or locked. Every successful sample had stable
before/after panel/category/owner identity. One panel/category/owner was observed
at4Hz; this cannot exclude calls to other contexts between samples.

Artifact: `_generated/runtime-26720-151926.json`.

## Confirmed geometry

Current panel0x929ccc58, category6. Root and actual row parent both320x185.
Six ordinary visible rows, each height26:

| Visual top | Bottom |
|---:|---:|
|5|31|
|31|57|
|57|83|
|83|109|
|109|135|
|135|161|

Both ordinary name/value boxes have left9 and width307. Native map order is
not visual order (Dexterity precedes Strength in the captured key traversal).
There were no native progress rows. All39 successful samples recorded:

- g_diagBottom161, parentHeight185, width307, valueRight316.
- g_diagCapacity0, g_diagRows0, status `no room for XP rows`.
- g_slots.count0, allIDs=-1; rowSkill cache all=-1.

Current capacity formula gives floor((185-(161+6))/26)=0. With five vanilla
rows at this same actual26-pixel pitch, bottom135 leaves44 pixels after the
divider gap, so capacity is floor(44/26)=1. This explains zero rows in the
six-row layout and only one row in the five-row layout without requiring an
XP-detection failure. The earlier25px/22px snapshot was from another moment;
its grid cannot be assumed for this scene/font/UI state.

## Confirmed selected-owner XP state

Selected owner is present in g_seen (78 registry entries). Dexterity increases
94.9242249 ->94.9552155 during the capture, active=true/recent=true, last gain
age about0.8–0.9s in first/last samples. Athletics25.9523869 is active/recent,
pending0.00074768, last gain58–68s old. Martial arts81.0501556 is live but
inactive/not recent for this particular armed character (last=0).

All three raw float references equal their Snapshot values. Pointer-index
bindings have matching owner, Skill pointer, statIndex and nonzero stable
token. Dexterity and Athletics collection/registration therefore worked for
this owner, while geometry rejected rendering. This snapshot does not prove
that Martial arts was recently trained by the separately reported unarmed
character; it must not be generalized to that other character's history.

## Limits and next decision

There is now direct runtime evidence of insufficient space at the current
native row height, rather than a guess about tiny XP, stale pointers or the
old map-order defect. A future fix must address the actual available space
without hiding ordinary stats, covering neighboring HUD blocks or silently
changing XP thresholds/lifetime. No additional production logic patch has
been made pending Root's integration/user decision.

## Authorized exact-fit correction

Root subsequently approved reclaiming only our unnecessary gap/trailing pitch,
without modifying native HUD geometry, font size, XP state or widget height.
The existing progress/name widgets already have height=pitch-2 (24px here).
Current layout now starts at vanilla bottom, and capacity counts actual last-row
height: remaining>=rowHeight ? 1+(remaining-rowHeight)/pitch :0. Consequently
185/bottom161 fits one24px row and185/bottom135 fits two rows on26px pitch;
both last rows end exactly at185. No native or neighboring rectangle overlaps.

A one-pixel divider gets a two-pixel gap only if that gap fits **after the full
allowed capacity**, capped by MaxRows. It hides in exact-fit layouts. This
reservation does not depend on the number of currently active skills, so adding
a second skill cannot shift the first row. This preserves a divider in roomy
layouts while retaining all measured row capacity in tight layouts.

Eight new panel assertions include these exact measured bounds, all MaxRows1..5,
one-pixel-too-short parents,100 alternating5/6-row refreshes, and divider stability
when another skill activates. Six of the first seven assertions failed against
the unmodified872b source before the correction; all31 panel checks now pass.
Original XP Info at0x3d8f measures remaining height from the vanilla bottom,
without this plugin's former+6px gap; its placement helper0x2d14/0x2d8b retains
the native widget height. This narrow correction likewise preserves widget
height and only uses space proven available by the native snapshot.

Final exact-fit artifact: `D:/DEV/Kenshi/build/SkillLevelUpInfo/SkillLevelUpInfo.dll`,
141824 bytes, SHA-256
`df789027e693578087824a40f58f96dd50374d148aa0622f921052cd0e292cdf`.
Adjacent map timestamp0x6ae48f91 matches the final link. All99 behavior checks
(31panel),9 probe parser checks,37 KenshiLib0.5.0 imports,15 installed MyGUI
imports, the exact two exports and three62-entry MO catalogs are verified.
## Native acceptance of the exact-fit candidate

Root installed df789 and sampled PID35788 after confirming its creation time,
executable path and matching loaded-module header. Two separate 10-second runs
each produced 40 successful samples with unchanged panel/owner identity:

- Six vanilla rows: bottom161, capacity1; the visible XP label/progress/value
  rectangles were y161,h24, ending at185 inside the185px parent in all 40 samples.
- Five vanilla rows: bottom135, capacity2; both visible rows were y135/h24 and
  y161/h24, ending at159 and185 in all 40 samples. No sampled row flickered away.

Ignored raw artifacts: runtime-35788-162252.json and runtime-35788-162516.json.
The user subsequently requested the vanilla label font, a maximum of two rows
in MCM and restoration of the separator in these tightly fitted layouts. Those
changes are a new candidate and require their own acceptance; these observations
prove the df789 geometry, not that later candidate's appearance.
