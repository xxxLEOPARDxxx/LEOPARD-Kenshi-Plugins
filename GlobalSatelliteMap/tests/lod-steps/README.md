# Native fixed-step quality gap,2026-11-04

RuntimePID26248 completed ordinary sourcelevel0 atx2..x4: this was finished9m
terrain, not a stalled worker. Atx4 it stretches one9m texel over2display pixels.
The former96visible3m limit rejected the2160x1044 viewport atx4 (worst112),
x3(180),x2.5(242),x2(378). x2 also sat exactly on the old strict density<9 gate.

Actual production policy now admits at most512visible windows, including the
exact9m/pixel boundary. The1m upgrade still requires the ORIGINAL96whole-view
limit, so the bigger cache does not turn an existingx8three-metre view into
hundreds of one-metre cold jobs. Real key/lattice validation remains unchanged.

`run.py` tests bothGPU1/8GiB underVC10/Od and/O2, using the native projection
formula and viewport observed in the log. Every resulting key is checked by the
actual FineJob mapping. It also covers above9boundary refusal, the original1m
quality threshold and oversized-view refusal. No game/atlas files are touched.
The earlier production96limit RED matrix is retained inred-before512.log.

This fixes admission, not cold rendering speed: x2needs338three-metre windows
in the measured sample; x8needs28. Both are produced progressively. Actual
runtime timing/visual acceptance and source-bank reuse are separate checks.
