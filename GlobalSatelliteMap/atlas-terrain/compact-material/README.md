# Compact native terrain material control

Isolated two-SID source owner and WARP material control. No Main/probe/installed-game integration, new MCM setting, cache rewrite or universal material batcher. This produces the native **packed GBuffer**, not a display-ready diffuse/RGB preview. Root owns the later native Sun/HDR composition and actual images.

Use bundled Python `C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py`. It has Pillow; the default system Python may not. The runner configures VC10 x64/SDK7.1 and uses fxc10.0.26100.0. Outputs stay in `_generated`; `--skip-mutants` skips negative controls only.

## Fixed source/capture contract

Region3,3/cell10 has selectors18,4,0,0 and SIDs2743-Newwworld.mod /48690-Newwworld.mod. The selected native roles share6 unique BC3 diffuse and6 BC1 normal DDS sources. `case-sources.json` lists exact paths/content hashes. Full12-mip DDS file bytes are hashed; the actual compact arrays preserve the audited native-geography9-level resources. Overlay.3.3 is1024² and colour.3.3 is2048², one array layer each with complete11/12 mip chains; global1024² blend retains11 levels. No padded92-layer bank or four-pair painted pool is allocated.

`_source/GeographyTerrain.hlsl` and GeographyVertex.hlsl are exact pinned local snapshots of the previously audited native-derived geographic programs. They are ignored generated files, not redistributed game assets. `prepare_sources.py` reconstructs them from the installed game and existing authenticated project generators in its own ignored mirror; it never writes game files or the probe's build directory. Their source and compiled bytecode remain unchanged: PS4b3b71ccb9c75b79791b9cf3bef5f858ba9c53151a9cfc0d65cd2e7d113fdebe /VSdbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238. There is no generic diffuse replacement. Native ordered biome blending, wetness, cliff phase/gradients, normal packing, gloss and depth remain literal. The generator checks installed original terrain/gbuffer/wet/shadow sources too.

The exact same shader can consume compact sources because role indices are remapped to0..5. t7 aliases t0 and adds no image. Set paintedTileBase=(-36864,-36864), which selects layer0 with the same native36900m UV denominator/18m border inside this one painted region. geoSelectorRegion=(-36864,-36864,4,4); only selector texel2,2 is nonzero. Unused15-SID constant/distortion slots remain zero; the existing VS/PS225-float4/15-distortion ABI is retained.

The generated PinnedParameters/PinnedDistortion come from the explicitly frozen historical active-biomes snapshot, with zero-fade isolation. They are **fixture values**, not live material/weather state or active-resource proof. A runtime adapter must copy/recheck current native parameters and all active source winners before using the helper. `ApplyRoles` changes only the two selected rows' texture-role fields. No new Boolean authorizes unrelated `verifiedSources` callers. Arbitrary finite native inputs still need canonical domain proof; this owner adds no shader clamps or formula fallback.

## Public owner and binding API

All D3D operations require the caller's serialized render thread/immediate-context path and live same device. No helper thread/native/Ogre/file calls.

1. `Inputs::Init(device,otherOwnedPayload)` checks total against90MiB and the unchanged192MiB global cap **before** allocation. It owns seven textures/views: t0BC3array6×2048/9mips, t1BC1array6×2048/9, t2overlayarray1×1024/11, t3colorarray1×2048/12, t4interior1×1RG32F zero, t5blend1024/11, t6selector4×4RGBA8. t7 is an alias, not an eighth texture. Initialization failure clears preceding owners.
2. Upload one prepared source at a time: `UploadDds(index,data,bytes)` for12 pinned encoded DDS blobs, and `UploadRgba(index,data,bytes)` for overlay/color/blend tightly packed RGBA8 mip0. RgbaPins validate the decoded pixel hashes. PNG decoding/source winner validation belongs to the caller's bounded preparation job. No input pointer/byte payload survives an upload; encoded+decoded+pending buffers all remain the caller's CPU ledger. The C++ test holds only one16MiB maximum blob plus<1MiB geometry, not all50MiB DDS plus24MiB PNG data together. Python fixture preparation is offline and is not a claimed64MiB runtime decoder.
3. Each upload validates hash/shape before writes, refuses duplicate/out-of-range/late calls and temporarily disables/restores exact native predication. Device failure after submitted writes latches broken until Clear; no retry/publication from uncertain data. Uploads allocate no textures/shaders/staging and perform no file discovery. Hashing is bounded per initial source upload, not per pixel/frame/Tr.
4. `Complete()` exposes sources only after all15 uploads and healthy device. `View(0..7)` returns borrowed SRVs while ready; `OwnedGpuBytes()` reports83885956B for the allocated source owner and0 after Clear. Pending/unready sources remain fully charged. Retain external views only under the caller's own reference/payload accounting; detach all caller bindings before teardown. No shutdown/hot-unload/all-leaks claim.
5. `_generated/Shaders.h` supplies native VertexShader/PixelShader and reflected CB byte offsets/sizes (448B VS,3856B PS). The harness shows exact layout/sampler/CB setup. Geometry is the separate fixed-geometry65²40B packet/8572-index strip API; morph0, identity world and all15 VS cliff slots remain unchanged.

The source owner uses83885956B. The control harness adds676000B four-chunk VB,17144B shared IB,4304B CB,786432B GBuffer targets and bounded individual readback/probe resources; it remains below90MiB declared payload. No runtime driver-residency/FPS guarantee follows. The present full viewer reservation189006000B cannot coexist with this owner under192MiB; future Main integration must replan/evict its window reservation rather than add a hidden source budget.

## Actual control results

The runner hashes the real fullmap.tif and reads only133² source heights7358..7490; fixed source vertices remain18m. The actual four65² globally aligned patches cover these256² controls with the same native monolithic material shaders:

- control3: spacing3m, center(-13824,-13824), bounds[-14208,-13440]².
- control9: spacing9m, same center, bounds[-14976,-12672]².
- shift3: spacing3m, center(-13632,-13632), bounds[-14016,-13248]², an even64-pixel shift.
- control1: spacing1m, center(-13824,-13824), bounds[-13952,-13696]²; same source geometry/materials at the finer requested direction.

Saved native packed RGBA8/normal-alpha RGBA8/radial R32 planes and exact VS/PS CB bytes are named per control in `_generated`; controls.json records bounds/formats/camera/reference hashes. Radial represents distance/farClip with cameraY10000/far20000, not world height directly. Normal alpha is the literal native zero terrain alpha, not accepted coverage.

Across36,864 overlapping same3m samples, packed material/normal max delta1LSB (196 changed components), reconstructed height max0.00243142879m. Across7,396 common3m/9m samples, heights differ at most0.00238656325m; material/normal max47LSB/28802 changed components are recorded, **not** equality failures, because material texture derivatives differ with raster density. Actual height errors for all four controls are bounded by the direct TIFF anti-diagonal checks in the test log. These controls do not prove region/world-border seams, current native world admission, native Sun/display parity or a complete map feature.

Tests separately read actual BC3/BC1 layers against independent DDS decoding, verify generated final PNG mips under foreign predication, preserve a seeded context, cover cap+1/incomplete/double/hash refusals, repeated resource references and actual allocation failures. Full counts/functional mutants and frozen hashes are recorded in verification.json. Compilation failures do not count as mutant kills. Independent review is separate from this implementer evidence.
