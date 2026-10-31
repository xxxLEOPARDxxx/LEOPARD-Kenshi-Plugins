# Gate Push Guard v1 — implementation design

Independently authored MCM analogue, requested 31 October 2026. Inspiration only: requizm's Gate Push Fix, Steam 3730136323. No upstream code or original DLL is used.

The guard observes actual outside-to-inside movement across a continuously closed player-owned door. It returns the character to the last observed outside point only when the segment crosses the finite closed leaf aperture. First observation, gate closure/opening, load/reset, identity changes, discontinuities, long sample gaps, and unsupported geometry only seed/invalidate state. Already-inside characters are never relocated based on proximity. Unconscious, ragdoll, carried/carrying, turret, bed/cage/furniture and animation-driven actors are excluded.

Door load/update events feed a fixed registry of full five-field handles. A native movement-update callback records only scalar/POD observations near a private spatial-cell index. Its queue is fixed to 2048 deduplicated actors and rejects overflow without a full-table lookup. Main-thread updates drain this fixed queue, resolve handles afresh and process a fixed number of doors/nearby observations; there is no added per-frame world/character traversal or native spatial query. A fixed sample array per door has a fixed identity hash lookup and expiry. No raw actor/building pointers survive a callback. Capacity failures log and skip safely.

Independent native review established that the SDK's spatial query `max` arguments do not bound scanning or allocations. That primitive was removed completely. Independent geometry review also found that joining a hysteresis anchor to a later position invents a straight trajectory around a door edge. The core now verifies the actual consecutive segment crossing the finite aperture; its anchor supplies only the return target. Both regressions have production tests/contracts.

Geometry uses native closed/inside/outside door markers and the current closed leaf's own mesh bounds transformed to world space. Native semantics and export mappings must be verified against installed KenshiLib and actual Steam executable before the movement adapter is considered ready. Large/nonfinite/ambiguous geometry is skipped.

The pure production core is tested natively with VC10, first observed RED then GREEN. Native binding verification, source review and live gate/door, save/load, skins, performance and memory checks are separate acceptance stages. Static/pure tests do not establish gameplay or leak-free runtime acceptance.

Settings apply live, optional MCM, standalone INI beside DLL. RU/EN/ZH gettext MO runtime, PO source. Detailed logging last, default off; errors always. No restart flags where no restart is required. v1 remains v1 through prepublication corrections. No installation or publication in this delegated task.
