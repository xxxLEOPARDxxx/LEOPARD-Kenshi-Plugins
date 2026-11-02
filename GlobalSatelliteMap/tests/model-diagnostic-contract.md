# One-shot model diagnostic bridge

2026-11-02. New GsmModelDiagnostic.h only, with focused fixtures. No main/build-script edit, final plugin build, game/native getter invocation, installation or commit was performed here.

```cpp
GsmModelDiagnostic::Once modelDiagnostic;
bool fresh=false;
HRESULT hr=modelDiagnostic.Capture(debug,town,scene,generation,
    &GSM::generation,nativeSourceVerified,&fresh);
if(fresh){
    const GsmModelDiagnostic::Report&r=modelDiagnostic.Values();
    // Root's synchronous Debug log: source codes/counts/copied numeric values.
}
// On explicit epoch/session retirement:
modelDiagnostic.Reset();
```

Root supplies the already resolved, lifetime-valid OriginProof Town and current scene, after native source/coordinate validation. This town may be unloaded; an empty loaded-building census is a valid bounded result. A different nearest-loaded-town diagnostic remains a separate caller choice. No town selection/discovery is added by the bridge.

Once::Capture returns S_FALSE before constructing NativeModels::Snapshot when OFF or already attempted in the same generation. OFF preserves the one-attempt latch. A new validated nonzero generation or explicit Reset permits another attempt. Missing source verification, current live generation or town/scene refuses before native access and does not consume the attempt. `fresh` is true only for an actual first attempt, including an unsupported/failing source, so the caller can log exactly once.

Report is value-only: generation, exact modelHr and censusHr, packets, modelValid, modelShadersVerified=false, rawSceneWorld[16], sourceSceneOrigin[3], localBounds[6], modelError[256] and NativeLoadedCensus::Result loaded. Packet exposes no public VertexCount, so none is fabricated. The return code is failed modelHr when present, otherwise censusHr; detailed source HRESULTs remain separate. There is no ErrorLog or logging policy inside the helper. Unsupported native model admission is ordinary diagnostic data, not automatically a player-visible error.

The transient native adapter calls the existing NativeModels::Snapshot::Read with the supplied native source verification, copies only public numeric metadata/LastError, then Clear retires every packet/state/COM reference **before** the loaded census and caller's log. No native object pointer escapes into Report, and Once stores no snapshot/resource/context. Existing snapshot validation may create up to16 sampler/input-layout/rasterizer state objects; root explicitly accepted these short-lived metadata validation objects. No shader, texture, buffer, output, Raster initialization, Decode, draw, exposure, native creation/visibility/load call or GPU context submission is introduced by the helper.

The three model shader sources have not yet been verified by the main file worker. `modelShadersVerified` remains false even when model metadata is valid, and the bridge never executes the embedded shader bytecode used for existing input-layout validation. No automatic GST coordinate conversion is performed. Existing native model profile, owner/registry/type/shape/buffer/material bounds and loaded-census guards remain responsible for source admission.

Failed model metadata is cleared while its HRESULT/message are retained; loaded census may still provide independent information. A failed/throwing census publishes no partial loaded records. C++ source exceptions are converted to fixed codes/messages. A live-generation change after either source retires the entire report to E_ABORT; a stale-town census is skipped when the model acquisition already observed a change. The supplied live-generation pointer and serialized source lifetime must remain valid for the synchronous call. Native snapshot Clear is required to be nonthrowing, as its existing COM release implementation expects.

Storage is one fixed value report below64KiB plus scalar epoch/latch fields; no growing queue/cache or per-frame string exists. Repeated calls check only scalar gates/live generation and do not construct the snapshot. At most one bounded model snapshot and one bounded loaded census run per accepted epoch. Their existing borrowed-buffer/source limits apply, and no COM reference remains after the call. These static/unit bounds are not measured CPU/FPS or long-session memory guarantees.

TDD observed the initial inert helper fail the unverified no-query contract. Fresh `tests/run_model_diagnostic_tests.ps1`:30 direct production-policy checks and40 actual NativeOps/facade checks PASS; eight compiled functional mutants rejected; full real SDK VC10/O2/GL adapter compile PASS. The adapter fixture uses the entire unchanged helper implementation with only owned native-source seams, tests full matrix/origin/bounds copies and exact arguments/codes/error text, packet release before census, no construction on OFF/repeat, and intentionally supplies no VertexCount API. Mutants enforce retirement, one attempt, OFF/unverified gates, world postcheck, failed-source cleanup and unverified shader state. No game module is loaded/invoked by these fixtures.

First real native metadata acquisition, actual packet availability/material support, live getter ABI/serialized callback behavior and combined runtime performance remain root-owned acceptance work before buildings are drawn.
