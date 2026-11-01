Save preview candidate, 1 November 2026
======================================

Latest full-width correction candidate
--------------------------------------

DLL325120 bytes, SHA256
`7076f4725d20f2aa6fc6120a0e50922119c381f76df941d2ad1c9e72c01e7d52`.
User accepted the previous HUD correction in game, then requested the image
span the save dialog rather than remain640px wide. Display now upscales to the
dialog width, preserving aspect and limiting height to available space. A
height-limited/portrait image remains centered without cropping. The missing
image row uses the same full width. Stored JPEG bounds640x360 remain unchanged.
New regression first failed against the old placement, then passes with34policy
checks. Complete preview suites, native geometry and108RMB checks pass; independent
narrow review CLEAR. This candidate is built, not yet installed at this checkpoint.

Live acceptance of preceding950ad322 candidate
---------------------------------------------

User screenshots20:38:53/20:39:00 show HUD/minimap and character panels included,
save/pause menus excluded, and a selected old slot's missing-image message.
User explicitly reported the behavior works; the outstanding feedback was width.
Read-only sidecar validation confirms:

* `UI Tweaks Test Screen`:77254bytes,640x360,77214-byte JPEG, native quick.save
  timestamp/size match; SHA5314c94f870a921b2fd8b7983eb00121020e7042755e28b1a822f675b2705ab2.
* `autosave1`:78416bytes,640x360,78376-byte JPEG, matching revision;
  SHA6195d27d73ac42caf7454183feafbbd54ca49e62ef39e1e1d5b2eb8d77997cbd.

Evidence JSON is ignored `tests/_generated/live-hud-preview-evidence.json`.
No user save was modified by inspection. Quicksave, main-menu live appearance,
all skins/scales and long-run FPS/memory are not proved by these two sidecars.

Previous HUD correction candidate (historical geometry)
----------------------------

DLL: `D:/DEV/Kenshi/build/UITweaks/UITweaks.dll`, 325632 bytes.
SHA-256: `950ad322cc61c5d429ef73261780cff5d83fec82857c060a4ee49398cda75a19`.
Clean build and all 148 native imports resolve (KenshiLib 54/Ogre 14/MyGUI 80).

User live feedback corrected the required screenshot: HUD/minimap/character
panels must be included; only save/load and pause menus/backdrop are excluded.
The original candidate below instead excluded all MyGUI. Its actual sidecar in
`UI Tweaks Test Screen` was inspected read-only: 640x360, 69975-byte JPEG payload,
70015 bytes total. The user's screenshot also demonstrated excessive frame
padding and a visible empty frame before selection. These are corrected here:

* The panel is image size plus 6px per edge, preserving aspect ratio without
  upscaling/cropping. A 640x360 image has a 652x372 panel. An old selected save
  gets a compact 48px message; before user selection the panel stays hidden.
* Input delegates are attached after native show: MultiListBox change and child
  ListBox mouse activation handle different rows and the already-highlighted
  first row. Keyboard navigation also opts in. Native default updateInfo does
  not activate a preview. Weak root/list references guard menu destruction.
  Both own and inherited visibility must hold. The installed getters expose
  these separately: native top-level inherited visibility remains true while
  its own visibility is false. Corrected fixtures reproduced the old behavior
  RED (hidden popup forced a double pass; hidden load root accepted activation)
  and passed GREEN after the gate correction.
* With no modal, the normal full GUI renders once and then the game backbuffer
  is read. With modals, a one-save same-desc D3D11 texture backs up the clean
  scene; only modal subtree draw alpha is zeroed for an extra HUD pass, then one
  readback is taken. Exact alphas and clean scene pixels are restored before the
  normal full GUI is presented. No visibility/focus or previous-Present buffer
  shortcut is used. Native alpha traversal includes private skin children and
  InheritsAlpha=false descendants, avoiding a broad layer skip that would omit
  BestMinimap on the Window layer above the pause backdrop's Main layer.
