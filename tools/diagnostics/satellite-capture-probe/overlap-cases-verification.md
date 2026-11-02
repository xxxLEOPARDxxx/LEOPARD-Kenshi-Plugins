# Fixed overlap case1 candidate — 2026-11-03

Scope: one additional fixed diagnostic case. Original case0/default remains
unchanged; normal atlas behavior, raster/worker/evidence format, caps and native
lifetimes are unchanged. No game, install, INI, commit, push or publication.

`[Probe] OverlapCase=1` is read only when admitting a NEW `OverlapControl=1`
job. It selects sourceStart(6128,6128), camera(-9360,-9360), primary1,1/region3,3,
same2048 square/18432m/9m footprint. Default case0 retains source(5120,6144),
camera(-18432,-9216). Unsigned values>1, and direct non-control/nonzero cases,
refuse before disk query/directory creation. The job latches controlCase beside
its world identity; both the native terrain block and copied worker block use
that value. Later INI changes cannot redirect either. Invalid selection leaves
the policy output block untouched.

TDD RED: expanded policy test initially failed VC10 C2660 because the original
selector accepted no case argument. After implementation:

- `test_overlap_control.py`:8,388,871 checks of both actual selectors, every
 2048² sample's exact global lattice/checker parity, unchanged normal selectors,
 original default, source start/end and invalid-case unchanged output.
- Same test compiles actual Begin/Block/BeginAtlas/new-request bodies:16 checks
 for job/world latching, coherent native/worker blocks, active INI changes,
 default/original case, normal atlas ignoring the key, and pre-I/O/resource
 refusal. Six compiled latching/admission mutants are rejected.
- `test_atlas_worker.py`:119 actual worker checks;8 existing atlas cases,
 2 complete full-size control cases and4 control refusal cases. Both exact
 headers, all three planes and EOF are read back. Case1 still writes exactly
50,331,680 bytes and never touches a GST writer.
- Full probe `build.ps1`: PASS, VC10 x64 `/GL` + `/LTCG`; all included shader,
 WARP, resource/lifetime/raster/ABI/request/atlas/worker/source-contract suites
 passed. Expected mutants printed their intended failure then passed rejection.
 Existing C4018/C4512 and HLSL X3206 warnings remain; no new warning category.
- Candidate559104 bytes, SHA-256
 `9cf875c2bd3f9e4a5e45d2fb37a34e7709511633edb72f68285cd04951a7d4e3`;
 actual decorated startPlugin,159 native imports present in installed libraries.
- `git diff --check` passed (line-ending notices only).

Performance/lifetime: one unsigned job field, no new cache, native pointer,
thread, target, readback allocation, INI poll or per-frame traversal. Existing
new-job creation path reads the optional case once. Native and worker memory,
cancel/join/quarantine and one-frame limit are unchanged. Each tested selector
uses stack values and returns without mutation on invalid cases. No runtime
FPS/memory claim or successful case1 game capture is made.

Actual previous case0 evidence established the examined edge-only defect,
four geometry columns/six decoded material columns, not a universal halo width
or a completed production atlas fix. Case1 is only the next bounded experiment.
README documents both footprints and that distinction.

Frozen sources/candidate: overlap-cases-frozen.json. Independent review and root
installation/runtime acceptance are separate follow-up steps.
