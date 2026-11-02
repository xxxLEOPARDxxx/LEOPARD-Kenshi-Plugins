# Owned fixed-terrain raster primitive

This header is an isolated D3D11.0/C++03 pass. It is not connected to Main, the
probe, a game process, the viewer or an atlas writer. It introduces no cache
format or installed map. The only supported material domain is the frozen
compact two-SID cell10 control, with the exact fixture parameters/distortion.
Those defaults are not current native weather or active resource-winner proof.

`Raster::Init(device,shaders,samples,grid,callerGpuBytes)` borrows the validated
height view and supplied shader bytecode only for Init. `Grid` is a rectangle of
1..16 globally aligned 65-square patches, with a nonzero caller epoch. The
complete two-sample source halo and destination shape are checked before owner
creation. Source identity, byte order, real payload and epoch validation belong
to the outer adapter; this typed borrowed source cannot authenticate a file.
Vertex/index buffers are immutable. No CPU mesh/source pointer is retained.

`Shaders` contains the frozen compact VS/PS bytecode. Init checks the exact byte
count and SHA256, including the reflected ABI: VS448 and PS3856 bytes at b0.
The literal native 40-byte POSITION4/NORMAL3/BINORMAL3 declaration, identity
world, absolute positions, morph0 and fifteen vertex distortion slots remain.
The shaders themselves are not copied or rewritten by this helper.

`Render(enabled,sources,frame,callerGpuBytes)` borrows a completed
`CompactMaterial::Inputs` staged owner. It requires a serialized owning render
thread and the original device. The source owner must remain unchanged through
the call. This helper does not own/synchronize native data, files or workers.
OFF returns S_FALSE before any validation or D3D query. Enabled Render allocates
no heap memory, creates no resources and makes no file/native/Ogre calls.

Frame is typed: epoch, center X/Z, spacing, camera Y, far clip, water height and
wetness. Reflected constants are constructed internally, preventing arbitrary
foreign CB sizes/layouts/padding. Spacing is in [1,9] m, wetness in [0,1], water
height in [-10000,9800]. Camera is above every owned vertex and at most20000m;
far clip is at most1000000m and must exceed the conservative full radial extent
down to world height0. The full 256-square footprint must fit both owned patch
coverage and cell10 [-18432,-9216] on X/Z. A four-patch block is not sufficient
for arbitrary shifted9m windows; missing terrain is refused, never extrapolated.

The three outputs are256-square SRV/RT textures: native packed RGBA8 material,
native RGBA8 normal/alpha, R32 radial depth. D32 is private. Outputs are borrowed
through Texture/Output; callers retaining COM references across Clear must charge
those retained/pending resources separately. Clear releases only this owner's
references and supports reInit. A successful HRESULT means commands submitted;
it is not a guarantee of arbitrary native inputs producing finite material data.
Current finite/canonical admission is limited to the explicit frozen defaults.

## State and refusal contract

Before mutation, Render refuses stream output and writable CS/OM UAV, RTV or
DSV aliases of any source or owned target. Completed source views are fixed by
the staged owner and checked for device identity. Read aliases of owned outputs
are scoped across all six shader stages using the existing SavedOutputReads.

Existing GraphicsState preserves IA0..15, topology/index/layout, graphics
shaders/classes, VS/PS b0, PS t0/s0, viewport/scissors/rasterizer, OM targets/UAVs,
blend/depth/stencil and predication. A local guard additionally preserves PS
t1..7 and samplers2/4. No other slots or compute shader/buffers/samplers are
written. IA16..31 remain untouched. Foreign predication is disabled during the
pass and restored. OM UAV append counters are retained using the existing
keep-count restoration. There is no ClearState call in production.

Admission failures preserve all foreign state and previous output bytes. Once
commands begin, device failure may leave output contents incomplete; callers
must honor the HRESULT and refuse publication. RAII still restores bindings.
No exhaustive device-removal/OOM-in-driver or game-shutdown claim is made.

## Explicit resource ledger

The caller value includes every other resident/pending/retired GPU owner,
including at least CompactMaterial::SourceBytes=83,885,956. It excludes this
raster's current payload. Init and every enabled Render independently check
both the fixed90MiB control limit and192MiB viewer cap. No automatic cap raise
or resident-count change occurs.

Payload = patches*169000 + shared17144 + targets1048576 + constants4304 bytes.
Four patches own1,746,024 bytes; sixteen own3,774,024. Source plus four patches
is85,631,980 bytes, leaving8,739,860 below90MiB for caller readback/composition
and other owners. Shader/state COM objects and driver overhead are not a known
texture/buffer byte payload; the caller still needs explicit driver/headroom
accounting. Init uses one169000-byte stack packet plus the helper's53868-byte
scratch. Render uses bounded stack constants and existing bounded state guards.

## Verification

Run bundled Python `run_tests.py` here; `--skip-mutants` omits only the compiled
runtime negative controls. VC10x64/SDK7.1 and the frozen compact generated
fixtures are required. The runner authenticates shader/parameter/source/manual
control bytes before tests. It reads actual pinned DDS files and already decoded
PNG fixtures serially, through the real staged source owner. It never starts or
attaches to a game process.

The initial compiled unimplemented Init failed the expected success assertion.
Tests compare all three planes byte-for-byte with the independently rendered
compact controls at3m,9m,shifted3m and1m. They also check complete foreign-state
snapshots, six-stage output aliases, pixel reads/samplers, high IA slots, an
actual false predicate, hidden OM append count, OFF/refusals, COM reference
balance, sixteen-patch admission, retained output lifetime and Clear/reInit.
All27 creation sites are independently refused through an isolated real-body
copy; actual created resources must be cleaned and retry must succeed. A
compiled cleanup-loss control must fail this test. Additional compiled controls
exercise budgets, shader identity, charge, epoch/domain, radial clipping, OFF,
SO/aliases, predicate/restoration and geometry layout.

These checks establish this primitive and four control outputs. They do not
prove equal material derivatives across windows, all-world palette/paint seams,
native HDR/light/water/model composition, current source winners, FPS or long-run
memory behavior. Packed YCoCg still needs its neighbour/parity decoding contract.
No completed detail-layer or final RGB map feature is claimed.
