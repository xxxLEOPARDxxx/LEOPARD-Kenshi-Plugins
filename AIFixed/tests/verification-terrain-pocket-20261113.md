# Local candidate: terrain floor and rounded Lagoon supports

Not a release or a gameplay acceptance. The prior installed944F0C5D build was
explicitly rejected by the owner for the actual Lagoon actors.

## Captured causes

- Guard1/15/98510744:6/3684570368 at(18773.75,254.39,47727.40): native exterior
  candidates134.50/134.85/136.06 are below authenticated base terrain139.968.
  The prior strict0.3m agreement rejects these candidates. Other sites fail
  the existing downhill-direction constraint, which remains intact.
- Drone1/61/2046386688:3/2518066432 at(18780.78,260.48,47723.61): current
  physical normalY=.846 agrees analytically with the actual rounded COL013
  support endpoint, not a flat city deck. A nearby face exists on its cap;
  existence alone neither proves nor disproves a connected exit.

## Changes and safety bounds

- Small0.3..1.0m downward nav/terrain discrepancy is corrected only for the
  platform branch, with an actual group2 terrain ray hit within the explicit
  1m ray segment, walkable normal, terrain/physical contact agreement and a
  repeat exterior query matching the original nav point within0.1m.
- Rounded footing with0.60<normalY<0.95 needs a valid nearby face matching the
  current Havok face, physical/native positions within0.5m, and a positive
  native local-confinement query with threshold3.0HK metres. Flat decks,
  ordinary connected ramps, unknown/busy queries remain excluded.
- The audited native query uses the native HK position/radius and a
  nonblocking shared changeMutex lock. An inner SEH boundary lets RAII release
  that lock before a native fault suspends recovery.
- Cap limit remains14m; confirmed steep-support limit remains18m. Candidates
  must leave the supporting slope, stay in the loaded zone and on the same
  gate side, avoid water/barriers and pass full physical standing clearance.
- Sampling frequency/2048-entry history/16-item batch/500ms mature probe gate
  are unchanged. New native work runs only inside mature probes, at most2/s;
  it is not a per-frame world scan. Native search uses finite working storage,
  but no FPS or leak guarantee follows from static and offline checks.

## Verification

- RED: captured lower guard and rounded cap cases reject with the prior code.
- GREEN: extracted landing flow74 checks, pocket wrapper9 including real
  Windows SEH fault and lock release. Native queries are explicitly fake in
  these policy tests; their outcomes do not demonstrate live walkability.
- Existing policies217, guard cycle20, retained capture trace59, beds26,
  gates85, lifecycle/debug58, settings/locales171, live-delay parsing15.
  Total734 checks pass. Captured trace ordering/time remains synthetic.
- Native clock audit; actual COL013 geometry; exact72-instruction island
  wrapper and485-instruction resolvePoint comparisons on Steam and supported
  RE GOG pass. Vanilla GOG is only an offline comparison image.
- VC10x64/O2/GL/LTCG build passes. All117 KenshiLib imports exist in the actual
  complete10131-name export table; new island query is native slot5006.
- Independent review requested physical/native position and face-key ties;
  both implemented and checked. No remaining review blocker.

Still required: actual recovery of the reported guards/Drone in Lagoon;
native pocket result, destination clearance and gameplay FPS remain unmeasured.
