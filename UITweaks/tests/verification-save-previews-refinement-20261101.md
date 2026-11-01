# Save preview refinement, 2026-11-01

Root delivery checkpoint: independently rebuilt candidate328192B,
SHA256`b6a0589ba8d888062c907c3586e49d8a50c8fba39b85e0269aa37b82ac659d8d`,
installed with all3 matching MO files after user closure. Personal UITweaks.ini
hash unchanged. Backup:
game/_codex_work/ui-save-previews-20261101/before-save-png-height-refinement.
User acceptance of this new PNG/Save-menu/height refinement is pending. The
previous full-width candidate7076 was already accepted, before these addenda.

Authorized scope: UITweaks only. No installation, game control or publication by
this task. The parent authorized a scoped commit after independent review.
Skill-only design approval gates are explicitly waived.

Implementation plan:

1. Pin a one-time raised opening position, stable selection, full-width frame,
   uncropped aspect-fit image, hidden empty state, and increased dimensions in
   the real placement policy tests.
2. Support existing slots in both native save and load dialogs. Verify SaveGameMenu
   inherits LoadSaveWindow at offset zero, bind the actual root/list/games tuple,
   and retain the existing explicit selection gate.
3. Replace point sampling with WIC Fant filtering; encode lossless PNG up to
   1600x900 and 6 MiB, retaining legacy JPEG sidecar decoding and atomic writer.
4. Run real WIC codec/filter tests, extracted adapter/capture/writer fixtures,
   geometry/RMB regression, production build and import audit; request independent
   review from the parent. Record limits and candidate identity here.

Design choices: keep a single capture per successful save and the existing
3840x2160 readback pixel cap; keep one decoded preview texture and one pending
writer. The latest user correction supersedes vertical cropping: raise the dialog
immediately when opened, reserving space based on viewport aspect before a row is
chosen. Do not reposition on row changes or while the user drags the dialog.
On short viewports fit the entire image into the remaining height, centered inside
the full-width frame. No placeholder or frame is visible without a valid image.

Native ABI evidence: installed EXE RTTI SaveGameMenu COL RVA 0x18318b0 / hierarchy
0x18318e0 lists SaveGameMenu, LoadSaveWindow and wraps::BaseLayout with member offset
zero; LoadGameMenu likewise has a LoadSaveWindow base at zero. Existing base-layout
list 0xA0 / games 0xA8 and SaveInfo 0x108 assertions remain. Static SDK addresses
and raw Steam EXE code are not sufficient to verify runtime methods: prior live
evidence and official KenshiLib slot resolution are used. The fresh read-only
running-process Save audit was completed; its results appear below.

The preceding full-width DLL `7076f4725d20f2aa6fc6120a0e50922119c381f76df941d2ad1c9e72c01e7d52`
was installed and accepted by the user before these latest requests. That acceptance
covers the earlier HUD/full-width behavior, not the new Save-menu support, stable
opening anchor, height setting, hidden empty state, or higher-resolution PNGs.

Initial RED evidence: policy fails `ValidDimensions(1600,900)`; actual extracted
selection adapter fails an existing save-menu slot whose root/list differs from
the load menu. Further codec/layout checks and final results follow below.

## Current candidate and verification

Candidate `D:/DEV/Kenshi/build/UITweaks/UITweaks.dll`, 328192 bytes, SHA256
`b6a0589ba8d888062c907c3586e49d8a50c8fba39b85e0269aa37b82ac659d8d`.
Full canonical production build succeeds. Import audit: 338 named imports,
eight modules, zero missing against installed game and system DLL exports.
Independent review is CLEAR; the reviewer reran the full suites and rebuilt this
final DLL. The parent also inspected the key diff without additional findings.
The previous `4c1e8f34ff3199836860ed18a3a84197090b4e79ee57ef456d934f7b4e28862a`
was an earlier build of this source; the reviewed build above is the delivery candidate.
This task has not installed this candidate, changed a save, or published content.
Release version remains the existing pending v12. Only UITweaks source/artifacts
were edited; compiled RU/EN/ZH MO files were regenerated locally from the PO files.

Fresh verification:

- Policy: 73 checks, including stable reserved opening geometry, two thumbnail
  sizes at the same selected position, hidden missing images, viewport bounds
  from 640x480 through 4K, and uncropped aspect ratios on short viewports.
- Actual Windows WIC: 220 checks, including PNG signature and exact pixel values,
  high-entropy 1600x900 lossless encode/decode within the cap, legacy JPEG decode,
  checkerboard anti-aliasing, rejected dimensions, and a real 4K filtered resize.
- Actual worker: 34 checks. A legacy version-1 JPEG sidecar is read through the
  production Read path with a real quick.save revision. An encoder failure leaves
  the previously good sidecar byte-identical; cancellation/atomic commit tests pass.
