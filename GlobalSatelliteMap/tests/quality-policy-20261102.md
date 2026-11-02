# Measured maximum-zoom quality policy, 2026-11-02

Scoped change: `AtlasPolicy.h` has the single192MiB GPU bound, `AtlasWater::Cap` aliases it, and the window policy has128 detail +16 fallback slots. Windows remain256 square, CPU cap64MiB and renderer work at most four windows per200ms. No main renderer/caller, UI, INI, build script, shader, cache encoding or16-file GST2 layout is changed here. Root owns candidate rebuilding, installation and runtime acceptance.

Root's existing live measurement supplies primary-cell4096x4096, viewport2160x1044, densityLevel1 but sourceLevel2 under the former56-detail/96MiB profile. The new production-policy fixture uses those measured dimensions with explicit synthetic pan phases; it does not invent a measured pan origin. The phase500 witness reproduces the old visible24 result.

Observed **RED before the header edits**:

```
primary4096x4096 viewport2160x1044 source2 density1 visible24 planned35
baseBytes94634160
FAIL measured4096 primary2160x1044 viewport retains densityLevel1
```

Observed **GREEN after the edits**:

```
primary4096x4096 viewport2160x1044 source1 density1 visible60 planned96
baseBytes189006000
Quality policy PASS56755 checks
```

The test covers every256-pixel subwindow phase separately on X/Z and diagonal, primary seams and independent4095/4096 rounded boundaries. It compares every actually visible enumerated key with the admitted plan, including the opportunistic apron bound. The theoretical worst measured visible phase requires60 detail slots, exceeding the former56. The fixed128 pool keeps this viewport's sourceLevel equal to densityLevel1 without changing sampling/filtering. The adjacent4096/4097 band naturally requests densityLevel0; the fixed pool may coarsen that unmeasured band to1, and the test requires pan stability/bounds rather than claiming universal density preservation. Larger viewports and other zoom bands can still coarsen when their explicit fixed capacity is exceeded.

Inclusive current payload:

| Item | Bytes |
|---|---:|
|144 windows, material/normal/height+owned RGBA8 output|188743680|
|Default256 RGBA8 scratch+176B constants|262320|
|Base total|189006000|
|HDR/water/tone private pipeline|3408256|
|Combined|192414256|
|192MiB bound|201326592|
|Headroom|8912336|

The earlier188743856 base estimate omitted default scratch. All helpers now use the same authoritative cap, including exact/+1-byte rejection for ordinary terrain, HDR terrain, water and pipeline. Existing whole-cell budget tests intentionally accept configurations admitted by192MiB and still refuse a third80MiB cell;64MiB CPU file-read limits stay unchanged. Borrowed native sources are not duplicated; driver overhead/queued retirement is separate from this owned-payload ledger.

Run `tests/run_quality_policy_tests.ps1`:56,755 checks PASS and seven separately compiled functional mutants rejected (former56 detail slots, split water cap, omitted default scratch, and each of the four +1-byte guards). Compiler failures are not counted as mutant rejection. Mutants live in isolated copied-header trees under ignored `build/quality-policy-tests`, never modify production headers, and make no game calls.

Fresh regressions after the cap change: terrain32policy+53reader+92WARP and six mutants; water2source+11policy+163WARP and eight mutants; pipeline21policy+126WARP and ten mutants. Default terrain/water/HDR/tone shader bytecode stays unchanged. The full viewer suite passes strict metadata/worker/geometry/clock/pan/adoption/supersession/reposition checks,3MO source coverage,563 extracted caller lifecycle/scheduling checks and45 water-worker checks plus their functional negatives. Caller fixtures now derive array sizes and bounded full-pool recovery cycles from WindowSlots rather than a stale72 constant.

No source recapture, disk-cache expansion, native call, game/UI action, installation or commit was performed by this scope. Header fixtures/native oracles establish bounded behavior and pixel regressions, not runtime visual acceptance, FPS, actual VRAM residency or long-session leak absence. Current root README and build/candidate packaging remain root-owned; archived96MiB verification remains historical.
