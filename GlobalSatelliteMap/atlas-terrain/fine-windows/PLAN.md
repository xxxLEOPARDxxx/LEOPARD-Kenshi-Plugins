# Bounded visible fine-window planner

Scope is this new directory only. No Main/thread/cache/engine or existing helper
edits. Use the existing projection policy's untranslated-density and worst-phase
admission concept, with the actual294912m global extent and new integer grids.

1. Test the production API before implementation: 256px keys with9/3/1m spacing,
   exact logical73728m tile boundaries, finite rectangle/screen/epoch checks,
   clipping, density selection and panning at unchanged width/height.
2. Implement a C++03 pure planner with at most128 jobs. Desired spacing is1m
   below3m per screen pixel,3m below9m, otherwise base9m with no fine job.
   Admission reserves `(ceil(width/span)+1)*(ceil(height/span)+1)`, capped by
   world-grid axes, before any enumeration. If1m cannot fit, try3m; otherwise
   retain base9m. Clipped visible bounds never promote a level.
3. Require explicit current source/material-stamped GPU reservation: retained
   and pending bytes, source peak, raster peak, actual per-window bytes, total
   slots/unavailable slots and a caller cap no higher than192MiB. No implicit
   empty192MiB pool. The source plan must be admitted and nonzero.
4. Sort visible jobs before optional one-window apron. Apron uses spare beyond
   the worst visible phase only. Keys carry full source/material64-bit stamps.
   Publication helpers require exact successful completion; no function evicts
   or withdraws the existing coarse fallback.
5. Run actual-body /O2 and /Od tests and a small set of compiled regressions.
   Pin the unchanged policy dependency, document costs/ownership/limits and
   freeze the exact completed implementation for independent review.