- Existing GPU WARP: 21 checks; lifecycle: 30; alpha transaction: 1045; selection:
  15 (both dialog identities, no default image, first-row click, same names in
  different roots, one anchor per opening, no reanchor on selection, new empty
  slot rejected); HUD sequencing: 16; compiled translations: 18.
- Extracted height adapter: 11 checks for separate Save/Load roots, repeated and
  alternating opens, changing settings on the next open, external/native resizing,
  weak-root invalidation/address reuse, unsupported alignment and parent ownership.
- Native MyGUI height fixture: installed vanilla/Dark Save and Load XML all use
  direct-child GamesList Stretch alignment. The actual native resize expands
  that viewport by 65 pixels for a 325-to-390 root change and restores it at 100%.
  This tests native widget geometry; actual rendered row counts still need live UI.
- Existing native geometry suite and 108 RMB checks pass unchanged.
- `git diff --check -- UITweaks` passes.

The real WIC test caught a new pitfall during implementation: using BGRA as an
alpha-bearing source let Fant treat unused backbuffer alpha-zero bytes as image
transparency, blackening RGB. The RED pixel test now passes with the source
declared 32bppBGR (BGRx), explicitly ignoring that unused fourth byte before
filtering. This preserves the previous capture semantics without changing game
pixels or the readback format. One isolated 3840x2160-to-1600x900 filter took
12.089 ms on the first run and 17.693 ms on a later run. These are isolated CPU
fixture timings, not an in-game frame/FPS result or a maximum-duration guarantee.

## Ownership and workload

Capture remains one accepted readback per successful save request, under the
existing 8,294,400-pixel readback cap and 64 MiB GPU scene-backup cap. No extra
capture, continuous screenshot cache, save-directory scan, or new worker is added.
Filtering is one-shot CPU work on owned pixels; PNG encoding and atomic writing
remain on the existing single CPU-only worker. Worst-size thumbnail is 4,320,000
BGR bytes; the selected RGBA texture is at most 5,760,000 bytes. Each sidecar payload
is capped at 6 MiB. Native save data is never changed by the preview writer.

The full readback buffer is at most 33,177,600 bytes. WIC may make an additional
owned bitmap copy while filtering; both are released after that capture. PNG
encoding uses an HGLOBAL stream plus bounded encoded bytes in the worker.
Selection holds only one preview texture; decoding validates dimensions before
allocating output. Failure to encode/decode is reported and cannot turn a failed
capture into an overwrite of a good preview. Existing cancellation, revision
checking, bounded worker shutdown, and GUI unlink lifetimes remain in place.

The additional MCM height setting is SaveDialogHeightPercent, default 120, bounded
100..160, applied on the next menu opening without MCM_RESTART. Two weak references
and two base/last-height records (one per Save/Load context) prevent cumulative
enlargement when surviving menu roots alternate. Native/external height changes
recapture the base; destroyed roots are invalidated by the existing unlink helper.
Height is bounded by viewport and preview space. A layout without a direct,
vertically stretched list is left unchanged with an error instead of enlarging
only its outer frame. The setting also works when screenshots are disabled.

Steady-state Tick still compares widget geometry before changes and performs no
decode/filter/INI polling or file scan while selection is unchanged. AnchorMenu is
called only after native showLoad/showSave, before selection; user drag/resize is
then respected, with image fitting or hiding if space becomes insufficient.

## Read-only native evidence and remaining acceptance

The initial game checks returned no process. A later check found PID 13024,
start 2026-11-01 22:18:34 Moscow, executing
`D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe`. The root-folder
Steam EXE previously inspected is a different raw image; this explains the code
mismatch. No launch, focus change, input, injection or process write was performed.
The parent coordinated the game; this task only read its memory.

Live native audit now confirms official slot 6189 showSave at RVA 0x481bf0 allocates
0x118 bytes and writes SaveManager+0x108. Its ctor resolves to 0x481660 and calls
LoadSaveWindow ctor 0x47f890 with the same this pointer, before setting Save's own
vtable at 0x16d8aa8. Save's virtual select at 0x47a3a0 reads the inherited list at
+0xA0, games data at +0xB8, and indexes SaveInfo by stride 0x108. The base ctor
scans local and user roots into the games lektor at +0xA8 and constructs the list
from that same index. There is no invented save-dialog field or guessed ABI cast.

Ignored evidence: `tests/_generated/save-show.asm`, `save-menu-ctor.asm`,
`save-menu-select.asm`, `base-menu-ctor.asm`, and the existing native audit JSON.
The read-only `audit_save_previews.py` includes save-show for repeat inspection.

Parent task will coordinate installation or user acceptance. Actual save-menu
appearance, opening/selection stability, screenshot text quality, save latency,
FPS, and long-run memory still require controlled game evidence; the tests above
do not prove universal skin compatibility or zero driver/native leaks.
