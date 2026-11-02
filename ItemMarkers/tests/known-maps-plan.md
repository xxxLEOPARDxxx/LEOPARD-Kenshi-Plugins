# Known location maps — implementation plan, 2026-11-02

Goal: the existing lower-left eye also marks a location map only when every
listed location is known, with at least one genuine current location. Unknown,
missing or unresolved references keep the eye off. MapsKnown is independent of
Blueprint, default on; Debug remains last/default off. No release/global catalog
version changes, install, UI, personal INI, commit or publication in this stage.

The original list is required: native selectTowns clears/rebuilds townsHandles,
removes already-discovered towns and can randomize a subset. Constructor leaves
that array empty; getTownsDiscovered only returns a per-consume counter. Never
call selectTowns, consume, tooltip or reveal helpers from marker code.

- [x] Pin the actual installed KLib export slots/Steam504b byte windows for
  constructor/select/consume/getter/tooltip, discovery getter and town lookup.
- [x] RED actual policy/caller tests for zero-town, multi-town unknown, unresolved,
  destroyed handles, knowledge changes and world replacement at unchanged addresses.
- [x] Build a lazy, ephemeral fixed-size town knowledge batch, once per existing
  400ms refresh only when a visible map needs it (creation/MCM defer to that cadence).
  Native getAllTowns is its existing things list; group current town data/known
  status in a fixed hash table. Resolve only each map's original towns references.
  No saved native town pointers, growing world cache, native array writes or
  per-frame allocations/traversal. Limits/faults fail closed.
- [x] Share the blueprint eye's asset, geometry and overlay creation. Track the
  boolean state per live icon. Refresh maps outside trade too; handle resolution
  precedes item access; unchanged state performs no widget creation/deletion.
  Existing stolen/trade/grade/blueprint flows remain covered.
- [x] Add MapsKnown settings and EN/RU/ZH messages, source defaults and player
  README. Produce/validate source catalogs and local MO test artifacts only.
- [x] GREEN full feature/body tests plus compiled mutants; measure bounded host
  changed/unchanged work without claiming game FPS. Build local candidate, verify
  imports/exports/source hashes, hand off for independent root review.

Completed candidate verification: see known-maps-review.md and
known-maps-native-report.json. Independent review and actual game acceptance
are parent/root follow-up, not part of this agent's completion claim.

Public adapter shape: MapKnowledgeBatch owns bounded scratch state;
MapLocationsKnown(Item*, MapKnowledgeBatch&) reads it; RecheckDynamicMarks
reuses one lazy batch for the current visible-icon cadence. All implementation
and verification files stay under ItemMarkers; the shared build output is a
local candidate only. No approval ceremony is required by the user's override.
