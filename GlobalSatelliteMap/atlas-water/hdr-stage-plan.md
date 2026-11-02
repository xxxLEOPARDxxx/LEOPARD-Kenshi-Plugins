# Optional HDR stage

Only WaterPipeline.h, its policy/WARP fixture and generate_pipeline.py plus these
docs change. Existing Render keeps its signature and exact water-OFF early no-op.
No production shader/DXBC/resource/cap changes. Root owns the real local-light
layer, scene mapping and caller integration.

Per-call borrowed HdrStage(context,apply) sits after unexposed terrain HDR and
before optional native water, then one final native exposure. RenderWithStage
accepts optional water+stage; RenderTerrainWithStage uses no water inputs.
Null stage+water OFF returns S_FALSE before queries; a malformed nonnull stage
fails. Stage failure skips final display copy. No callback or native source is
retained. Callback owns its own scratch and restores any additional touched state.
Pipeline scopes existing compute bindings/predicate around the call.

Caller-owned ledger includes the real stage's526,416B private allocation outside
PipelinePrivateBytes. Pipeline bytes remain3,408,256; same shared cap used at
Init/Render. WARP uses a real additive compute fixture if native point-light
layer cannot safely be referenced here; compare its ordering with unmodified
native terrain, opaque water and native exposure. Cover OFF/ON/OFF,locals with
water OFF,failures after private HDR writes,bindings/predicate/counters,+1 budget,
and compiled functional mutations. Independent review before any main install.
