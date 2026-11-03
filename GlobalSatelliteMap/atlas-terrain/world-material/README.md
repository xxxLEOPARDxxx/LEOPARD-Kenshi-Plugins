# World material CPU planner

Read [DESIGN.md](DESIGN.md) for the concrete bank/staging/provenance/paint/cap
boundary. This is an authenticated offline base-snapshot plan, not a GPU owner
or active-native-source approval.

`WorldMaterialPlan.h` is C++03 and allocation-free. Init validates the existing
compiled64-region catalog once for a nonzero epoch. Build accepts any of1024
real coarse cells, produces deterministic combined/per-SID texture role maps,
original selector/15-row CB offsets and byte-budget candidates, and leaves the
previous output unchanged on invalid requests. All five native slots/tree
identities and actual role aliases survive. No engine/D3D/file call is added.

Run bundled Python `run_tests.py` with VC10/SDK7.1. It hashes the existing catalog,
snapshot, shader ABI sources and66 actual case asset files, generates an
independent expected table, runs optimized/unoptimized tests and eleven compiled
negative controls. Artifacts remain in `_generated`; no game/INI/install writes.
`--skip-mutants` skips only the negative controls.

The planner's source/accumulator cost is for256-square staged jobs. Every other
live/pending/retired GPU byte is a caller charge. Its candidates do not prove
shader staging, source winners, multi-cell paint/scissor parity or Main readiness.
