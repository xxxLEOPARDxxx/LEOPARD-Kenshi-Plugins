# Scoped implementation plan

The requested design is one fixed reusable working surface. Retaining a separate mutable copy per streamed entry would grow the cache; mutating the immutable base cannot remove an old additive model. The chosen single surface therefore copies pristine base planes first, then leaves model replay/lighting to the caller. No main integration or model-selection policy is part of this change.

1. Write actual WARP copy/reset/source-preservation/state/negative/lifecycle fixtures and compile an inert API. Observe functional RED at the first missing Copy, not a missing-header/compile error.
2. Implement fixed owned textures, strict complete input validation, shared-cap checks, scoped read aliases/predicate, writable-hazard refusal and exact copies.
3. Exercise every partial Init allocation failure via extracted production Init/Clear, retry/destruction/ref-balance and a full native SDK/decode compile.
4. Prove regressions reject compiled faulty implementations for byte copies/direction/height, caps, own-as-pristine, read-state loss, predicate, hazards, source retention and partial lifecycle.
5. Freeze source/tests after a complete fresh suite; document explicit payload, callback/output lifetime, perf limits and hand off for independent review. No install, commit, game call or unrelated file modification.
