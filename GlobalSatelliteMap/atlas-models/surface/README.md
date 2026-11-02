# Reusable model working surface

This isolated C++03/D3D11 helper preserves the immutable streamed terrain cache. Each affected window starts by copying its pristine three cached planes into one reusable working surface. The caller then replays the **current** model set into the working surface with `AtlasModelDecode`, shades its SRVs synchronously, and reuses it for the next window. A removed model disappears on the next successful copy/replay. There is no per-cache-entry surface, stored model overlay, shader, thread or game/native engine call here.

```cpp
AtlasModelSurface::Layer surface;
HRESULT hr=surface.Init(true,device,allOtherLiveGpuBytes);
// Each affected window; borrow these three SRVs only during Copy.
hr=surface.Copy(true,pristineBaseSrvs,allOtherLiveGpuBytes);
if(SUCCEEDED(hr)) {
    // ModelDecode outputs = surface.Texture(0..2), existing SRVs = Output(0..2).
    // Replay current models, then pass Output(0..2) to the terrain lighting path.
}
surface.Clear();
```

Only `S_OK` publishes a usable copy; `S_FALSE` means disabled and does not establish fresh content. Do not shade/replay a failed copy. A device failure during the three asynchronous copies may leave the **working surface** incomplete; discard it normally with the enclosing device lifecycle. Pristine source resources are never write destinations. This helper neither batches models nor chooses them, proves coordinates, changes exposure, or integrates a main caller.

`Init(bool,ID3D11Device*,callerBytes)` creates three fixed 256x256 single-mip/single-slice DEFAULT textures and three SRVs. Formats are material `R16G16B16A16_FLOAT`, world normal/explicit coverage `R8G8B8A8_UNORM`, absolute world height `R32_FLOAT`. Outputs have only `D3D11_BIND_SHADER_RESOURCE`; decoder-owned scratch copies into these DEFAULT resources, so output UAVs are unnecessary. `Texture(i)`/`Output(i)` return borrowed pointers, null for i>=3, valid until Clear/destruction. Init twice is refused. Clear is idempotent; failure at any of the six resource/view creation steps releases all earlier ownership and supports retry. External/context-held references remain their owners' responsibility, including their budget until released.

`Copy(bool,SRVs[3],callerBytes)` validates every plane before touching context or texels: same device, exact SRV dimension/format/mip range, actual texture 256x256/one mip/one slice/exact format/non-MSAA, DEFAULT or IMMUTABLE, SRV bind, no CPU/misc flags, no other bind categories beyond optional RT/UAV, pairwise distinct resources and no alias of any owned output. A one-mip view cannot hide a multi-mip texture. Null/missing last plane, wrong device, shapes or aliases refuse the whole copy before any first-plane write.

All six shader stages' destination SRV bindings are temporarily unbound using the existing reviewed `SavedOutputReads` and restored on scope exit. Source read bindings may remain. CS/OM UAV, RT and DS writable aliases of a source or output are refused before mutation; this helper never unbinds/rebinds writable native bindings or resets their hidden append counters. `SavedPredicate` disables and restores predication around the copies. No shader, constants, sampler, draw, dispatch, stream-output or native/Ogre state is changed. Context state uses existing helpers, not a second renderer or new state framework.

Copy is bit-exact. It does not filter, normalize, clamp HDR, derive height, replace NaNs, infer coverage, or invent a missing ground value. The fixture includes material >1, explicit coverage zero, height NaN payload and signed zero. Semantic validity was established by the cache producer/decoder, outside this copy layer.

The explicit GPU texel payload is exactly **1,048,576 bytes** (256x256x(8+4+4)), with no constant buffer or private scratch. Both Init and Copy require `callerBytes <= AtlasView::GpuCap - PrivateBytes`; `AtlasView::GpuCap` is the authoritative 192 MiB cap. Caller accounting includes all other live/pending/retired cache/render resources, and excludes this surface exactly once. Current planned combined payload is 196,086,608 bytes; adding the separate future loaded raster 1,048,800 gives 197,135,408, both below 201,326,592. These are explicit payload ledgers, not total driver/process allocation measurements.

Each enabled successful Copy submits exactly three GPU resource copies totaling 1 MiB and retains inputs only through that synchronous CPU call; the command context/driver owns command lifetime. It allocates no C++ heap objects or D3D resources per Copy, reads no disk/INI, performs no GPU readback, and uses fixed bounded descriptor/alias/state queries. Three 6x128 read-state scopes use 18 KiB of pointer storage on Win64 plus bounded locals and temporary COM references. Call only for affected windows, within the caller's existing four-shade/tick limit; this helper introduces no per-frame polling. OFF Init/Copy returns S_FALSE before inspecting even sentinel device/source pointers. Live CPU/GPU performance and complete integration lifecycle remain to be measured.

Run `./run_tests.ps1` from this folder. This compiles/runs the complete isolated WARP suite, actual Init/Clear extraction with an allocation-failing device seam, full Kenshi SDK/native/model-decoder adapter (compile only), and all compiled mutants. No game modules are loaded or native game functions executed by the tests.