* Temporary GPU resources are COM-owned, at most 64MiB including samples.
  Public native window attributes return borrowed Texture2D/RTV; the capture
  scopes AddRef retained objects. CopyResource temporarily unbinds and restores
  OM RT/DS bindings without calling Ogre during the unbound interval. No private
  D3D offsets, extra shader, continuously cached frames or worker rendering.
* Modal lookup is limited to 4096 nodes/64 depth per accepted save; alpha state
  is limited to 1024 weak references, roots to eight. Scope unwind restores
  alpha, references and GPU state; TLS guards exclude nested RTT captures.
  Save ticket/world/renderer checks run before readback and before submission.

Read-only installed binary audit additionally verified:
`Widget::_updateAlpha` implementation 0x1bc6f0 recurses through the exported thunk
for ordinary children (0x1bc753) and skin children (0x1bc778). The unchanged
setInheritsAlpha setter triggers that traversal without visibility/focus changes.
D3D11RenderWindowHwnd getCustomAttribute at 0x46d50 delegates to 0x43d40, supporting
borrowed ID3D11RenderTargetView (0x43e64) and ID3D11Texture2D (0x43ebf). Installed
ListBox::_activateItem 0x118ee0 emits mouse activation even for an unchanged row;
MultiListBox change maps back to face indices before emitting its public event.

Depth contents are not backed up, so the extra GUI pass required a separate
native check. Read-only installed backend IRenderTarget::begin resolves to
executable RVA 0xa76fe0. At 0xa770a2 it sets R9D=3 (LESS_EQUAL), at 0xa770a8
R8D=0, at 0xa770ab EDX=0, then at 0xa770ad calls RenderSystem vtable+0x250:
_setDepthBufferParams(false,false,LESS_EQUAL). Actual D3D11 depth params at
0x33310 call the write setter at 0x33380, which normalizes DL and writes the
depth-write field at +0x5e8; false is D3D11_DEPTH_WRITE_MASK_ZERO. This confirms
that HUD passes do not write depth in the installed backend. Copy scopes leave
the native state cache unchanged and restore all saved OM RT/DS bindings.

New checks: 31 placement/policy, 15 WIC, 21 real WARP GPU restore (including DSV bindings), 27 writer,
30 lifecycle, 1045 alpha (including 1025-node budget rollback values), 10 selection,
16 extracted capture ordering/cancellation/nested RTT; 12 translations and
vanilla/Dark XML inheritance pass. The new candidate still requires live HUD,
menu exclusion, focus/flicker, selection/layout, quick/autosave, FPS and retention
acceptance. The following first-candidate record is historical, not current HUD
behavior or proof that this correction has been accepted in game.

Independent final review of hash 950ad322...75a19: CLOSED/CLEAR. Reviewer repeated
all current preview suites, RMB108, native geometry, 148-import audit and final
size/hash verification, and independently checked visibility and depth behavior.

Original candidate record
--------------------------

Candidate: `D:/DEV/Kenshi/build/UITweaks/UITweaks.dll`, 311296 bytes.
SHA-256: `a4b6b4c2f9d6be0ee8ad79f83c895bd30be2dcac362a7eb4f87229f9dbfe4486`.
Clean VC10/SDK7.1 build. Exact RE_Kenshi startup and MCM exports verified.
All 146 native imports resolve against installed DLLs: KenshiLib 54, Ogre 14,
MyGUI 78. No game installation, personal settings, publishing or version bump.

Implementation and native evidence
----------------------------------

The installed Steam 1.0.65 process (PID 36248) was inspected using
ReadProcessMemory only. No native calls were injected, no process memory was
written, and no windows were controlled. The audit script records code/RTTI
in ignored `_generated/` files. Installed KenshiLib SHA-256:
`5b6240622d06801dc39896e385efb834b8e1fe2fcdf86067d071ae7a04f6c83b`.

* `SaveFileSystem::saveGame`, Steam RVA `0x473380`, slot 6169, queues the copy.
  It records the actual destination in currentSave. Its return is acceptance,
  not proof that the background copy succeeded.
