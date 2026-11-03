# ItemMarkers: open grade rules and runtime SDK

Goal: let other mods add grades/colours for arbitrary item kinds, including backpacks, without recompiling ItemMarkers. A configuration-only mod must not require a DLL. Dynamic per-instance quality needs a versioned public C API.

Architecture: optional `ItemMarkers/grades/*.ini` profiles describe named grades and deterministic item/material/type rules. A bounded provider registry offers higher-priority per-instance results; no match falls through to configuration and then unchanged native Gear handling. No arbitrary memory offsets, scripts or casts of nongear items. Existing marker styles, minimum display rank and visibility settings still apply. Grade names/count are independent of the six native display ranks (0..5).

Constraints: VC10/x64; copied plain C ABI structures, no STL crossing DLL boundaries; bounded registries; callbacks only on GUI thread; no filesystem/INI work per icon/frame; errors always logged; provider changes coalesce into the existing 400ms visible-icon refresh. Configuration is opt-in, no enabled sample rules. Preserve personal settings, theft/ownership and knowledge-eye code.

## Execution

- [x] Add `shared/ItemMarkersApi.h`, pure bounded registry/rule matcher and production-boundary tests. Prove item/material matching, custom nongear grades, priority/fallback, malformed outputs, capacity, stale handles, reentrant changes and unregister.
- [x] Add cold profile loader, strict bounded validation and deterministic ordering. Validate complete profiles transactionally; reject malformed/truncated files without replacing good state. Provide disabled/example profile and reload entry point.
- [x] Integrate one appearance resolution per decoration. Provider registration/notification requests deferred refresh; redraw only changed visible appearances. Hold callback module references until successful unregister; serialize registry access; quarantine a throwing/faulting provider once.
- [x] Add public SDK examples and RU/EN usage/lifecycle documentation. Add a translated reload control before the final debug setting. Do not increase a prepared release number twice.
- [x] Run focused /Od and /O2 production tests, existing known-map regressions and native linked build/export/import validation. Obtain independent perf/lifetime review; correct findings.
- [ ] Commit/push this scope separately. Runtime validation and release preparation follow only a verified candidate; do not claim gameplay QA from host tests.

2026-11-03 candidate:20,622 pure controls,52 actual DLL/config controls,34 extracted refresh controls per /Od and /O2; six refresh mutants rejected. Known maps10,543 controls and15 mutants still pass;12 native windows/6,139 instruction pins/three locales/56 imports verified. C ABI size and pack restoration compile in C; client example compiles. Independent review CLEAR; provider callbacks are documented trusted/nonblocking/read-only. No gameplay or FPS claim. Linked candidate190,464 bytes, SHAf25c231a06e5263b432d55f88ef01b6d9cd45e0e82c049b1ce86f2a81bf15523. Not yet installed or published.

Review focus: DLL unload during callback; malformed config and colour keys; no false Gear cast for backpacks; notification storms; unchanged-icon widget churn; world/item destruction; module load order; no changes to item stats or ownership.
