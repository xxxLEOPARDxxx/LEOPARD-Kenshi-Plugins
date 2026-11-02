# Bounded native-water integration, 2026-11-02

User accepted the terrain viewer and authorized unattended completion. Keep the
16 existing GST2 files and their2MiB window index. No world recapture for water.

1. Finish the pan-phase regression and live-check stable detail/edge streaming.
2. Keep the independently reviewed native snapshot passive: current absolute
   water height, actual waterDistant parameters/textures/samplers and game time.
   Source pins are verified in the background worker, never during rendering.
3. Add an optional linear-HDR terrain output to the existing relighter without
   altering its default exposed RGBA8 path. One256-square HDR work texture and
   bounded scratch are sufficient for the fixed-size map windows.
4. Compose the existing native DISTANT/NO_REFLECTION water helper in HDR, then
   apply the existing native exposure once into the owned RGBA8 display image.
   Preserve actual native mip ranges, including a one-mip specular cube; do not
   generate mips or substitute terrain resources. Validate format/state/aliasing
   and inclusive GPU payload under96MiB. No default or guessed water values.
5. Add a live MCM water toggle and matching EN/RU/ZH text. Water-off must avoid
   snapshot work and retain the original terrain path. Invalid water state logs
   an error and leaves terrain visible. Reuse native snapshot/sampler/COM owners.
6. Independently test/review GPU composition and ownership. Then use the real
   paused scene to inspect native values, mips and water off/on/off. Test actual
   game time separately from wall clock; record FPS/memory as measured limits.

Loaded/unloaded building models, their LODs, local lights and shadows follow as
separate render layers. A terrain/water result is not completion of those layers.
