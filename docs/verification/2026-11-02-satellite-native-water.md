# Private satellite distant-water layer

The optional Water setting uses the current native waterDistant material,
absolute water height, game time, five existing textures and their real sampler
states. Its five source files are checked in one bounded16MiB worker job;
failure disables only water. The terrain manifest, cache and source index stay
intact. The default is off; errors are always logged, detailed messages optional.

The reusable256-window pipeline renders terrain to linear HDR, applies the
native DISTANT/NO_REFLECTION material and runs native exposure once. Additional
owned payload3,408,256bytes; combined current bound98,042,416bytes <96MiB.
No atlas recapture, new large cache or native resource-loading call is added.

Fresh verification:

-293 native-snapshot checks plus seven functional mutants; complete VC10 native
  adapter and all mutant adapters compile. Exact25 exports, six passive-layout
  windows and five source pins pass the independent native audit.
-19 GPU policy and126 real WARP checks; ten independently compiled functional
  mutants rejected. Raw HDR agrees with the native fixture exactly; composed
  water and one exposure differ by at most one UNORM step.
-45 worker/height-metadata checks and four functional mutants;293 extracted
  main-caller checks and one sampler-only paused-refresh mutant pass. Existing
  viewer geometry, worker, lifecycle and locale checks also pass.
-Independent pipeline/main review CLEAR, including failure latching, world
  generation, OFF cleanup, sampler-only invalidation and four-window fairness.
-Installed548864B candidate SHA256
  `cc80f27ab1b589d89d93f7fe0de66132f27697483c29f19f04bc0e757e6e0418`.
  Backup and before-file hashes are under the private before-water-integration
  directory. Existing personal settings retained; Water=1 added only for QA.
-Live PID7604, paused SAT-render-proof-20261101 at01:09: source validation and
  native snapshot succeeded. Observed waterY100, texture mips8/12/12/5/10 and
  native depth bias-1/-1. The ocean layer replaced submerged terrain;16 fallback
  windows completed, no water/viewer error. These values were read, not assumed.

Live OFF/on/OFF, daylight appearance, continuous animation, frame time and long
memory runs remain pending. The bounded four-window refresh does not establish
simultaneous animated-water epochs over the whole viewport. The geometric
height mask does not reproduce native depth-biased coastline rasterization;
near water/reflection/rain/scum are outside this distant-water material layer.
Buildings, distant geometry, local lights and shadows remain separate work.

The subsequent max-zoom quality task must address observed sourceLevel2 versus
densityLevel1. The current96MiB cache cap can still coarsen native detail.
