# Local lighting integration checkpoint, 2026-11-02

Private development only; this is not completion of the satellite map.

The global map can now add existing native point and spot lights to the streamed
terrain, before native water and the single exposure step. Factory LocalLights is
OFF. RU/EN/ZH MCM applies it live; Debug remains last and defaults OFF. No source
world zone is activated and no light is invented for an unloaded town.

A live paired read with a nonzero floating origin proved the coordinate conversion:
raw scene position + (-51296.703125, 0, 3071.80664) = captured absolute coordinates.
The caller requires matching generation, scene, origin, town-node and shared-camera
data. Failure retains terrain and disables the optional contribution. Snapshots
are copied at most once per second; exact unchanged values do not dirty windows.
The existing shared four-window/200ms shading budget remains in force.

Actual paused SAT-render-proof-20261101, Hub at01:09, PID8632:52 registered lights,
31 active,21 hidden; localReady=1/localProof=1. Max-zoom plan sourceLevel=1,
densityLevel=1,50/50 visible windows,34 apron,144 resident. After refinement,
reads=246 and localRenders=15/localBatches=15 remained unchanged. Native DistantTown
snapshot also succeeded with one packet; its shader support is not claimed verified
by this diagnostic. Nearby ordinary building census reported Object_VP/Object_FP_40.
No building geometry was displayed by this integration.

Control restart with LocalLights=0 retained terrain, localRenders=0 and localReady=0.
The captures were panned by(-131,-120) pixels relative to each other; comparison
explicitly registers that integer translation. Median terrain channel difference1;
2861 warm pixels are brighter with local lighting ON across1394250 compared pixels.
This verifies visible contribution, not whole-frame pixel parity. Original images,
comparison script and logs are in D:/DEV/Kenshi/research/gsm-local-live-20261102.

The first91s process sample overlapped a game autosave and varied by109928448 bytes.
A subsequent181-sample/182s idle sample was exactly8031244288 private bytes;
handles14631..14633. This is short paused-scene evidence, not an FPS measurement or
proof against all long-session leaks. The full process includes other plugins.

Owned texture/buffer payload: base windows189006000 + water3408256 + local526416 =
192940672 bytes, below the existing192MiB bound201326592. Driver overhead and
retired/native resource lifetimes are separate. Copied light history is fixed and
bounded4096 entries; no per-render GPU allocation or full-world scan was added.

Fresh production-extraction tests cover mapping/revision/caller routing, OFF,
generation/scene changes, proof refusal, failure fallback, shared memory accounting
and four-window scheduling.108 revision +25 mapping +47 caller checks and12
compiled caller mutants pass. Native shader/HDR/state tests are documented by the
isolated light and water components. Final independent caller reviews:
D:/DEV/Kenshi/research/gsm-local-composition-review-20261102/report.md and
D:/DEV/Kenshi/research/gsm-final-main-review-20261102/followups-review.md.

Debug-only model diagnostics retire borrowed packets before logging copied values.
Two one-shot reports cover a distant source and the nearest current town. Full
copied town IDs/positions/proxy states are logged once per generation, independently
of proxy availability; no functions update town Y. The catalog is bounded4096,
has no per-frame source work and is for matching real distant assets. These are
diagnostics, not complete loaded/unloaded town rendering.

Remaining acceptance: moving/flickering lights, actual unpaused GPU/CPU/FPS costs,
repeated live toggles/load cycles and longer memory observation. Loaded/unloaded
building coverage and shadows remain separate unfinished work.
