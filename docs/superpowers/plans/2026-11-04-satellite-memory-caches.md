# Satellite RAM and VRAM caches implementation plan

User request, latest steering: independent MCM limits VRAM1..8GiB and RAM1..32GiB
in1GiB increments;
retain/prewarm prepared terrain in RAM, upload detailed visible/nearby tiles to
VRAM. No full atlas recapture or gameplay changes. Existing zoom steps remain.

Architecture: separate retained CPU tile payloads from bounded producer scratch.
Prepared immutable, identity-tagged tiles live in a bounded LRU RAM cache with
worker-side copies/IO. VRAM retains independently evictable fine windows under
one shared aggregate that includes layers, transient producer and GPU retirement.
Physical capacities must permit the8GiB setting, not just change the label.
No eager multi-GiB allocation, native calls on file workers, or per-frame INI.
Caps are process-session snapshots; settings requiring startup allocation changes
use MCM_RESTART. Defaults: RAM2GiB / VRAM1GiB; preserve other personal settings.

- [x] GPU policy: checked64bit1..8GiB selection, actual scaled fine residency,
  bounded metadata/retirement and honest admission; boundary/overflow/pressure tests.
- [x] RAM prepared-tile LRU: worker-owned immutable entries, identity validation,
  demand-before-prefetch, no eviction of published reads, bounded payload/metadata,
  allocation failure fallback and clean join/quarantine lifecycle; actual worker tests.
- [x] Main integration: two MCM integers, RU/EN/ZH help, session snapshot and
  real capacities; warm adjacent terrain after visible work; debug cache telemetry.
- [x] Review aggregate CPU/GPU accounting, hot-loop cost, failure/shutdown,
  lower/higher settings and repeated map close/open. Native build/import checks.
- [ ] Install only with fresh process-closed check; preserve atlas/INI backup.
  Live high-zoom pan/return shows RAM hits and resident GPU bytes. Commit and push.

Budgets refer to owned map resources; the game/driver also uses RAM/VRAM.
Allocation refusal must retain readable lower detail. Never claim weak-hardware
or long-session leak/FPS acceptance solely from static tests.
