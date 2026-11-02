# Registered DistantTown model implementation plan

Goal: one already registered static native DistantTown Entity, read-only native
snapshot and private256 D3D11 GBuffer raster. No active viewer integration.
Spec: D:/DEV/Kenshi/research/gsm-building-render-20261102/report.md.

Use brainstorming/design and TDD techniques; the user bypasses skill-only approval
ceremonies. Work only in this new folder. C++03/MSVC10/SDK7.1. No native loaders,
clones, visibility/node/material mutation, injection, process calls or installs.

API boundaries:

* `AtlasModels::Packet`: bounded COM-owned VB/IB/SRV/sampler references, copied
  input declaration/draw metadata/full world matrix; no native pointers.
* `AtlasModels::Raster::Init(device,VSbytes,PSbytes,callerBytes)` creates only
  three256 GBuffer targets and D32 depth (1,048,576B), constant buffers and fixed
  state. `Render(enabled,packet,View,callerBytes)` returns S_FALSE before any
  query when disabled; validates all inputs before clearing its private targets.
  GBuffer SRVs are read-only outputs. No HDR/exposure in this first layer.
* `NativeModels::Snapshot::Read(enabled,Town*,expectedScene,generation,
  sourceProfileVerified)` admits precisely one existing owner/state3 Entity,
  validates current native registration/material/declaration, copies metadata,
  retains real COM sources. Disabled Read clears and performs no world/profile
  lookup. Snapshot::Clear releases only COM references. Root supplies current
  generation and destroys/invalidates snapshot on world changes.

Files: ModelPolicy.h (bounds/metadata), GraphicsState.h (exact touched graphics
state), AtlasModel.h (COM packet/raster), NativeModel.h (guarded admission),
generate_models.py (native source/profile+shader assets), tests and run_tests.ps1.
Pinned original distant_town.hlsl is compiled without function edits; a small
uniform-signature adapter binds original VS/PS entry points. No fake geometry
or default material substitutes. Synthetic triangles exist only in WARP tests.

1. RED: production-state/disabled/budget/metadata fixtures against stubs.
2. GREEN: complete graphics keeper and fixed256 raster; actual native shader
   oracle covers split streams/colour/index offsets/world/origin/depth/state.
3. Native admission: prove and pin each native getter/layout/registration;
   use SubMesh::_getRenderOperation(0), never original SubEntity operation.
   Compile-check actual header and extracted admission sequencing/refusals.
4. Full fresh tests, compiled sensitive mutants, reflection/source hashes,
   ownership/budget report and peer review. No main build/install/commit.

Bounds: one object,16 draws/streams,64 declaration elements,65,536 vertices,
196,608 indices,32MiB unique borrowed buffer bytes,64KiB snapshot metadata.
Private resource ledger includes target bytes+constant buffers. It references
the shared AtlasView::GpuCap (now192MiB by root's quality change), with no local
cap constant or automatic increase. Current144-window+water caller192,414,256B
plus model1,048,720B =193,462,976B; headroom7,863,616B. The ≤32MiB borrowed
geometry retention is counted separately from new owned allocations.
Runtime candidate availability/native visual acceptance belongs to root later.
