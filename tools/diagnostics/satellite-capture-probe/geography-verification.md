# Primary 1,1 geographic material trial

## Local candidate and scope

Full `build.ps1` completed with exit 0 on 2026-11-02. Candidate
`build/SatelliteCaptureProbe.dll`: 334336 bytes, SHA256
`47d2fd8d90218a690068a3168284f43f5a16904be0b5b92085d023708fa0c8c4`,
156 native imports. Full evidence: `build/geography-native-bc-fix.log`, SHA256
`e6032c0b12bc1137b6600cd6c61a6286a3bfcf6bf147ca5736f6f9056ed93b2a`.
This follow-up candidate has not been installed or run in the game. Root
independent review cleared the targeted fix after reading request compensation,
COM snapshots/failure diagnostics and native code windows. A fresh targeted run
in `build/root-native-bc-fix-review.log` passed69 completion checks,2892537 GPU/
DDS checks,65546 geographic cases,90 role pairs and all seven mutants; fresh
render ABI verification passed with15 D3D windows. The candidate remained the
same47d2fd8d hash and334336-byte length. This clearance is for the next bounded
trial, not game runtime acceptance.

The previous candidate cd828d2e (source39ed4e5) passed root independent review
and a fresh full build, then failed its first installed request1509 before a
frame: native compressed allocation returned seven mip levels while the strict
uploader required nine. It retained one uncertain bank until process exit;
no repeated request was issued. Thus offline approval did not establish native
allocation success. The follow-up preserves the nine-level guard and budget,
compensates the native creation request, and logs failures with bank, layer,
stage, HRESULT and complete COM SRV/resource descriptors.

Read-only PID14656 evidence and the rerunnable inspection are under
`D:/DEV/Kenshi/research/global-satellite-20261031/geography-runtime-20261102/`
and the parent `inspect-geography-quarantine-20261102.py`. The exact executable
and Ogre/D3D module hashes, process creation FILETIME134380418783556007, owned
resource name, actual creator, D3D vtable and stable registry/view fields were
verified. The retained private bank requested additional8, had actualAdditional6
and cached native SRV format77/dimension5/levels7/slices24. This inspection made
no native/COM calls and wrote no process memory.

`[Probe] GeographicBiomes=1` is an explicit opt-in, read when the existing
private request begins; its default is 0. It additionally requires the existing
standalone512, painted, primary1,1 guards. Geometry, request scheduling and
normal captures retain their existing path. No main ZoneMap, callback, native
material or global texture default is edited.

The integrated path loads all 15 biome records and 92 exact DDS inputs for
primary1,1. A world-position 8x8 selector chooses each coarse cell's actual
ordered native SID/channel layout for every fragment. The real 1024 blend PNG
is sampled with world-space derivatives and a full generated mip chain. The
native descending base residual and original RGBA channels determine weights.
There are 39 ordered coarse layouts, 37 unordered SID sets and 57 fine-leaf
ordered combinations; a coarse center is not used as a fragment selector.

The first geographic isolation trial deliberately retains zero distortion,
zero fade distance and dry conditions. Native diffuse/normal role parameters,
brightness, slope factors and RGBA blending are used. This does not establish
final native lighting/effect parity or whole-map support.

## Input, ownership and bounds

`geography-inputs.json` records exact relative paths, lengths, SHA256 identities,
bank/layer mappings, 15 independent DIF/NML role tables, 64 live layout/tree
identities and the blend PNG identity. The generator verifies the actual
RE_Kenshi executable SHA256 `504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e`
and native code windows. Runtime checks those windows and the live 64 layouts
and complete masks before allocating. All 92 DDS are preflighted before any
pool allocation and checked again while streaming uploads. Missing or changed
records, paths, masks, shape, format or hashes refuse the request.

