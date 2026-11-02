# Known map eye candidate — 2026-11-03

Scope: ItemMarkers only. No install, personal INI, version, release metadata,
commit, push or publication. Existing prepared release 6 remains unchanged.

The map eye uses the original `GameData` `towns` list, current `GameDataContainer`
lookup by SID, and each genuine matching current town's `isDiscovered()`. Every
instance must be discovered. Empty/absent lists, unknown/missing/non-town refs,
genuine-town refusal, oversize input, table saturation and native read faults
keep the eye off. The consumed counter and selected handle array are never used.
The native selector, consumer, tooltip and reveal functions are never invoked.

`MapsKnown=1` is separate from `Blueprint=1`. MCM keeps Debug last/default off;
neither eye toggle requires restart. English/Russian/Chinese strings are complete.
The eye asset, tint, depth, alignment and lower-left geometry are exactly shared
with learned blueprints. New icons/MCM redraw wait up to 400 ms for map knowledge.

## Verification

- Native VC10 x64 `/GL` + `/LTCG` build: success, candidate 148480 bytes.
- `python ItemMarkers/tests/known_maps_tests.py`: 10543 checks on extracted actual
  production knowledge/refresh bodies; 15 compiled functional mutants rejected.
  Cases include all/any-known, no refs, absent refs, wrong/unresolved SID,
  destroyed town/item, duplicate unknown/refused instance, no shared world,
  table/town/reference limits, SEH read fault and next-batch recovery, independent
  toggles, hidden icons, cadence, existing inventory icons, same-address world
  knowledge changes, missing eye child, one shared scan for multiple maps,
  unchanged zero widget writes, and retained ordinary-item trade refresh.
- A new refused duplicate-town test first failed the inherited candidate; changing
  refusal to invalidate the complete ephemeral batch made it pass. This prevents
  a known genuine instance from hiding a second refused instance.
- `python ItemMarkers/tests/verify_known_maps_native.py`: 12 actual installed
  Steam504b/KenshiLib windows, 6139 instruction-byte checks, 5 thunk/call edges.
  Original refs, count reset/increment/getter, discovery getter, native town
  admission and tooltip/consume selector calls are authenticated offline.
- Three complete 49-entry PO catalogues compiled to ignored test MO artifacts;
  round-trip GNUTranslations matches every production Tr key.
- Candidate PE has startPlugin/MCM_Describe and 55 imports present in the installed
  game libraries. Hashes in known-maps-native-report.json and known-maps-frozen.json.

## Performance and lifetime

Only the existing 400 ms visible-icon cadence may build a lazy snapshot. Creation
and MCM cannot scan towns; hidden maps, disabled MapsKnown and maps without refs
do not scan. One snapshot is shared across the update, with at most 8192 native
world entries, 2048 distinct town data keys and 256 refs/map. A full-table probe
terminates. The 32768-byte fact table is stack-owned, plus flags/padding; no native
pointer escapes the synchronous call, no persistent world cache, new thread,
per-frame allocation or per-frame world traversal is introduced.

The extracted host fixture measured 10000 warm policy lookups at 0.163 ms with
zero allocations/new scans. This is an optimized synthetic-boundary fixture,
not a game CPU/FPS benchmark. The periodic unchanged UI fixture also confirms
zero new allocations or widget writes. A transition uses the pre-existing
Undecorate/Decorate path; its temporary mark-vector copy and widget creation are
not claimed allocation-free. Active icon entries remain owned/removed by the
existing MyGUI Unlinker; the new eye pointer is cleared on child unlink and never
used to dereference game data. Item handles are resolved before periodic access.

No live process access or game acceptance was performed. Vanilla/modded-skin
presentation, actual maps in merchant stock/containers, save/load behavior,
game FPS and memory growth still require root's live acceptance. Static bounds
and short fixtures do not establish absence of every possible leak.
