Save preview candidate, 1 November 2026
======================================

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
