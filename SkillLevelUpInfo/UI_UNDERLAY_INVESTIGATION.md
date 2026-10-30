# Native value-underlay alignment (candidate; visual QA pending)

Root captured `underlay-ca0-27924-vanilla.json` with fresh process identity and
an exact loaded-ca0 header/MAP match. There are24 records,22 valid identity
samples and2 polling races; no global stopped condition. This task reads only
saved evidence. User static screenshots21:49:49/21:50:06 were viewed locally.
No process access, UI automation, game calls or screenshots were performed.
Address-free source facts/hash are retained in `tests/underlay_profile.json`.

## Measured cause and focused layout

The stock StatsPanel backing is a single Stretch quad from atlas0,907,240,165
in Kenshi_UI.png. Actual background is324x219 at absolute0,1228, with native
content297x181 at10,1247. Native value text covers a full row and ends at303;
it is not a narrow column. The atlas value backing starts at source x150,
which maps to ceil(324*150/240)=203. Old progress outer bounds163..305 and its
Empty/Track offsets13/14 put its visible frame before that backing.

`PanelUnderlay.h` checks public ResourceSkin texture, single SubSkin basis and
normal-state UV profile. Global resource existence alone is insufficient: a
reviewer actual-source custom-quad repro exposed that false match. The selected
quad now passes RTTI SubSkin, a once-only native_setUVSet body/layout guard and
its actual UV check via legal protected member-pointer access to mRectTexture.
There is no invented derived-object downcast or guessed private field. Actual
UV validation repeats four float reads so same-pointer state changes invalidate
the mapping. VC10 gives protected UV offset50, matching native getter/setter;
unrecognized native profiles/quad types retain fallback without field access.
A bounded six-ancestor walk finds the actual whole
background quad; its live dimensions/absolute coordinates provide the affine
transform. The candidate whole progress frame is203..303, width100; inner
native frame/track offsets remain intact. The owned name gets181px, fitting
the observed150px Martial arts name that formerly clipped in141px. A second
render width480 maps the boundary to300; it is not a203px screen constant.
No font, palette, height, reflow or5/6-row capacity changes are made.

This is a verified stock geometry profile. Dark UI's277px UV profile and other
unknown textures/UVs retain ca0 placement. Changed artwork with identical
metadata cannot be detected; arbitrary atlas artwork is not promised. The
native nine-slice frame has fixed caps, and the narrower frame needs visual
QA even though its complete widget/children remain within the value backing.

## Allocation/call evidence (scope matters)

Native ca0 captures show owned labels2 creates/0 destroys, but native base,
progress and divider pointers churn. Five-row capture22/24 valid records has
10/11 distinct progress pointers,16 generic names per slot,14 divider pointers;
six-row23/24 has10 progress,16 names and16 dividers. Scroll/content/native
parents remain one address in each window. TypeReads rises192/96 because
native source labels change. Stable owned labels do not prove zero native
widget creation. Candidate labels share one RTTI validation per live source
binding; a new/deleted binding is still validated.

Offline primary native proof: installed SDK slot2157 resolves Steam891250.
CharStats::getGUIDataForMainInfo checks literal English Martial Arts:/Crossbows:/
Attack: via raw lineExists at8912e6/891358/8913c5 and calls clearPage through
vtable+68 at8913fa when absent. Its displayed keys pass through the game's
translation pipeline before setLineStatInfo. All valid saved RU maps lack the
English sentinels. This native localized-row rebuild is outside this focused
module fix; no alias, translation or original-method patch is introduced.

Independent actual-production fake repro also demonstrates plugin divider
churn:100 NPC and100 quiet-player panel interleaves each caused100 context
retirements/destructions, while owned labels remained2/0. Context reset now
hides/retains its live weak ref. Only a different active parent destroys it.
The same source fixture is RED before this change and GREEN afterward. This
proves the avoidable plugin path, not that those interleaves caused live churn.

Scalar counters distinguish calls/context resets/native progress creations/
divider creations/hides/parent retirements. Detailed Debug-only tracing keeps
32 fixed104-byte records: committed sequence, tick, generation, context bits,
opaque panel/root/owner, category/combat, live progress widget pointers before
and after original, divider before/original/after draw. Atomic sequence publish
lets the read-only parser skip partial/racing entries. Debug0 performs no trace
native lookups/ring writes. The probe never dereferences historical pointers.

## Verification and limits

- Actual-source layout fixture: stock324 and alternate480 widths,150px caption,
  Dark/unknown fallback, source churn, stable resource lookup/allocation,
  Debug gating/native removal,200 foreign interleaves and parent replacement.
- Existing vanilla/Dark five/six row, lifecycle/restore/ownership tests remain.
- VC10 helper proves ResourceSkin container ABI: SubWidgetInfo64/type24,
  vector32/map32, state rect8, cache40/main24/known32/applied33/reads36, SubSkin
  protected UV50. Installed engine
  getters and create-skin stride match; no GUI objects are invoked by proof.
- The renderer probe alone uses audited native offsets: Widget SkinItem base288,
  main338/texture360 and SubSkin UV50..5c. Production mapping uses public APIs
  plus the specifically guarded/verified protected SubSkin member pointer.
- New DLL/MAP/probe must be frozen together before Root installs/tests. Runtime
  visual acceptance of this candidate, especially the narrower native frame,
  remains pending. Native localized progress recreation remains present.

## Frozen candidate for Root QA

DLL: a63b8462542a0cf5263fc9b50e8a72b81be02290981665922db408fcdadf80f3,
162304 bytes. MAP timestamp6ae4f78e, SHA256
d47a9d42a363ef2a2b6cf3cfa95730f626d94a8fd7d65b318c0c1654c631f286;
PE image2f000. Fresh188 module checks plus16 base-parser and4 renderer-parser
checks pass. KenshiLib37 imports and MyGUI32 imports match installed exports;
only startPlugin and MCM_Describe are exported. All three62-entry MO hashes
remain unchanged. Source/version/INI/install/Git are not changed by QA tools.

The rejected E552 intermediate is not approved for installation. Final candidate
checks the actual quad UV, including same-pointer state changes and unknown RTTI.
Independent review reports no remaining source blockers. Native visual acceptance
is pending, particularly the100px whole frame with its native nine-slice caps.

After Root installs and verifies a fresh explicit PID/start/path, the ready command
uses the bundled Python and a NEW output path (no automatic PID discovery):

    python tests/readonly_underlay_probe.py --pid FRESH_CONFIRMED_PID --seconds 6 --interval .25 --out tests/_generated/underlay-a63b-NEW.json

Use the bundled Python executable, as in tests/run_tests.py. Debug1 enables32-entry lifecycle history; Debug0 keeps
scalar counters/geometry and leaves the ring empty. Context-mask bits are panel1,
root2,owner4,category8;16 means missing owner. Historical pointers remain opaque.
The reader enforces exact approved DLL/MAP/header and8MB total/256KB sample budgets,
never enumerates another PID, and refuses to overwrite existing renderer evidence.
