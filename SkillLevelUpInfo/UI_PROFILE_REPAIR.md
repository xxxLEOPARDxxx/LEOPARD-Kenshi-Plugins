# Native UI repair: vanilla and UI replacements

Root-authorized read-only captures are reduced to address-free geometry in
`tests/ui_profiles.json`. Vanilla six-row capture:24/24 samples. Vanilla
five-row capture:15 valid samples, then the reader stopped safely on an
unavailable address; it is not a complete six-second run. No process is opened
by this implementation task.

Observed causes:

- The five-row vanilla XP name has an outer height23, font25, and actual text
  region17,9,106,3. Only3px of text can render. The native progress creator
  hardcodes GenericTextBoxFlat; its frame/insets are retained by vanilla and
  replaced with a full text skin by Dark UI. Copying only the font is incorrect.
- Native attribute names use actual type/skin `Kenshi_PaintedWordWrapEmpty`
  with a full25/26px rendered client. A TextBox cannot safely become an EditBox
  merely by changing its skin.
- Vanilla bar child offsets are scaled13/14px, rather than the unscaled XML
  offsets10/11px. Inner-child nudges assume a skin profile and can misalign a
  visible frame. Move the complete native bar/frame together instead.
- Six-row vanilla:181px viewport, six25px rows at5,33,62,90,118,147; bottom172
  leaves9px. The17px of inter-row gaps must be redistributed to fit an XP row.
  Five-row vanilla fits5+2 rows of25px, one divider, and5px top padding exactly.
  Dark UI:185px viewport,26px native rows;5+2 or6+1 rows plus divider require183px,
  so reducing top padding5 to2 suffices. Fonts/widget heights need not shrink.

Implementation plan:

1. Reproduce these actual profiles against the production draw path (RED).
2. Keep native progress lines/ownership intact. Hide only their unsuitable
   generic name widget; create at most two parent-owned labels using the
   current native name's public runtime type, skin and rendered font. Disable
   mouse/key/picking along their bounded client chains. Cache stable bindings.
3. Cache original native row coordinates using live weak references. When
   recent XP is visible, preserve all widget sizes and redistribute only
   unused top/inter-row spacing inside the actual viewport. Reserve the full
   configured capacity independently of currently active skill count. Put a
   one-pixel divider in its own space; restore geometry on hide/context change.
4. Size XP rows from the actual native name row height. Move the entire bar
   and percentage down2px within each row. Native frame/child offsets, physical
  23/24px bar heights, palettes, source text regions and full fonts are retained.
5. Verify both UI profiles,5/6 transitions, viewport/skin changes, external
   native relayout, restoration and zero steady allocations/geometry writes.
   Build/freeze a candidate with matching MAP and read-only label diagnostics.

No UI-name detector, global resource patch, font shrink, native ownership-vector
edit, installation, version change or publication is part of this repair.

Review follow-up: restoration additionally requires a live cached parent and
the row's current parent to match it. Reparented rows retain the new context's
coordinates. Actual-header RED/GREEN tests cover restore after reparent,
capture under the new parent, destroyed cached parent and ordinary restoration;
the independent review's two failing cases now pass unchanged.
