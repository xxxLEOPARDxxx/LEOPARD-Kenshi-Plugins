# Stop shading the hidden coarse backdrop

Live E823 source had completed all fine and speculative work (phase4, source GPU
zero, 479 stored windows stable), while model/local raster counters kept growing.
The coarse Draw loop still shaded up to four windows per 200ms behind the fine
overlay, including repeating model raster/decode. This repair changes that
scheduling; it does not hide, evict or discard the existing coarse backdrop.

The guard independently checks current world/capture/material/view geometry,
complete unique anchored viewport coverage, every resident shaded entry's opaque
accepted source, and actual bound visible widget/texture/alpha/projected rectangle.
Center work, draining, missing tiles, pan/resize/context changes resume coarse
refresh. One fixed512-byte coverage scratch and indexed entry lookup add no
growing state or worker. Opaque source tracking fits existing FineEntry padding.

Fine sunlight, local lights and water remain dynamic. Diagnostic coarse/fine
shade counters now distinguish producer completion from continuing GPU shading.

Limitation: accepted alpha255 and finite shader paths preserve opacity; existing
nonfinite shader branches can output transparent pixels. Those rare pixels may
reveal older same-world coarse lighting while refresh is suspended. The coarse
texture and geometry still exist. No claim of GPU-readback-proven alpha is made.

Verification: independent policy/integration review; fresh32 production-policy
and17 extracted presentation controls passed; VC10 /GL /LTCG build and87 native
imports (added existing MyGUI getAlpha), startPlugin/MCM checks passed. Live FPS
and counter deltas still require an installed run. No public release or journal.
