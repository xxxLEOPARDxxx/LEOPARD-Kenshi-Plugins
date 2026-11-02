# Satellite maximum zoom and native local-light acquisition

Private development only. No public release, new world capture or save overwrite.

## Maximum zoom

The live map projects each primary cell to4096x4096 pixels inside a2160x1044
viewport. The former96MiB/56-detail policy selected source2 despite density1.
The shared192MiB/128-detail policy now selects source1/density1 in the real
game:60/60 visible windows,36 apron windows,115 resident images. After settling,
117 reads/122803200 bytes remain unchanged through a50x10-pixel map pan.
No global satellite error is present in this run. This establishes the measured
viewport case, not every possible viewport or visual filtering condition.

Root freshly reran56755 quality checks and seven compiled functional negative
controls. An independent reviewer reran56755 checks and reviewed all cache/plan
array bounds. The full viewer suite, native water caller/worker tests and
previous terrain/water HDR oracles also pass. Owned terrain payload189006000
bytes plus water3408256 bytes remains below201326592 bytes; driver residency
and retained native sources are separate.

## Real local lights, before rendering them

Installed571904-byte development DLL SHA256
`3a4b1635c7c27c28fd88dc2bd7c293ce31d1ad617d5d497e93cba16e5f82793b`
successfully reads52 registered lights,31 eligible and21 hidden, at the paused
test scene's01:09. Native gain1 and flicker time8709 remain stable. Copied scene
origin(-51296.7,0,3071.81) equals the simultaneously reported native water offset.
The first four copied light positions/radii/powers remain stable across reads.
This is actual native acquisition, not rendered lamp acceptance.

Acquisition runs only with Debug enabled while the map is visible and the
existing cancellable terrain/source validation has succeeded. The build proves
all seven local-light source pins are covered by that same worker. Native
registry discovery is limited to1Hz; summaries to5s. The bridge latches a read
failure until scene/generation/OFF reset. Hidden/world changes retire copied
values. No native object pointer is retained by the snapshot.

4335 collector/backend checks,16 compiled functional mutants and full adapter
compiles pass independent review. Root freshly ran20 extracted main/Shared
guard tests and10 actual passive town-header tests. Initial review found two
unguarded native reads; both were corrected and independently exercised before
installation. A separate bounded TownList census is covered by59 checks/nine
compiled mutants, including optimized publication rereads; its next live use
is still pending at this checkpoint.

No matching existing town proxy was found through the known map markers in
this run. The paired town/node coordinate proof therefore has not succeeded;
equal origin/offset alone does not authorize a guessed GST translation. Local
HDR rendering stays unconnected until that mapping is established.

## Short resource measurement

After the max-zoom pan,181 process samples over approximately180 seconds show
private bytes7921438720..7921520640 (range81920), working set
3978502144..3982880768 and handles14630..14636. This is a short paused-map
resource observation, not FPS evidence or proof of no leaks during extended
gameplay/save/load. CSV and raw log are under
`game/_codex_work/global-satellite-20261102/water-integration/` as
`quality-local-idle-memory.csv` and `live-quality-local-first.log`.

Buildings, existing distant models, lamp rendering and shadows remain unfinished.
