# Missing native XP bars: confirmed cause, 30 October 2026

The personal INI was read without changes and contained `Enabled=1`,
`PanelBars=1`, `FloatingLabels=0`, `Debug=0`. The authorized PID 35756 had
already exited before inspection; no crash inference is made and no other
process was opened. No live widget coordinates or runtime success are claimed.

Cause was established against actual unpacked Steam executable code and the
original XP Info DLL, not inferred from the prior UI fakes:

- `DataPanelLine::createMe`, RVA `0x6f8ee0`, gives **both** ordinary name and
  value text boxes the same left position, `int(parentWidth * .03f)`. Their
  widths are `int(W * .96f)` and `int(W * .93f)`. The value text is right-aligned
  inside the overlapping full-row box. It is not a separate right-hand column.
- The old plugin treated `w2.left` as the bar start and `w1.left` as the name
  start. Its check `barLeft < left + 8` therefore always rejected this ordinary
  native layout, producing capacity zero even with recent XP and enabled bars.
- Native `DataPanelLine_Progress::createMe`, RVA `0x6fa5e0`, uses 48% of parent
  width for the bar when `showKey=true`. Original XP Info's placement helper
  at RVA `0x2c9c` subtracts this width from the vanilla value box's **right edge**,
  then adds ten pixels. This layout is now used, bounded to the actual parent.
- `GUIWindow::getHeight`, RVA `0x6e20e0`, reads the `win` widget height directly.
  `getContentHeight` is a separate method. The suspected height-getter confusion
  was ruled out; the actual nested parent remains a valid clipping constraint.

`inspect_native_panel_layout.py` reproduces these checks using only file reads.
At a 400-pixel parent: both native text boxes start at 12; value right edge is
384; the XP bar is 192 wide, starting at 202 and ending at 394.

The production `DrawPanel` regression with these native full-row coordinates
was observed failing twice before the fix. It now creates two recent XP rows
and gives the bar the original right-edge placement. The earlier narrow-column
fixture remains useful as compatibility coverage, but no longer defines the
native contract. Nested 75-pixel viewport clipping and stable-widget reuse
checks still pass.

Detailed logging now records state transitions for missing panel/owner/root,
disabled bars, non-player panel, pending session/character baseline, unusable
layout, insufficient space, no recent XP and visible rows. Geometry diagnostics
include outer and actual-parent sizes, baseline bottom/pitch, name left,
value right, resulting bar bounds, capacity and row count. They are emitted
only when relevant state/geometry changes, and remain off by default.

No INI, LiveXpBars, save, game UI or installed DLL was changed. Runtime visual
acceptance still requires installing the checked candidate and obtaining actual
player XP in the current scene; diagnostics can then distinguish any remaining
scene/overhaul-specific issue from the confirmed corrected geometry bug.

## Follow-up: bars disappear after weapon/character changes

The user rejected the first geometry candidate (`4db6cba3...`) after bars
appeared but lost their second row or disappeared in a six-stat layout.
This was not treated as runtime acceptance.

The next confirmed defect was the pitch calculation: `getLineByNum` walks
the native key-sorted map, while the plugin measured only successive maximum
bottoms in that order. A visual 25-pixel step could therefore become a 50-,
75- or 100-pixel jump, causing a false capacity of zero or one. The current
code sorts at most 512 ordinary row bottoms on the stack and takes the smallest
positive adjacent difference. It excludes all native progress lines from this
baseline, matching the original XP Info helper at RVA `0x28e0`:

- `0x2978`: exclude `DPL_PROGRESS` (`0xb`).
- `0x2988`: virtual `DataPanelLine::getBottom`, vtable `+0x58`.
- `0x2b1d`: sort the collected integers.
- `0x2b30..0x2b43`: minimum positive adjacent difference.
- `0x3d8c..0x3db3`: available panel height divided by pitch and capped by
  configured MaxRows. The previously accepted parent-viewport clipping remains
  stricter than the original's forced minimum of one overflowing row.

`inspect_native_panel_layout.py` verifies these instructions in the actual
unpacked Steam exe/original DLL. The original uses the same sorted/minimum-gap
scheme; this correction restores that behavior instead of changing XP rules.

Bounded read-only snapshot of explicitly authorized PID 13268 at
2026-10-30 11:34:14 UTC confirmed the installed `4db6...` DLL, PanelBars=1,
MaxRows=3, equal lifecycle generations 336/336, actual native root and row parent
320x185, and 22-pixel ordinary rows at tops 30,5,55 in native key order. Only
615 bytes were read. That moment's category had three ordinary rows and no
progress rows, so it does not prove live five/six-row XP or this new fix. The
process had exited before the bounded activity follow-up; no other PID was
searched. The regression extends the captured 25-pixel grid to five/six rows:
bottom127 leaves room for two rows; bottom152 leaves room for one. It checks
the same character/panel across weapon-mode changes 5 -> 6 -> 5.

The actual pre-fix production translation unit was rerun against the new
fixtures: eight checks failed, including native map ordering, tiny Athletics
XP, stable second rows and weapon-mode switching. Current production passes
all 23 panel checks. These also exercise a single representable float ULP at
Athletics 86.911 (less than one percent), 300 unchanged refreshes, independent
same-named characters, alternating native panels, and adaptive capacities of
one, two, three and five. Fake stats now optionally hold independent native
references rather than making every skill share one value. The fake WidgetRef
also implements native unlink invalidation, permitting real panel switching.

There is no epsilon in this module's `Skill::Observe`; any representable
positive delta marks activity before notification thresholds. Native
setLineProgress calls getLine/createLine then the same setProgress method;
there is no used/keep-alive flag, and DatapanelGUI update has no hide-unused
pass. Neither XP thresholds/lifetimes nor native progress updates were changed.
No extra per-frame widget creation, caption/progress writes or heap allocation
was added by the layout correction. LiveXpBars remains unchanged.

Final local candidate: `D:/DEV/Kenshi/build/SkillLevelUpInfo/SkillLevelUpInfo.dll`,
141824 bytes, SHA-256
`872b5526f1e3fd074a21431414dc7b814d0afbbead05efb3dcbf553078db3c1b`.
The adjacent `.map` was generated by the same final link. All 91 module
behavior checks pass (23 panel); 37 KenshiLib 0.5.0 imports, 15 installed
MyGUI imports and exactly the startPlugin/MCM_Describe exports are verified.
All three local MO catalogs were rebuilt and cover all 62 active entries.
No game installation or version change was made for this follow-up.
