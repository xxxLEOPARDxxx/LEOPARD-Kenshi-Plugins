# Bounded local terrain-HDR light layer

2026-11-02. New files only; frozen value snapshot, main viewer, water pipeline and generators remain unchanged.

1. Require an explicit confirmed translation with matching generation/source-origin stamp. Accept absolute GST bounds and decoded material/normal/height plus caller-owned <=256 RGBA16F terrain HDR. Preserve raw snapshot positions; no inferred origin conversion.
2. Extract the installed deferred light_fs attenuation/spot/BRDF tail and native common functions into ignored generated HLSL. Adapt only known world position, top-down V, runtime point/spot selection and fragment-clip control flow. Keep HDR and coverage. No Tone. Local lights precede opaque water.
3. Own one256 RGBA16F UAV scratch and a2128-byte constant buffer. Process all intersecting4096 values through32-light batches, copying scratch back between batches. Explicit192MiB payload ledger, no per-Render allocation/resource creation, native/context write aliases refused and exact state restored.
4. Compile and run real WARP fixtures, native original point/spot fragment references, multi-batch/all4096 accounting, mapping/bounds/format/device/budget/OFF refusal, coverage/HDR and context/counter restoration. Functional mutants must compile and fail GPU assertions. No game calls, installation or commits.
