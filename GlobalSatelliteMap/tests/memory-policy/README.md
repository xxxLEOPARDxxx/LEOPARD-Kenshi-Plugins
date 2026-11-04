# Session RAM/VRAM policy proof, 2026-11-04

Production owns one Main-only boot configuration: `Configure(gpuGiB,ramGiB)`
acceptsVRAM1..8 andRAM1..32GiB, defaultsVRAM1/RAM2GiB, and refuses a changed configuration after
the first success. Repeating the same configuration is harmless. Configure must
run before workers; hot INI/MCM edits require restart. Getters return references
to stable boot storage so aliases created during static initialization do not
freeze the old512MiB limit.

Maximum physical metadata is6144fine slots, independent of active capacity and
the unchanged96visible demand. Active slots reserve192MiB for actual K2,
40MiB viewer and margin, then floor remaining bytes by actual1.25MiB/window:
GPU1..8GiB gives665/1484/2304/3123/3942/4761/5580/6144slots. Storage is lazy;
these counts do not allocate corresponding GPU payload. GPU debt and fences
remain included in the total selected ceiling. Scratch/producer CPU stays64MiB;
the separately selected RAM budget belongs to the prepared-tile cache.

`run.py` compiles actual policy/coordinator/retirement headers on VC10/Od and/O2,
starts one short process per setting (no reconfiguration races), and passes31
controls each. It covers defaults, allVRAM1..8 independent settings, extended
RAM9/16/31/32GiB, invalidGPU0/9/~0 andRAM0/33/~0,
immutability, GPU aliases acrosswater/local/fineplanner, visible96, active/max
bounds, actual K2+full cache admission, pending/retired debt, overflow, complete
scaled resident-list validation/duplicate refusal, and actual WARP event-fence
drain/reuse. Opaque debt counters exercise multiGiB arithmetic without allocating
multiGiB data. Native actual-session stats/admission headers also compile/O2/GL.

Initial `--legacy` run before production changes failed at the default budget
check (old512MiB profile). Those two small RED logs are preserved. That switch is
historical, not applicable after the new API exists. Current normal runner is
the reproducible production proof. No game/UI/INI, installed DLL or atlas changed.

Performance review: resident duplicate validation uses a fixed32KiB open-address
index rather than O(N²) comparisons; no per-pan heap. Retirement records have a
first-free cursor and high-water bound, so mass insertion is linear and empty
idle polling does not scan12544records. Main integration must use active counts
and cache visible lists rather than scanning all6144slots on every unchanged
frame. Array-backed metadata is bounded and must remain in the real CPU ledger.
Host/WARP proofs do not establish measured game FPS, allocation success on all
hardware, or absence of every long-session leak.

The RAM32GiB extension first failed at the real Configure acceptance gate on
the previousRAM8GiB ceiling (`ram32-red.log`); its GREEN process still exercises
the same lazy/fenced GPU path without allocating32GiB of RAM. Larger RAM limits
do not change GPU slot counts or the separate64MiB producer scratch budget.

`native-cpu-verification.json` records a separate bounded admission check of the
final native Main layout. An included actual GlobalSatelliteMap.cpp was compiled
to COFF solely to read initialized sizeof constants; it was never linked/run.
The unchanged production reservation and MakeBudget bodies then ran on the host
under both optimizations. With the full6144 index, planning metadata and scratch,
192widget bindings,2MiB existing-town allowance,3MiB loaded-packet reserve, global
window index and TWO ordinary jobs (each index plus1MiB pixels), total charged
CPU is66,845,462B against67,108,864B:263,402B remains. Source hashes and exact
allowances are in the compact JSON. This is a conservative defined scenario,
not a measurement of every allocation inside Ogre/MyGUI or the whole game.
