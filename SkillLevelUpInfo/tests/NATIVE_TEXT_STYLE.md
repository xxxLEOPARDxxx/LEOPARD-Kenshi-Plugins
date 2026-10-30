# Requested vanilla font / two-row limit / divider

The previous df789 candidate's actual1/2-row geometry was accepted by the user
and Root's40/40-sample runs in `runtime-35788-162252.json` and `162516.json`.
This follow-up preserves its24px XP rectangles, positions, bar colors and HUD.

New explicit user choices: XP skill names should use the same font face/size
as the vanilla labels; MaxRows should be1..2 (default2); the divider should
remain visible in the tight layout. No font resource name or pixel size is
invented. The last ordinary row's actual rendered text supplies name/value
styles separately, via a bounded four-widget client chain, never a tree scan.
ISubWidgetText's public virtual font API avoids empty/nonvirtual TextBox getters
on composite EditBoxes. Setters run only when face/height differ.

## Measured SDK/native ABI difference

An independent VC10 helper with the exact production includes/defines reports
SDK EditText mFont=0xe8, mFontHeight=0xf8, sizeof=0x180. The actual installed
MyGUIEngine export bodies give different protected-field offsets:

| Actual export body | RVA | Relevant instructions |
|---|---:|---|
| EditText::getFontName |0xc58f0|mov rax,[rcx+0xe0]; add rax,8; ret|
| EditText::getFontHeight |0xc3a00|mov eax,[rcx+0xf0]; ret|
| EditText::getTextAlign |0xc3c10|read[rcx+0xb4]|
| EditText::getCaption |0xc3800|lea rax,[rcx+0x70]; ret|
| EditText::getViewOffset |0xc59e0|read[rcx+0x114], [rcx+0x118]|
| EditText::getTextSize |0xc5900|TextView at[rcx+0x140], plus shadow height|
| Widget::getClientWidget |0x1bc030|mov rax,[rcx+0x3d0]; ret|

Therefore this module does not use shared FontRefresh's protected mFont member
access for its null guard. That SDK offset would read the native texture field,
and merits a separate shared-code audit. No shared file was changed here.

Instead, startup checks the actual loaded engine's exported getFontName and
getFontHeight instruction profiles once (bounded export-jump following). Only
the matching profile enables styling. Public EditText/SimpleText RTTI must
also match before a read-only null check at the proven native mFont offset0xe0.
Unknown profiles keep the default style and emit one startup error; unknown
text kinds are skipped without changing the style;
there is no SEH and no potentially-null getFontName call. All actual style,
text-region and text-size operations use public APIs, never protected offsets.
A local helper loads the actual installed MyGUI DLL and verifies this profile
without opening Kenshi or invoking any GUI/game getter. This check passes.

## Tight divider without losing rows

Where spare space allows, the existing two-pixel gap/divider remains unchanged.
Where rows fit exactly, a one-pixel border may occupy y=vanillaBottom-1 and
end at vanillaBottom, before the first XP rectangle. Actual public text region,
alignment and measured text size must prove that both label/value text ends
before that border. Cache keys include live logical/rendered WidgetRefs,
rendered-text/font-resource identities, actual rendered caption, public shadow
and logical/rendered EditBox word-wrap states, face, size, region and alignment. Stable refreshes
compare const references/scalars and issue no copies or extra measurements.
Unknown/full-height/scrolled text suppresses only the border, preserving both
the accepted XP rectangles and capacity. Shadow changes invalidate the cache;
the new height comes from getTextSize, without adding guessed shadow pixels.
This conservative fallback still needs Native font/divider visual acceptance.

## Verification and limits

- Direct/composite styles, empty outer TextBox metadata and missing native font.
- Native0xe0 null guard with nonzero decoy at the incompatible SDK location.
- Unknown runtime profile safely skips font access.
-100 stable updates cause zero extra font setters/geometry writes/measurements.
-25px name/21px value cells within26px vanilla rows permit the1px border;
  full-height unknown padding hides it without losing the two XP rows.
- Existing exact185px bounds and five/six-row switches remain unchanged.
- Actual LoadSettings/MCM_Describe tests:1..2/default2; legacyINI3 clamps in
  memory and leaves both that INI value and unrelated settings untouched.
- Three locale hints/PO/MO and README reflect the user-selected two-row limit.

The read-only probe now includes actual direct/client rendered fonts, text
region/view size, shadow/alignment, divider coordinates and cached measurement
state. It uses these native getter-verified offsets for optional font reads.
No new process was inspected during this follow-up; Root confirmed the game
had closed. Native font/clipping/divider acceptance remains pending.

Final local candidate: `build/SkillLevelUpInfo/SkillLevelUpInfo.dll`,145920 bytes,
SHA-256 `1487b01b6194345ead383a1708565ce5b5c6e2669fc0c472fb1332ed6185ee43`.
Adjacent map timestamp0x6ae4a90a matches the same link.126 behavior/native-profile
checks and11 probe parser checks pass.37 KenshiLib0.5.0 imports,21 installed
MyGUI imports, exactly two plugin exports and all three62-entry catalogs are
verified. The loaded-profile flag is exposed in the map for read-only QA.

Independent review caught a stale padding proof in the first uninstalled font
candidate: actual text could change from25px to26px while face/region/alignment
stayed the same. Five regression assertions failed before the cache correction;
they now pass for caption, source identity, shadow, public wrap and unlink/address
reuse. Additional checks cover rendered-client unlink and wrap changes while
the logical wrapper remains alive. Divider geometry and accepted1/2-row bounds
were not changed by this correction. Caption/state comparisons do not allocate
or copy on stable frames; invalidation remeasures actual text without inventing
extra shadow pixels. ISubWidgetText exposes no word-wrap getter; available
public EditBox modes are observed for both logical and rendered widgets.
