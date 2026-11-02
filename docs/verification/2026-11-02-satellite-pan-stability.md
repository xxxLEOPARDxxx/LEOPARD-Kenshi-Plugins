# Satellite viewer: stable detail while panning

The accepted viewer could switch source levels after a one-pixel pan: visible
primary rectangles had rounded widths, and the actual visible window count
occasionally crossed the48-slot limit. The independent production fixture
reproduced both cases before the change.

LOD density now uses the entire projected grid; admission reserves the worst
visible phase at that zoom.56 detail slots plus16 pinned fallback slots remain
within the existing96MiB cap (94,634,160 bytes including scratch/constants).
An opportunistic one-window apron uses only remaining worst-phase capacity.
Fresh visible windows precede lighting refreshes and off-screen prefetch.

Verification:

- Full viewer suite:13 manifest,55 worker,17 geometry,13 clock,3469 window
  policy checks and both new pan regressions passed.
-662137 extracted geometry/bounds/supersession checks,26 adoption/cancel and
  ten immediate-resize checks passed; six existing compiled mutants rejected.
- Independent review swept68418 planner phases/13867834 checks, eight actual
  priority cases and four additional functional mutants; no blocking findings.
- Development DLL495616 bytes, SHA256
  `e6dc69c4d4bdd635ed39c3a2e83cc920100fa45c802a6803dfd08b60d5d2fdd3`.
  Installed with the personal INI preserved. No GST2 recapture or source edits.
- Live paused SAT-render-proof-20261101, PID5580: initial16 fallback windows;
  detail cache reached72 and retained them across zoom/pan. At sourceLevel3,
  visible windows reached32/32; reads stabilized at120 (125952000 bytes).
  A50x10-pixel drag retained the picture without clearing the map. No viewer
  error was recorded. This is a short functional test at an intermediate zoom,
  not maximum-zoom acceptance, a CPU/FPS comparison or a long leak test.

Water, building geometry and local-light layers are separate unfinished work.
