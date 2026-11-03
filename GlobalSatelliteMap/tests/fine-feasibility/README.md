# Fine feasibility regression

The live x2.25 view (primary 9207 pixels, viewport 2160x1044) has density 8.00782 m/px. Its real conservative 3 m grid needs 288 slots, above the actual 96-slot limit. It must retain ordinary base LOD rather than disabling both render paths.

`run.py` compiles the production `GsmFinePolicy::CanRefine` and actual fine-window planner with VC10 /Od and /O2. Seven controls cover the live failure, feasible closer zoom, exact 96-slot limit and next 104-slot boundary, invalid source and viewport. The original missing-policy RED is retained in `red-missing-gate.log`. These are host policy checks, not FPS or native GPU measurements.