* `sync`, `0x473f10`, slot 6170, checks state 2 at `+0x1e0`. It inspects the
  error string at `+0x1e8`, clears it, then returns to NORMAL. The adapter saves
  the success decision before calling native sync and verifies currentSave.
* `LoadSaveWindow::updateInfo`, `0x47df10`, slot 4637, indexes the games buffer
  at `+0xb8` with `index * 0x108`. `SaveManager::loadInfo`, `0x47db20`, joins
  SaveInfo.location at `+0xb8` and SaveInfo.name at `+0x28`. The preview uses
  those exact fields; same-name slots in different roots remain distinct.
* Actual RTTI identifies MyGUI::OgreRenderManager with IRenderTarget at `+8`.
  A dynamic_cast filters main-backend drawing from offscreen GUI targets.
  The installed compositor chain finishes scene/HDR/debug before custom MYGUI,
  all in the same output. The hook reads native D3D11RenderWindowHwnd's
  backbuffer before the first interface draw, without hide/show operations.
* `ForgottenGUI::shutDown`, `0x6ea850`, destroys the MyGUI manager at `+0x48`.
  The early hook frees the owned texture/panel and unregisters references before
  that destruction. References support subsequent manager recreation.
* Ogre::Root::shutdown has a verified native export (`0x2a6490` in installed
  Ogre). It cancels and joins the CPU-only writer outside DllMain. A five-second
  timeout retains one cancelled job and handle without freeing live state.
  The worker has no engine object pointers and cannot render or access MyGUI.

Bounds and ownership
--------------------

One accepted request can read at most 8294400 pixels from the main game window.
The temporary CPU readback is at most 33177600 bytes; native D3D staging/resolve
resources are additional and governed by the installed renderer. The owned
thumbnail is at most 640x360 BGR; display is one RGBA texture, at most 921600
bytes before driver overhead. One writer and one pending capture bound CPU
buffers and work. JPEG payload is capped at 256 KiB plus a 40-byte header.

Only the selected save's optional `uitweaks-preview.bin` is read. There is no
per-frame screenshot, decode, INI read or full-save scan. Repeated widget
geometry is compared before setters. Upload uses native blitFromMemory to
handle row pitch; no raw mapped-texture memcpy is assumed. Worker file handles,
COM objects, critical sections and unlaunched jobs have scoped ownership.
Cancellation and main-file revision checks gate atomic replacement.

The revision stamp is quick.save size and last-write time. External edits that
intentionally preserve both cannot be detected by this lightweight stamp.
Missing, stale or invalid preview files never block native loading.

Verification
------------

* Policy: 26 checks, including aspect ratios, zero/overflow inputs, readback/file
  caps, pending save ordering and viewport placement.
* Real Windows WIC: 15 checks, including JPEG round trip, dimensions, malformed
  input, channel order and Unicode conversion.
* Extracted production writer: 27 checks with real Win32 threads, WIC and files.
  Success, early/commit cancellation, stale revisions, missing destination,
  temporary cleanup, normal join, timeout quarantine and lock unwind pass.
* Extracted save/load adapters: 30 checks. Native-cleared errors, confirmation
  before/after capture, different roots, refused save, wrong thread, title menu,
  load/new invalidation, quicksave and autosave completion pass.
* Twelve new compiled translations are present in ru_RU/en_GB/zh_CN. Actual
  vanilla and Dark UI XML frame, font, text-area and palette inheritance checked.
* Existing RMB regression: 108 checks pass. Native geometry/TLS/SEH/crop suite
  passes. Five order hooks and the sequential portrait Trade fix are preserved.
* Independent read-only review: CLEAR. Reviewer independently repeated all the
  above suites, native import audit and candidate size/hash verification.

Limits and next acceptance
--------------------------

This candidate has not yet been installed or accepted in a live save test.
Manual save with its menu open, quicksave, rolling autosaves, overwrite/failure,
both Load Game contexts, vanilla/Dark UI appearance, UI scaling/window sizes,
capture delay/FPS and long-run memory retention still require game evidence.
Static XML checks and isolated tests do not prove universal skin compatibility,
all native driver failure paths, zero retained memory or an FPS result.
