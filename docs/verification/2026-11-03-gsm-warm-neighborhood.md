# Warm neighborhood retention, 2026-11-03

Locally installed candidate:GlobalSatelliteMap.dll1,113,600bytes,SHA14857304559b79fd7b30c274ef81a2f2e56da4ea5a2cb7945741a5ddfc9f4e81. No atlas rerender or public release. Personal INI hash remains63d3ca9d5db3013724288fd70eacad35367df25e4e449a6012ff675ead6156f8.

## Proven cause

FineObserve retired every offscreen GPU tile at two/frame even when there was free capacity. A viewport pan could discard28-55 previously sharp tiles in roughly0.14s at200fps. The session's actual disk files/stored count were52-63, below128; subsequent return reached phase4/sourceGPU0 using disk, so this was not evidence of a full cold native rerender on every return. Lost GPU residency required unnecessary reads/uploads and could remain visually blurry during that work.

## Repair

Keep same-scope offscreen tiles until actual memory/slot pressure. Use64-bit LRU touch with outside-first eviction and protected producer pair; never shade offscreen warm cells. Preferred feasible resolution can evict obsolete-spacing entries rather than permanently choosing cached lower quality. Physical publication reserves actual current missing keys, not hypothetical Worst keys when the actual visible set is already cached.

Development profile:Gpucap512MiB,physicalfinecache256,visibleplan96,diskLRU512. CPU cap remains64MiB and every expanded object/fence ledger is counted. The separate visible96 bound avoids accidentally producing hundreds of visible cells just because the resident cache grew. Retirement table512 covers256fine+144ordinary+layers on clear without losing owner references; source/pending/retired GPU debt remains real. Sixteen overview fallback slots survive. No file schema, world/material/SHA authority or source identity was weakened.

Hardware observed before choosing this local profile:RTX4070TiSUPER16,376MiB total,6,495MiB used. This is a bounded private map-resource limit, not a promise to use all GPU memory or a universal hardware requirement. Cold uncached areas can still require native rendering; world/source changes correctly invalidate cache.

## Validation and limits

Independent review CLEAR. Per/Od and/O2:34,600 planner controls,16 profile/union/LRU controls,69 extracted warm-return caller controls+two old-behavior mutants,1,218 actual WARP GPU/fence controls including400 retired owners. Actual512-file LRU/fault/cleanup, FineWorker fault matrix, coordinator fault matrix and prefetch regressions pass. Linked native candidate has startup/MCM exports and84 verified native imports. The unchanged cached-return fixture performs zero Get/Queue/allocations; this is not a live FPS/long-session leak guarantee. User gameplay confirmation is pending.

Machine freeze:D:/DEV/Kenshi/research/gsm-warm-neighborhood-20261103/verification.json, SHA1631e07c2b00aa0534c0c08ee5c7657280896188f9d4b6850351e88c356a2f25.
