# Primary 1,1 geographic material trial

## Local candidate and scope

Full `build.ps1` completed with exit 0 on 2026-11-02. Candidate
`build/SatelliteCaptureProbe.dll`: 330240 bytes, SHA256
`cd828d2edf6b080e8bb11b311d613609b5295c04cba074fe86236616c85215bf`,
156 native imports. Full evidence: `build/geography-build.log`.
This candidate has not been installed or run in the game by this source task.
Root independent review cleared the scoped standalone512 trial after reading
the parser/policy, COM uploader, PrivateGeography, Terrain integration and HLSL
generator, then running a fresh full build (session67824, exit0). Its final
candidate hash above supersedes the timestamp-different local build. Root
confirmed 68 completion checks, seven geographic mutants and the full new/old
suites pass. No actionable blocker was found for this limited trial. Installation
and live acceptance remain separate parent-task actions.

This includes the production allocation/completion review follow-up. Build log
SHA256: `c3ca15e8fc4492bf509485de0c7604c960a34748464e35d2da7244e41233431a`.

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
the compressed source blocks byte for byte (native additional-mips minus 3).
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

- 2892528 real WARP compressed upload, all-layer/all-mip source-block checks,
  invalid-input-before-write checks, generated RGBA2D mip/alpha and budget checks.
- 65546 real WARP world/SID/channel/residual-weight cases use production HLSL,
  all fine-cell centers, recorded bilinear fixtures and outside bounds.
- 90 real WARP role-pair cases use production diffuse-bank sampling and all
  15x6 independent DIF/NML indexes, including the bank 23/24 boundary.
- Five generated-copy mutants fail: wrong layer, missing last compressed mip,
  omitted RGBA GenerateMips, wrong blend channel and coarse-center selection.
- 68 production geographic allocation/upload/ownership/accounting checks cover
  existing names, already-Loaded/wrong-creator resources, partial writes,
  exceptions after native credit, failed setLoaded and repeated attempts.
  Two further mutants omit notification and clear uncertainty before credit.
- Actual geography DXBC reflection verifies eight SRVs t0..t7, three samplers
  s0/s2/s4 and the 225-float4 biome table. Shader source is 24120 ASCII bytes.
- Existing 135194 mip-upload WARP checks, accounting mutants, private capture,
  native ABI/source/PE and all other build checks pass. Native imports stay 156.

The new FXC potential-uninitialized warning was removed with an initialized
shared diffuse-bank helper. Existing native shader X3206 and CaptureProbe
C4018 warnings remain. Tests do not prove game runtime timing, leak freedom,
geographic pixel acceptance or seam/normal correctness after real binding.

## Remaining acceptance

Independent review of the scoped integration is complete. An installed trial must
confirm primary1,1 geography, finite normal/GBuffer output, untouched geometry
seams, repeat resource/accounting balance, cancellation and resource lifetime,
CPU/frame time and memory behavior. Full native distortion/fade/wet effects,
native PNG-vs-array parity and expansion beyond this fixed region remain separate.

For this first trial the shader still evaluates each selected coarse-root biome,
up to four, even when a fine-fragment weight is exactly zero. Derivatives are
computed before the loop, but no zero-weight shortcut was introduced: avoiding
that work needs a shared full material-output comparison, including negative
residual weights and nonfinite inputs. GPU runtime cost remains unmeasured.