The strict DDS parser accepts only 2048x2048, 12 source levels and the exact
DXT5 diffuse/DXT1 normal header and payload layout. Nine GPU levels preserve
the compressed source blocks byte for byte. Nine total GPU levels are this
pool's chosen policy, not verified native Terrain mip parity. The native
BC allocator drops two requested total levels at4914A..49171: requesting
additional8 returns seven total, so this pool requests additional10 to obtain
the intended nine. The native branch is now guarded by two additional exact
code windows (15 D3D windows total). The same bounded read-only registry snapshot
found 28 main Terrain BC arrays (14 BC1/14 BC3); all requested additional8 and
had seven actual levels. The old single-biome reference accepted that reduction.
The 24-layer DIF, 22-layer DIF and 46-layer NML resources each stay below
128MiB: 134217216, 123032448 and 128624832 bytes respectively. Pool total is
385874496 bytes. Painted arrays, blend and selector bring fixed texture bytes
to 503315252. The existing 1GiB combined cap also includes reported terrain
memory and a 64MiB output allowance. Per-step rechecks do not measure transient
driver allocations. No limit is automatically raised.

Encoded inputs total more than 128MiB and are never retained together: the
loader streams one file, maximum about 5.6MB, with a 128MiB per-source guard.
It checks 2GiB host physical and commit headroom before allocation. Decoded
blend storage, object/codec overhead, terrain and driver memory are additional.

Native arrays/2D images are private owned names. Their exact resource/SRV
descriptors and actual TextureManager creator are verified. Scoped COM uploads
retain no renderer state and touch no OM/shader/viewport state. The actual
creator receives one resource notification, followed by setToLoaded once;
there is no CPU-image writeArray fallback or second native upload. A partially
submitted/credited failure latches one bounded quarantine and prevents retries.
Normal cleanup follows private material/Terrain destruction and verifies sole
ownership before removing textures. One fixed shader identity can use the
existing opt-in cache; no cross-request smart pointers are kept.

All five resources now use the production `CompleteBank` template: availability,
latched uncertainty, allocation, unloaded/creator preflight, complete upload,
native notification, setLoaded and final state verification. Every exception or
failure after allocation leaves uncertainty latched; another attempt refuses
before allocation and Destroy refuses eviction. The isolated test models only
the native boundary while executing this exact production sequencing template.

## Verification

- 2892537 real WARP compressed upload, all-layer/all-mip source-block checks,
  invalid-input-before-write checks, generated RGBA2D mip/alpha and budget checks.
- 65546 real WARP world/SID/channel/residual-weight cases use production HLSL,
  all fine-cell centers, recorded bilinear fixtures and outside bounds.
- 90 real WARP role-pair cases use production diffuse-bank sampling and all
  15x6 independent DIF/NML indexes, including the bank 23/24 boundary.
- Five generated-copy mutants fail: wrong layer, missing last compressed mip,
  omitted RGBA GenerateMips, wrong blend channel and coarse-center selection.
- 69 production geographic allocation/upload/ownership/accounting checks cover
  existing names, already-Loaded/wrong-creator resources, partial writes,
  exceptions after native credit, failed setLoaded and repeated attempts.
  Two further mutants omit notification and clear uncertainty before credit.
  The request compensation regression was observed RED with8 and GREEN with10;
  a real WARP reproduction of the native seven-level texture is refused by the
  unchanged nine-level upload guard, preserves poisoned mip0 and reports the
  actual seven-level SRV/resource descriptors through the production snapshot.
- Actual geography DXBC reflection verifies eight SRVs t0..t7, three samplers
  s0/s2/s4 and the 225-float4 biome table. Shader source is 24120 ASCII bytes.
- Existing 135194 mip-upload WARP checks, accounting mutants, private capture,
  native ABI/source/PE and all other build checks pass. Native imports stay 156.

The new FXC potential-uninitialized warning was removed with an initialized
shared diffuse-bank helper. Existing native shader X3206 and CaptureProbe
C4018 warnings remain. Tests do not prove game runtime timing, leak freedom,
geographic pixel acceptance or seam/normal correctness after real binding.

## Remaining acceptance

The scoped integration and targeted-fix reviews are complete. A new installed
trial must
confirm primary1,1 geography, finite normal/GBuffer output, untouched geometry
seams, repeat resource/accounting balance, cancellation and resource lifetime,
CPU/frame time and memory behavior. Full native distortion/fade/wet effects,
native PNG-vs-array parity and expansion beyond this fixed region remain separate.

For this first trial the shader still evaluates each selected coarse-root biome,
up to four, even when a fine-fragment weight is exactly zero. Derivatives are
computed before the loop, but no zero-weight shortcut was introduced: avoiding
that work needs a shared full material-output comparison, including negative
residual weights and nonfinite inputs. GPU runtime cost remains unmeasured.
