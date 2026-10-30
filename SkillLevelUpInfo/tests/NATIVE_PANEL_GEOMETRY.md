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
