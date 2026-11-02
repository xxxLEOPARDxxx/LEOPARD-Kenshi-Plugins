# Mod-list scrollbar layout, 2026-11-02

The native scrollbar now has space between the mod-button frame and the
vertical divider. The change adjusts this ScrollView's outer width once when
BuildTab creates it, after the canvas has made vertical scrolling visible.
Native Client/VScroll alignment, clipping and wheel handling remain in use.
No global XML, UI scale, other controls, widgets or event hooks are changed.

Before the change the button right edge was `listW-20`, exactly the vanilla
scrollbar's left edge. The divider begins at `listW+8`: the gaps were0/8.
The helper measures the current native client/gutter, centers that reserved
gutter in the available space, and retains button positions and heights.
If a different skin reserves a wider gutter, button/canvas width is trimmed
to the measured client before use. Hidden scrolling retains the original
width. Invalid geometry is left untouched.

Actual local template evidence:

| Skin | Root width | Client right | VScroll left/width | Right padding |
| --- | ---: | ---: | ---: | ---: |
| Vanilla Kenshi_ScrollViewEmpty |135|115|115/20|0|
| Dark UI Advanced same template |135|115|117/16|2|

For vanilla the new gap is4 logical pixels on either side; Dark UI's
existing symmetric2-pixel internal padding gives6/6 around its16-pixel bar.
These are current native widget coordinate units; no hardcoded DPI multiplier
or replacement skin is introduced. Unknown asymmetric skin padding remains
as authored within the centered native gutter.

MyGUIEngine_x64.dll SHA256
`fea6f1f935703812485bd6606ff4d3ceea172ece131ed4c6dcbf7a7e25b083a8`:
getViewCoord191EC0 copies the actual client rectangle. ScrollView::setSize
191D90 invokes Widget's native resize then updateView191BA0. Native
ScrollViewBase::updateScrollSize1933F0 changes the client by the actual bar
width when showing/hiding it; the template's own padding remains. This is
why the helper uses the measured visible client instead of a fixed20-pixel
bar. The template Client has Stretch and VScroll Right|VStretch alignment,
checked by the test against both real local XML files.

`list_layout_tests.py` first compiled and executed the old actual BuildTab
block: RED for touching/uncentered scrollbar and clipped wider-skin buttons.
The current test compiles the actual helper plus the actual production block,
with only MyGUI/native widget effects replaced by a boundary fixture.
The fixture models the template's native align/visibility behavior; it does
not execute the in-game renderer. It checks both current skins, synthetic
12..48-pixel gutters, widths800..2560 and selected3840-wide tabs, client
insets, per-button order/height/client bounds, unchanged vertical canvas
range, hidden scrolling and idempotence/refusal paths. Four sensitive
compiled mutants must fail: missing helper call, missing gap, no canvas trim
and no button trim. Generated RED/GREEN/mutant outputs stay under
`tests/generated/list-layout`.

Cost/lifetime review: two bounded passes over the already-created mod-button
vector, only during BuildTab; scalar copies and existing native getters,
at most one list resize, one resize per clipped button and one canvas resize.
No allocations, retained pointers, new global state, new widgets or work in
the frame/update/wheel paths. The existing parent/WidgetRef teardown owns all
widgets exactly as before. This does not establish game FPS or long-run
memory behavior. Root review and in-game vanilla/modded-skin visual acceptance
remain separate; no DLL was installed or game UI operated for this change.

GSM warning alignment, supplied separately to root: customArea is created at
`(0,0)` in g_right; g_right starts at `listW+20`. Attach's x4/y2 adds only4/2
to that origin. Actual Steam DataPanelLine::createMe6F8EE0 (slot2618) reads
scrollWin width and uses `int(width*.03f)` at6F8F7D/6F8F8A as its label-widget
x, followed by the textbox skin's own text inset. A proportional3% area-width
inset follows the native row more closely than a fixed4. No GSM source was
changed here.
