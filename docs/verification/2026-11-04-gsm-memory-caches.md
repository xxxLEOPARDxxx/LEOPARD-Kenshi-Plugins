# Satellite RAM/VRAM caches, 2026-11-04

Latest user requirement: independent1GiB MCM steps, VRAM1..8GiB and RAM1..32GiB.
Private development build, no public version or publication. Source defaults are
VRAM1GiB/RAM2GiB; local test settings explicitly use8/32. Both are restart-marked
because the process-session budget is immutable before workers start.

## Implemented

- Boot configuration read once; no per-frame INI.64bit live-reference budget
  aliases include all GPU layers and checked pending/retired/producer debt.
- Real fine GPU capacity665..6144 lazy slots, bounded96 visible source plan.
  Resident validation uses a fixed hash table instead of quadratic comparisons.
  The later same-day zoom follow-up expands the3m plan to512 (1m remains96);
  see2026-11-04-gsm-zoom-followup.md.
- Worker-owned prepared fine-tile RAM LRU after successful Put/validated Get.
  Cache hits avoid disk; published reads are separate scratch copies. Optional
  RAM allocation failures preserve valid disk results. Cache bytes include
  bounded index/node/payload metadata, separately from64MiB producer/IO scratch.
-8..64 optional surrounding pairs, after visible work and600ms stability;
  stationary views may warm. No speculative GPU uploads. Higher RAM settings
  retain more visited data rather than eagerly rendering the entire world.
- Ordinary map close keeps the same worker/cache. Hidden completion still
  drains/acknowledges work and releases unused reads; world/disable cancels.
- O(log N) fine lookup and bounded visible-key enumeration. Native MyGUI images
  use a192-widget pool, not6144 retained hidden widgets. Texture bindings clear
  before reuse. Planning scratch is a charged global serialized workspace,
  avoiding several hundred KiB of native thread stack growth.
- RAM hits/misses/bytes and selected limits are optional diagnostic telemetry,
  not admission snapshots. Unknown worker completion retains whole ownership
  and separately charges the RAM ceiling instead of disguising it as64MiB.

## Verification

- Native VC10 /GL /LTCG, startup/MCM/86 native-import checks pass.
- Installed DLL75f41fe57b205592b6362e4d39bd3b56e2f2d7851273e30e87cbd25afb4d6cdd,
  1,249,280B. Build log: build/GlobalSatelliteMap/memory-cache-build.log.
- GPU policy31 controls across12 distinct profiles on/Od,/O2, including RAM32,
  invalid33, VRAM8, lazy actual capacity, overflow and actual WARP retirement.
- RAM actual-worker/real-file LRU/allocation/quarantine controls pass on both
  optimization levels; independent RAM32 delta review CLEAR. No32GiB fixture.
- Index6479 controls per optimization plus4 functional mutants; actual OOM
  rollback and metadata-only6144-entry capacity. No allocations in visibility.
- Widget-slot18442 controls per optimization;6144 visited entries reuse192 slots.
- Main actual extracted boot/hidden-result paths pass14 profile/input cases
  per optimization, including unchanged INI on reopen and world cancellation.
- Expanded warmup8332 controls per optimization,128 distinct surrounding keys;
  existing actual prefetch/event caller38 controls per optimization and3 mutants.
- Actual native sizeof +actual MakeBudget extraction, conservative fullindex/
  manifest/town/city reserve/TWO ordinary jobs:66,845,462B below67,108,864B.
  The263,402B remainder is a bounded payload check, not whole-process memory.
- Independent GPU, RAM, index/Main/lifecycle and widget-pool reviews CLEAR.

## Installation and pending acceptance

Fresh CIM+GetProcess both showed Kenshi closed immediately before replacement.
Installed at02:47MSK. Backup and exact receipt:
`D:/DEV/Kenshi/backup/gsm-memory-cache-20261104/installation.json`.
Other INI bytes are unchanged; only VramCacheGiB8/RamCacheGiB32 were inserted.
Three MO catalogs and README installed. capture/ready/window-index hashes match
the prior accepted atlas; no rerender or mods.cfg change.

User is asked to load SAT-render-proof-20261101, pause, pan/return onx8 and reopen
the map for actual cache-hit/visibility measurements. Live FPS, long-session
growth and low-end hardware support remain unproven until measured. Hub's exact
native model refusal is still a separate diagnostic, preserved by this candidate.

Live PID26248 after the user's pan/reopen sequence confirms selected8GiB GPU and
32GiB RAM limits. At515..525s: RAM243,478,872B/232entries,418hits,638misses;
fine GPU186,122,240B, viewer31,522,078B, sourceGPU0, retiredGPU0, complete
visible phase4/spacing3,14/14pairs and all64 warmup pairs completed. These
are actual cache operations, not reserved32GiB or a long-session leak/FPS proof.
Whole-process private memory about8.4GiB is not attributable solely to this mod.

After installation a narrow reviewed repair separates CPU-only RAM read admission
from GPU upload admission when the physical GPU cache is full. Source candidate
f47d5cf00f9585527abd94d8c3662964837fef36c811cffaee6d9ee44ba63cb8,
1,249,280B/86imports, is built but not installed while PID26248 holds75f.
Visible PeekRead/actual upload still checks GPU admission; speculative completion
only acknowledges an unused validated CPU blob. Full-cache CPU-read/denied-upload,
source/join gates and extracted no-speculative-display controls pass.

User's latest acceptance reports blurry x2..x4, slower x6/x8 first detail and a
horizontal seam. These remain new investigation tasks; cache counters do not
prove those image/performance defects fixed. Hub refusal now reports HRESULT
8007000e at copied-sampler/COM-publication; its precise sub-bound is being audited.

Cleanup: two reproducible OBJ files (~12MiB) could not be removed because the
automatic tool review rejected deletion. No alternate deletion route was used.
