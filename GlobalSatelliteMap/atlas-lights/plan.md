# Native local-light value snapshot — implementation plan

2026-11-02. Scope is new atlas-lights files only. No rendering, HLSL, main viewer/MCM, game actions, installation or commits.

1. Implement/test copied value and native clock policies. Preserve exact native daytime ramp rounding and sinf effect operation order. Test a private copied-instruction clock fixture against synthetic memory, without loading a game module.
2. Implement the actual templated production collector using two bounded preallocated4096-value buffers and one synchronous32KiB pointer scratch. OFF makes no native queries. Overflow refuses before enumeration. Root may later process every copied light in32-sized batches; this component imposes no32-light total limit.
3. Implement the pinned raw-ABI native backend. Cache heavy module/export validation once per world generation; use resolved pure getters per light. Read the registered owner/clock fields exactly, verify current scene/concrete light/parent, preserve hidden aggregate-return ABI, and recheck source identity before publishing. Retain no Ogre object pointers after Read.
4. Test actual collector and extracted native methods with owned fixtures: registry bounds, global/scene/generation changes, loaded visible/power/day/effect filtering, point/spot fields, source faults, snapshot retirement and profile caching. Compile the complete native adapter with VC10 without linking/invoking it. Audit exact installed module, export, ownership and shader-source pins.
5. Run functional mutants, independent review and final fresh verification. Document fixed memory/work bounds and remaining runtime/native-frame/HDR/cadence acceptance.

Reference: D:/DEV/Kenshi/research/gsm-local-light-snapshot-20261102/contract.md. Root's override removes the proposal's32-total-light policy: all4096 admitted registered lights remain available. No world scans, scene activation, visibility/power/floor writes, MainLight execution, source I/O in the snapshot, or asynchronous native references are permitted.

Narrow follow-up: capture SourceSceneOrigin[3] from the same expected scene using the proven native getRelativeOrigin hidden-result ABI. Preserve raw positions; compare scene/origin before and after collection and refuse a rebase. Verify root0/-XYZ via exact copied native instructions on owned data, actual publication RED→GREEN,4335 checks/16 mutants, complete adapter compiles and updated19-export/three-origin-window audit. No renderer or coordinate-conversion implementation is added.
